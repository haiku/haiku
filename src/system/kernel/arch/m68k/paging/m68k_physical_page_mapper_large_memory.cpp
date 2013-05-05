/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


/*!	Implementation of a physical page mapping strategy (PhysicalPageMapper,
	TranslationMapPhysicalPageMapper) suitable for machines with a lot of
	memory, i.e. more than we can afford to completely map into the kernel
	address space.

	m68k: WRITEME: we use more than 1 pgtable/page
	x86:
	We allocate a single page table (one page) that can map 1024 pages and
	a corresponding virtual address space region (4 MB). Each of those 1024
	slots can map a physical page. We reserve a fixed amount of slots per CPU.
	They will be used for physical operations on that CPU (memset()/memcpy()
	and {get,put}_physical_page_current_cpu()). A few slots we reserve for each
	translation map (TranslationMapPhysicalPageMapper). Those will only be used
	with the translation map locked, mapping a page table page. The remaining
	slots remain in the global pool and are given out by get_physical_page().

	When we run out of slots, we allocate another page table (and virtual
	address space region).
*/


#include "paging/m68k_physical_page_mapper_large_memory.h"

#include <new>

#include <AutoDeleter.h>

#include <cpu.h>
#include <lock.h>
#include <smp.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "paging/m68k_physical_page_mapper.h"
#include "paging/M68KPagingStructures.h"
#include "paging/M68KVMTranslationMap.h"


// The number of slots we reserve per translation map from mapping page tables.
// One slot would suffice, since the map is locked while mapping a page table,
// but we re-use several slots on a LRU-basis so that we can keep the mappings
// a little longer, thus avoiding re-mapping.
#define SLOTS_PER_TRANSLATION_MAP		4

#define USER_SLOTS_PER_CPU				16
#define KERNEL_SLOTS_PER_CPU			16
#define TOTAL_SLOTS_PER_CPU				(USER_SLOTS_PER_CPU \
											+ KERNEL_SLOTS_PER_CPU + 1)
	// one slot is for use in interrupts


using M68KLargePhysicalPageMapper::PhysicalPageSlot;
using M68KLargePhysicalPageMapper::PhysicalPageSlotPool;


class PhysicalPageSlotQueue {
public:
								PhysicalPageSlotQueue();

	inline	PhysicalPageSlot*	GetSlot();
	inline	void				GetSlots(PhysicalPageSlot*& slot1,
									PhysicalPageSlot*& slot2);
	inline	void				PutSlot(PhysicalPageSlot* slot);
	inline	void				PutSlots(PhysicalPageSlot* slot1,
									PhysicalPageSlot* slot2);

private:
			PhysicalPageSlot*	fSlots;
			ConditionVariable	fFreeSlotCondition;
			ConditionVariable	fFreeSlotsCondition;
};


struct PhysicalPageOpsCPUData {
	PhysicalPageSlotQueue	user;
		// Used when copying from/to user memory. This can cause a page fault
		// which might need to memcpy()/memset() a page when being handled.
	PhysicalPageSlotQueue	kernel;
		// Used when memset()ing or when memcpy()ing memory non-user memory.
	PhysicalPageSlot*		interruptSlot;

			void				Init();

private:
	static	PhysicalPageSlot*	_GetInitialSlot();
};


// #pragma mark -


class LargeMemoryTranslationMapPhysicalPageMapper
	: public TranslationMapPhysicalPageMapper {
public:
								LargeMemoryTranslationMapPhysicalPageMapper();
	virtual						~LargeMemoryTranslationMapPhysicalPageMapper();

			status_t			Init();

	virtual	void				Delete();

	virtual	void*				GetPageTableAt(phys_addr_t physicalAddress);

private:
			struct page_slot {
				PhysicalPageSlot*	slot;
				phys_addr_t			physicalAddress;
				cpu_mask_t			valid;
			};

			page_slot			fSlots[SLOTS_PER_TRANSLATION_MAP];
			int32				fSlotCount;	// must be a power of 2
			int32				fNextSlot;
};


class LargeMemoryPhysicalPageMapper : public M68KPhysicalPageMapper {
public:
								LargeMemoryPhysicalPageMapper();

			status_t			Init(kernel_args* args,
									 PhysicalPageSlotPool* initialPool,
									 TranslationMapPhysicalPageMapper*&
									 	_kernelPageMapper);

	virtual	status_t			CreateTranslationMapPhysicalPageMapper(
									TranslationMapPhysicalPageMapper** _mapper);

	virtual	void*				InterruptGetPageTableAt(
									phys_addr_t physicalAddress);

	virtual	status_t			GetPage(phys_addr_t physicalAddress,
									addr_t* virtualAddress, void** handle);
	virtual	status_t			PutPage(addr_t virtualAddress, void* handle);

	virtual	status_t			GetPageCurrentCPU(phys_addr_t physicalAddress,
									addr_t* virtualAddress, void** handle);
	virtual	status_t			PutPageCurrentCPU(addr_t virtualAddress,
									void* handle);

	virtual	status_t			GetPageDebug(phys_addr_t physicalAddress,
									addr_t* virtualAddress, void** handle);
	virtual	status_t			PutPageDebug(addr_t virtualAddress,
									void* handle);

	virtual	status_t			MemsetPhysical(phys_addr_t address, int value,
									phys_size_t length);
	virtual	status_t			MemcpyFromPhysical(void* to, phys_addr_t from,
									size_t length, bool user);
	virtual	status_t			MemcpyToPhysical(phys_addr_t to,
									const void* from, size_t length, bool user);
	virtual	void				MemcpyPhysicalPage(phys_addr_t to,
									phys_addr_t from);

			status_t			GetSlot(bool canWait,
									PhysicalPageSlot*& slot);
			void				PutSlot(PhysicalPageSlot* slot);

	inline	PhysicalPageSlotQueue* GetSlotQueue(int32 cpu, bool user);

private:
	typedef DoublyLinkedList<PhysicalPageSlotPool> PoolList;

			mutex				fLock;
			PoolList			fEmptyPools;
			PoolList			fNonEmptyPools;
			PhysicalPageSlot* fDebugSlot;
			PhysicalPageSlotPool* fInitialPool;
			LargeMemoryTranslationMapPhysicalPageMapper	fKernelMapper;
			PhysicalPageOpsCPUData fPerCPUData[B_MAX_CPU_COUNT];
};

static LargeMemoryPhysicalPageMapper sPhysicalPageMapper;


// #pragma mark - PhysicalPageSlot / PhysicalPageSlotPool


inline void
PhysicalPageSlot::Map(phys_addr_t physicalAddress)
{
	pool->Map(physicalAddress, address);
}


PhysicalPageSlotPool::~PhysicalPageSlotPool()
{
}


inline bool
PhysicalPageSlotPool::IsEmpty() const
{
	return fSlots == NULL;
}


inline PhysicalPageSlot*
PhysicalPageSlotPool::GetSlot()
{
	PhysicalPageSlot* slot = fSlots;
	fSlots = slot->next;
	return slot;
}


inline void
PhysicalPageSlotPool::PutSlot(PhysicalPageSlot* slot)
{
	slot->next = fSlots;
	fSlots = slot;
}


// #pragma mark - PhysicalPageSlotQueue


PhysicalPageSlotQueue::PhysicalPageSlotQueue()
	:
	fSlots(NULL)
{
	fFreeSlotCondition.Init(this, "physical page ops slot queue");
	fFreeSlotsCondition.Init(this, "physical page ops slots queue");
}


PhysicalPageSlot*
PhysicalPageSlotQueue::GetSlot()
{
	InterruptsLocker locker;

	// wait for a free slot to turn up
	while (fSlots == NULL) {
		ConditionVariableEntry entry;
		fFreeSlotCondition.Add(&entry);
		locker.Unlock();
		entry.Wait();
		locker.Lock();
	}

	PhysicalPageSlot* slot = fSlots;
	fSlots = slot->next;

	return slot;
}


void
PhysicalPageSlotQueue::GetSlots(PhysicalPageSlot*& slot1,
	PhysicalPageSlot*& slot2)
{
	InterruptsLocker locker;

	// wait for two free slot to turn up
	while (fSlots == NULL || fSlots->next == NULL) {
		ConditionVariableEntry entry;
		fFreeSlotsCondition.Add(&entry);
		locker.Unlock();
		entry.Wait();
		locker.Lock();
	}

	slot1 = fSlots;
	slot2 = slot1->next;
	fSlots = slot2->next;
}


void
PhysicalPageSlotQueue::PutSlot(PhysicalPageSlot* slot)
{
	InterruptsLocker locker;

	slot->next = fSlots;
	fSlots = slot;

	if (slot->next == NULL)
		fFreeSlotCondition.NotifyAll();
	else if (slot->next->next == NULL)
		fFreeSlotCondition.NotifyAll();
}


void
PhysicalPageSlotQueue::PutSlots(PhysicalPageSlot* slot1,
	PhysicalPageSlot* slot2)
{
	InterruptsLocker locker;

	slot1->next = slot2;
	slot2->next = fSlots;
	fSlots = slot1;

	if (slot2->next == NULL)
		fFreeSlotCondition.NotifyAll();
	else if (slot2->next->next == NULL)
		fFreeSlotCondition.NotifyAll();
}


// #pragma mark - PhysicalPageOpsCPUData


void
PhysicalPageOpsCPUData::Init()
{
	for (int32 i = 0; i < USER_SLOTS_PER_CPU; i++)
		user.PutSlot(_GetInitialSlot());
	for (int32 i = 0; i < KERNEL_SLOTS_PER_CPU; i++)
		kernel.PutSlot(_GetInitialSlot());
	interruptSlot = _GetInitialSlot();
}


/* static */ PhysicalPageSlot*
PhysicalPageOpsCPUData::_GetInitialSlot()
{
	PhysicalPageSlot* slot;
	status_t error = sPhysicalPageMapper.GetSlot(false, slot);
	if (error != B_OK) {
		panic("PhysicalPageOpsCPUData::Init(): Failed to get initial "
			"physical page slots! Probably too many CPUs.");
		return NULL;
	}

	return slot;
}


// #pragma mark - LargeMemoryTranslationMapPhysicalPageMapper


LargeMemoryTranslationMapPhysicalPageMapper
	::LargeMemoryTranslationMapPhysicalPageMapper()
	:
	fSlotCount(sizeof(fSlots) / sizeof(page_slot)),
	fNextSlot(0)
{
	memset(fSlots, 0, sizeof(fSlots));
}


LargeMemoryTranslationMapPhysicalPageMapper
	::~LargeMemoryTranslationMapPhysicalPageMapper()
{
	// put our slots back to the global pool
	for (int32 i = 0; i < fSlotCount; i++) {
		if (fSlots[i].slot != NULL)
			sPhysicalPageMapper.PutSlot(fSlots[i].slot);
	}
}


status_t
LargeMemoryTranslationMapPhysicalPageMapper::Init()
{
	// get our slots from the global pool
	for (int32 i = 0; i < fSlotCount; i++) {
		status_t error = sPhysicalPageMapper.GetSlot(true, fSlots[i].slot);
		if (error != B_OK)
			return error;

		// set to invalid physical address, so it won't be used accidentally
		fSlots[i].physicalAddress = ~(phys_addr_t)0;
	}

	return B_OK;
}


void
LargeMemoryTranslationMapPhysicalPageMapper::Delete()
{
	delete this;
}


void*
LargeMemoryTranslationMapPhysicalPageMapper::GetPageTableAt(
	phys_addr_t physicalAddress)
{
	ASSERT(physicalAddress % B_PAGE_SIZE == 0);

	int32 currentCPU = smp_get_current_cpu();

	// maybe the address is already mapped
	for (int32 i = 0; i < fSlotCount; i++) {
		page_slot& slot = fSlots[i];
		if (slot.physicalAddress == physicalAddress) {
			fNextSlot = (i + 1) & (fSlotCount - 1);
			if ((slot.valid & (1 << currentCPU)) == 0) {
				// not valid on this CPU -- invalidate the TLB entry
				arch_cpu_invalidate_TLB_range(slot.slot->address,
					slot.slot->address);
				slot.valid |= 1 << currentCPU;
			}
			return (void*)slot.slot->address;
		}
	}

	// not found -- need to map a fresh one
	page_slot& slot = fSlots[fNextSlot];
	fNextSlot = (fNextSlot + 1) & (fSlotCount - 1);

	slot.physicalAddress = physicalAddress;
	slot.slot->Map(physicalAddress);
	slot.valid = 1 << currentCPU;

	return (void*)slot.slot->address;
}


// #pragma mark - LargeMemoryPhysicalPageMapper


LargeMemoryPhysicalPageMapper::LargeMemoryPhysicalPageMapper()
	:
	fInitialPool(NULL)
{
	mutex_init(&fLock, "large memory physical page mapper");
}


status_t
LargeMemoryPhysicalPageMapper::Init(kernel_args* args,
	PhysicalPageSlotPool* initialPool,
	TranslationMapPhysicalPageMapper*& _kernelPageMapper)
{
	fInitialPool = initialPool;
	fNonEmptyPools.Add(fInitialPool);

	// get the debug slot
	GetSlot(true, fDebugSlot);

	// init the kernel translation map physical page mapper
	status_t error = fKernelMapper.Init();
	if (error != B_OK) {
		panic("LargeMemoryPhysicalPageMapper::Init(): Failed to init "
			"kernel translation map physical page mapper!");
		return error;
	}
	_kernelPageMapper = &fKernelMapper;

	// init the per-CPU data
	int32 cpuCount = smp_get_num_cpus();
	for (int32 i = 0; i < cpuCount; i++)
		fPerCPUData[i].Init();

	return B_OK;
}


status_t
LargeMemoryPhysicalPageMapper::CreateTranslationMapPhysicalPageMapper(
	TranslationMapPhysicalPageMapper** _mapper)
{
	LargeMemoryTranslationMapPhysicalPageMapper* mapper
		= new(std::nothrow) LargeMemoryTranslationMapPhysicalPageMapper;
	if (mapper == NULL)
		return B_NO_MEMORY;

	status_t error = mapper->Init();
	if (error != B_OK) {
		delete mapper;
		return error;
	}

	*_mapper = mapper;
	return B_OK;
}


void*
LargeMemoryPhysicalPageMapper::InterruptGetPageTableAt(
	phys_addr_t physicalAddress)
{
	ASSERT(physicalAddress % B_PAGE_SIZE == 0);

	PhysicalPageSlot* slot = fPerCPUData[smp_get_current_cpu()].interruptSlot;
	slot->Map(physicalAddress);
	return (void*)slot->address;
}


status_t
LargeMemoryPhysicalPageMapper::GetPage(phys_addr_t physicalAddress,
	addr_t* virtualAddress, void** handle)
{
	PhysicalPageSlot* slot;
	status_t error = GetSlot(true, slot);
	if (error != B_OK)
		return error;

	slot->Map(physicalAddress);

	*handle = slot;
	*virtualAddress = slot->address + physicalAddress % B_PAGE_SIZE;

	smp_send_broadcast_ici(SMP_MSG_INVALIDATE_PAGE_RANGE, *virtualAddress,
		*virtualAddress, 0, NULL, SMP_MSG_FLAG_SYNC);

	return B_OK;
}


status_t
LargeMemoryPhysicalPageMapper::PutPage(addr_t virtualAddress, void* handle)
{
	PutSlot((PhysicalPageSlot*)handle);
	return B_OK;
}


status_t
LargeMemoryPhysicalPageMapper::GetPageCurrentCPU(phys_addr_t physicalAddress,
	addr_t* virtualAddress, void** handle)
{
	// get a slot from the per-cpu user pool
	PhysicalPageSlotQueue& slotQueue
		= fPerCPUData[smp_get_current_cpu()].user;
	PhysicalPageSlot* slot = slotQueue.GetSlot();
	slot->Map(physicalAddress);

	*virtualAddress = slot->address + physicalAddress % B_PAGE_SIZE;
	*handle = slot;

	return B_OK;
}


status_t
LargeMemoryPhysicalPageMapper::PutPageCurrentCPU(addr_t virtualAddress,
	void* handle)
{
	// return the slot to the per-cpu user pool
	PhysicalPageSlotQueue& slotQueue
		= fPerCPUData[smp_get_current_cpu()].user;
	slotQueue.PutSlot((PhysicalPageSlot*)handle);
	return B_OK;
}


status_t
LargeMemoryPhysicalPageMapper::GetPageDebug(phys_addr_t physicalAddress,
	addr_t* virtualAddress, void** handle)
{
	fDebugSlot->Map(physicalAddress);

	*handle = fDebugSlot;
	*virtualAddress = fDebugSlot->address + physicalAddress % B_PAGE_SIZE;
	return B_OK;
}


status_t
LargeMemoryPhysicalPageMapper::PutPageDebug(addr_t virtualAddress, void* handle)
{
	return B_OK;
}


status_t
LargeMemoryPhysicalPageMapper::MemsetPhysical(phys_addr_t address, int value,
	phys_size_t length)
{
	addr_t pageOffset = address % B_PAGE_SIZE;

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner _(thread);

	PhysicalPageSlotQueue* slotQueue = GetSlotQueue(thread->cpu->cpu_num,
		false);
	PhysicalPageSlot* slot = slotQueue->GetSlot();

	while (length > 0) {
		slot->Map(address - pageOffset);

		size_t toSet = min_c(length, B_PAGE_SIZE - pageOffset);
		memset((void*)(slot->address + pageOffset), value, toSet);

		length -= toSet;
		address += toSet;
		pageOffset = 0;
	}

	slotQueue->PutSlot(slot);

	return B_OK;
}


status_t
LargeMemoryPhysicalPageMapper::MemcpyFromPhysical(void* _to, phys_addr_t from,
	size_t length, bool user)
{
	uint8* to = (uint8*)_to;
	addr_t pageOffset = from % B_PAGE_SIZE;

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner _(thread);

	PhysicalPageSlotQueue* slotQueue = GetSlotQueue(thread->cpu->cpu_num, user);
	PhysicalPageSlot* slot = slotQueue->GetSlot();

	status_t error = B_OK;

	while (length > 0) {
		size_t toCopy = min_c(length, B_PAGE_SIZE - pageOffset);

		slot->Map(from - pageOffset);

		if (user) {
			error = user_memcpy(to, (void*)(slot->address + pageOffset),
				toCopy);
			if (error != B_OK)
				break;
		} else
			memcpy(to, (void*)(slot->address + pageOffset), toCopy);

		to += toCopy;
		from += toCopy;
		length -= toCopy;
		pageOffset = 0;
	}

	slotQueue->PutSlot(slot);

	return error;
}


status_t
LargeMemoryPhysicalPageMapper::MemcpyToPhysical(phys_addr_t to,
	const void* _from, size_t length, bool user)
{
	const uint8* from = (const uint8*)_from;
	addr_t pageOffset = to % B_PAGE_SIZE;

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner _(thread);

	PhysicalPageSlotQueue* slotQueue = GetSlotQueue(thread->cpu->cpu_num, user);
	PhysicalPageSlot* slot = slotQueue->GetSlot();

	status_t error = B_OK;

	while (length > 0) {
		size_t toCopy = min_c(length, B_PAGE_SIZE - pageOffset);

		slot->Map(to - pageOffset);

		if (user) {
			error = user_memcpy((void*)(slot->address + pageOffset), from,
				toCopy);
			if (error != B_OK)
				break;
		} else
			memcpy((void*)(slot->address + pageOffset), from, toCopy);

		to += toCopy;
		from += toCopy;
		length -= toCopy;
		pageOffset = 0;
	}

	slotQueue->PutSlot(slot);

	return error;
}


void
LargeMemoryPhysicalPageMapper::MemcpyPhysicalPage(phys_addr_t to,
	phys_addr_t from)
{
	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner _(thread);

	PhysicalPageSlotQueue* slotQueue = GetSlotQueue(thread->cpu->cpu_num,
		false);
	PhysicalPageSlot* fromSlot;
	PhysicalPageSlot* toSlot;
	slotQueue->GetSlots(fromSlot, toSlot);

	fromSlot->Map(from);
	toSlot->Map(to);

	memcpy((void*)toSlot->address, (void*)fromSlot->address, B_PAGE_SIZE);

	slotQueue->PutSlots(fromSlot, toSlot);
}


status_t
LargeMemoryPhysicalPageMapper::GetSlot(bool canWait, PhysicalPageSlot*& slot)
{
	MutexLocker locker(fLock);

	PhysicalPageSlotPool* pool = fNonEmptyPools.Head();
	if (pool == NULL) {
		if (!canWait)
			return B_WOULD_BLOCK;

		// allocate new pool
		locker.Unlock();
		status_t error = fInitialPool->AllocatePool(pool);
		if (error != B_OK)
			return error;
		locker.Lock();

		fNonEmptyPools.Add(pool);
		pool = fNonEmptyPools.Head();
	}

	slot = pool->GetSlot();

	if (pool->IsEmpty()) {
		fNonEmptyPools.Remove(pool);
		fEmptyPools.Add(pool);
	}

	return B_OK;
}


void
LargeMemoryPhysicalPageMapper::PutSlot(PhysicalPageSlot* slot)
{
	MutexLocker locker(fLock);

	PhysicalPageSlotPool* pool = slot->pool;
	if (pool->IsEmpty()) {
		fEmptyPools.Remove(pool);
		fNonEmptyPools.Add(pool);
	}

	pool->PutSlot(slot);
}


inline PhysicalPageSlotQueue*
LargeMemoryPhysicalPageMapper::GetSlotQueue(int32 cpu, bool user)
{
	return user ? &fPerCPUData[cpu].user : &fPerCPUData[cpu].kernel;
}


// #pragma mark - Initialization


status_t
large_memory_physical_page_ops_init(kernel_args* args,
	M68KLargePhysicalPageMapper::PhysicalPageSlotPool* initialPool,
	M68KPhysicalPageMapper*& _pageMapper,
	TranslationMapPhysicalPageMapper*& _kernelPageMapper)
{
	new(&sPhysicalPageMapper) LargeMemoryPhysicalPageMapper;
	sPhysicalPageMapper.Init(args, initialPool, _kernelPageMapper);

	_pageMapper = &sPhysicalPageMapper;
	return B_OK;
}
