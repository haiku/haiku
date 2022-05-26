/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "virtio.h"

#include <new>
#include <string.h>
#include <malloc.h>
#include <KernelExport.h>


enum {
	maxVirtioDevices = 32,
};


VirtioResources gVirtioDevList[maxVirtioDevices];
int32_t gVirtioDevListLen = 0;

DoublyLinkedList<VirtioDevice> gVirtioDevices;
VirtioDevice* gKeyboardDev = NULL;


void*
aligned_malloc(size_t required_bytes, size_t alignment)
{
    void* p1; // original block
    void** p2; // aligned block
    int offset = alignment - 1 + sizeof(void*);
    if ((p1 = (void*)malloc(required_bytes + offset)) == NULL) {
       return NULL;
    }
    p2 = (void**)(((size_t)(p1) + offset) & ~(alignment - 1));
    p2[-1] = p1;
    return p2;
}


void
aligned_free(void* p)
{
    free(((void**)p)[-1]);
}


VirtioResources*
ThisVirtioDev(uint32 deviceId, int n)
{
	for (int i = 0; i < gVirtioDevListLen; i++) {
		VirtioRegs* volatile regs = gVirtioDevList[i].regs;
		if (regs->signature != kVirtioSignature) continue;
		if (regs->deviceId == deviceId) {
			if (n == 0) return &gVirtioDevList[i]; else n--;
		}
	}
	return NULL;
}


//#pragma mark VirtioDevice

int32_t
VirtioDevice::AllocDesc()
{
	for (size_t i = 0; i < fQueueLen; i++) {
		if ((fFreeDescs[i/32] & (1 << (i % 32))) != 0) {
			fFreeDescs[i/32] &= ~((uint32_t)1 << (i % 32));
			return i;
		}
	}
	return -1;
}


void
VirtioDevice::FreeDesc(int32_t idx)
{
	fFreeDescs[idx/32] |= (uint32_t)1 << (idx % 32);
}


VirtioDevice::VirtioDevice(const VirtioResources& devRes): fRegs(devRes.regs)
{
	gVirtioDevices.Insert(this);

	dprintf("+VirtioDevice\n");

	fRegs->status = 0; // reset

	fRegs->status |= kVirtioConfigSAcknowledge;
	fRegs->status |= kVirtioConfigSDriver;
	dprintf("features: %08x\n", fRegs->deviceFeatures);
	fRegs->status |= kVirtioConfigSFeaturesOk;
	fRegs->status |= kVirtioConfigSDriverOk;

	fRegs->queueSel = 0;
//	dprintf("queueNumMax: %d\n", fRegs->queueNumMax);
	fQueueLen = fRegs->queueNumMax;
	fRegs->queueNum = fQueueLen;
	fLastUsed = 0;

	fDescs = (VirtioDesc*)aligned_malloc(sizeof(VirtioDesc) * fQueueLen, 4096);
	memset(fDescs, 0, sizeof(VirtioDesc) * fQueueLen);
	fAvail = (VirtioAvail*)aligned_malloc(sizeof(VirtioAvail)
		+ sizeof(uint16_t) * fQueueLen, 4096);
	memset(fAvail, 0, sizeof(VirtioAvail) + sizeof(uint16_t) * fQueueLen);
	fUsed = (VirtioUsed*)aligned_malloc(sizeof(VirtioUsed)
		+ sizeof(VirtioUsedItem) * fQueueLen, 4096);
	memset(fUsed, 0, sizeof(VirtioUsed) + sizeof(VirtioUsedItem) * fQueueLen);
	fFreeDescs = new(std::nothrow) uint32_t[(fQueueLen + 31)/32];
	memset(fFreeDescs, 0xff, sizeof(uint32_t) * ((fQueueLen + 31)/32));

	fReqs = new(std::nothrow) IORequest*[fQueueLen];

	fRegs->queueDescLow = (uint32_t)(uint64_t)fDescs;
	fRegs->queueDescHi = (uint32_t)((uint64_t)fDescs >> 32);
	fRegs->queueAvailLow = (uint32_t)(uint64_t)fAvail;
	fRegs->queueAvailHi = (uint32_t)((uint64_t)fAvail >> 32);
	fRegs->queueUsedLow = (uint32_t)(uint64_t)fUsed;
	fRegs->queueUsedHi = (uint32_t)((uint64_t)fUsed >> 32);
/*
	dprintf("fDescs: %p\n", fDescs);
	dprintf("fAvail: %p\n", fAvail);
	dprintf("fUsed: %p\n", fUsed);
*/
	fRegs->queueReady = 1;

	fRegs->config[0] = kVirtioInputCfgIdName;
//	dprintf("name: %s\n", (const char*)(&fRegs->config[8]));
}


VirtioDevice::~VirtioDevice()
{
	gVirtioDevices.Remove(this);
	fRegs->status = 0; // reset
}


void
VirtioDevice::ScheduleIO(IORequest** reqs, uint32 cnt)
{
	if (cnt < 1) return;
	int32_t firstDesc, lastDesc;
	for (uint32 i = 0; i < cnt; i++) {
		int32_t desc = AllocDesc();
		if (desc < 0) {panic("virtio: no more descs"); return;}
		if (i == 0) {
			firstDesc = desc;
		} else {
			fDescs[lastDesc].flags |= kVringDescFlagsNext;
			fDescs[lastDesc].next = desc;
			reqs[i - 1]->next = reqs[i];
		}
		fDescs[desc].addr = (uint64_t)(reqs[i]->buf);
		fDescs[desc].len = reqs[i]->len;
		fDescs[desc].flags = 0;
		fDescs[desc].next = 0;
		switch (reqs[i]->op) {
		case ioOpRead: break;
		case ioOpWrite: fDescs[desc].flags |= kVringDescFlagsWrite; break;
		}
		reqs[i]->state = ioStatePending;
		lastDesc = desc;
	}
	int32_t idx = fAvail->idx % fQueueLen;
	fReqs[idx] = reqs[0];
	fAvail->ring[idx] = firstDesc;
	fAvail->idx++;
	fRegs->queueNotify = 0;
}


void
VirtioDevice::ScheduleIO(IORequest* req)
{
	ScheduleIO(&req, 1);
}


IORequest*
VirtioDevice::ConsumeIO()
{
	if (fUsed->idx == fLastUsed)
		return NULL;

	IORequest* req = fReqs[fLastUsed % fQueueLen];
	fReqs[fLastUsed % fQueueLen] = NULL;
	req->state = ioStateDone;
	int32 desc = fUsed->ring[fLastUsed % fQueueLen].id;
	while (kVringDescFlagsNext & fDescs[desc].flags) {
		int32 nextDesc = fDescs[desc].next;
		FreeDesc(desc);
		desc = nextDesc;
	}
	FreeDesc(desc);
	fLastUsed++;
	return req;
}


IORequest*
VirtioDevice::WaitIO()
{
	while (fUsed->idx == fLastUsed) {}
	return ConsumeIO();
}


//#pragma mark -

void
virtio_register(addr_t base, size_t len, uint32 irq)
{
	VirtioRegs* volatile regs = (VirtioRegs* volatile)base;

	dprintf("virtio_register(0x%" B_PRIxADDR ", 0x%" B_PRIxSIZE ", "
		"%" B_PRIu32 ")\n", base, len, irq);
	dprintf("  signature: 0x%" B_PRIx32 "\n", regs->signature);
	dprintf("  version: %" B_PRIu32 "\n", regs->version);
	dprintf("  device id: %" B_PRIu32 "\n", regs->deviceId);

	if (!(gVirtioDevListLen < maxVirtioDevices)) {
		dprintf("too many VirtIO devices\n");
		return;
	}
	gVirtioDevList[gVirtioDevListLen].regs = regs;
	gVirtioDevList[gVirtioDevListLen].regsSize = len;
	gVirtioDevList[gVirtioDevListLen].irq = irq;
	gVirtioDevListLen++;
}


void
virtio_init()
{
	dprintf("virtio_init()\n");

	int i = 0;
	for (; ; i++) {
		VirtioResources* devRes = ThisVirtioDev(kVirtioDevInput, i);
		if (devRes == NULL) break;
		VirtioRegs* volatile regs = devRes->regs;
		regs->config[0] = kVirtioInputCfgIdName;
		dprintf("virtio_input[%d]: %s\n", i, (const char*)(&regs->config[8]));
		if (i == 0)
			gKeyboardDev = new(std::nothrow) VirtioDevice(*devRes);
	}
	dprintf("virtio_input count: %d\n", i);
	if (gKeyboardDev != NULL) {
		for (int i = 0; i < 4; i++) {
			gKeyboardDev->ScheduleIO(new(std::nothrow) IORequest(ioOpWrite,
				malloc(sizeof(VirtioInputPacket)), sizeof(VirtioInputPacket)));
		}
	}
}


void
virtio_fini()
{
	auto it = gVirtioDevices.GetIterator();
	while (VirtioDevice* dev = it.Next()) {
		dev->Regs()->status = 0; // reset
	}
}


int
virtio_input_get_key()
{
	if (gKeyboardDev == NULL)
		return 0;

	IORequest* req = gKeyboardDev->ConsumeIO();
	if (req == NULL)
		return 0;

	VirtioInputPacket &pkt = *(VirtioInputPacket*)req->buf;

	int key = 0;
	if (pkt.type == 1 && pkt.value == 1)
		key = pkt.code;

	free(req->buf); req->buf = NULL; delete req;
	gKeyboardDev->ScheduleIO(new(std::nothrow) IORequest(ioOpWrite,
		malloc(sizeof(VirtioInputPacket)), sizeof(VirtioInputPacket)));

	return key;
}


int
virtio_input_wait_for_key()
{
	int key = 0;

	do {
		key = virtio_input_get_key();
	} while (key == 0);

	return key;
}
