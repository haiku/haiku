/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef _VIRTIO_H_
#define _VIRTIO_H_


#include <virtio_defs.h>
#include <util/DoublyLinkedList.h>


struct VirtioResources {
	VirtioRegs* volatile regs;
	size_t regsSize;
	int32_t irq;
};

enum IOState {
	ioStateInactive,
	ioStatePending,
	ioStateDone,
	ioStateFailed,
};

enum IOOperation {
	ioOpRead,
	ioOpWrite,
};

struct IORequest {
	IOState state;
	IOOperation op;
	void* buf;
	size_t len;
	IORequest* next;

	IORequest(IOOperation op, void* buf, size_t len): state(ioStateInactive),
		op(op), buf(buf), len(len), next(NULL) {}
};

class VirtioDevice : public DoublyLinkedListLinkImpl<VirtioDevice> {
private:
	VirtioRegs* volatile fRegs;
	size_t fQueueLen;
	VirtioDesc* volatile fDescs;
	VirtioAvail* volatile fAvail;
	VirtioUsed* volatile fUsed;
	uint32_t* fFreeDescs;
	uint32_t fLastUsed;
	IORequest** fReqs;

	int32_t AllocDesc();
	void FreeDesc(int32_t idx);

public:
	VirtioDevice(const VirtioResources& devRes);
	~VirtioDevice();
	inline VirtioRegs* volatile Regs() {return fRegs;}
	void ScheduleIO(IORequest** reqs, uint32 cnt);
	void ScheduleIO(IORequest* req);
	IORequest* ConsumeIO();
	IORequest* WaitIO();
};


void virtio_init();
void virtio_fini();
void virtio_register(addr_t base, size_t len, uint32 irq);

VirtioResources* ThisVirtioDev(uint32 deviceId, int n);

int virtio_input_get_key();
int virtio_input_wait_for_key();

#endif	// _VIRTIO_H_
