/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

/*	Implementation of a physical page mapping strategy (PhysicalPageMapper,
	TranslationMapPhysicalPageMapper) suitable for machines with a lot of
	memory, i.e. more than we can afford to completely map into the kernel
	address space.

	We allocate a single page table (one page) that can map 1024 pages and
	a corresponding virtual address space region (4 MB). Each of those 1024
	slots can map a physical page. We reserve a fixed amount of slot per CPU.
	They will be used for physical operations on that CPU (memset()/memcpy()
	and {get,put}_physical_page_current_cpu()). A few slots we reserve for each
	translation map (TranslationMapPhysicalPageMapper). Those will only be used
	with the translation map locked, mapping a page table page. The remaining
	slots remain in the global pool and are given out by get_physical_page().

	When we run out of slots, we allocate another page table (and virtual
	address space region).
*/

#include "x86_physical_page_mapper.h"

#include <new>

#include <AutoDeleter.h>

#include <cpu.h>
#include <lock.h>
#include <smp.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <vm.h>
#include <vm_address_space.h>
#include <vm_translation_map.h>
#include <vm_types.h>

#include "x86_paging.h"


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

static const size_t kPageTableAlignment = 1024 * B_PAGE_SIZE;


struct PhysicalPageSlotPool;

struct PhysicalPageSlot {
	PhysicalPageSlot*			next;
	PhysicalPageSlotPool*		pool;
	addr_t						address;

	inline	void				Map(addr_t physicalAddress);
};


struct PhysicalPageSlotPool : DoublyLinkedListLinkImpl<PhysicalPageSlotPool> {
	area_id					dataArea;
	area_id					virtualArea;
	addr_t					virtualBase;
	page_table_entry*		pageTable;
	PhysicalPageSlot*		slots;

			void				Init(area_id dataArea, void* data,
									area_id virtualArea, addr_t virtualBase);

	inline	bool				IsEmpty() const;

	inline	PhysicalPageSlot*	GetSlot();
	inline	void				PutSlot(PhysicalPageSlot* slot);
};


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

	virtual	page_table_entry*	GetPageTableAt(addr_t physicalAddress);

private:
			struct page_slot {
				PhysicalPageSlot*	slot;
				addr_t					physicalAddress;
				cpu_mask_t				valid;
			};

			page_slot			fSlots[SLOTS_PER_TRANSLATION_MAP];
			int32				fSlotCount;	// must be a power of 2
			int32				fNextSlot;
};


class LargeMemoryPhysicalPageMapper : public PhysicalPageMapper {
public:
								LargeMemoryPhysicalPageMapper();

			status_t			Init(kernel_args* args);
	virtual	status_t			InitPostArea(kernel_args* args);

	virtual	status_t			CreateTranslationMapPhysicalPageMapper(
									TranslationMapPhysicalPageMapper** _mapper);

	virtual	page_table_entry*	InterruptGetPageTableAt(
									addr_t physicalAddress);

	inline	status_t			GetPage(addr_t physicalAddress,
									addr_t* virtualAddress, void** handle);
	inline	status_t			PutPage(addr_t virtualAddress, void* handle);

	inline	status_t			GetPageCurrentCPU(addr_t physicalAddress,
									addr_t* virtualAddress, void** handle);
	inline	status_t			PutPageCurrentCPU(addr_t virtualAddress,
									void* handle);

	inline	status_t			GetPageDebug(addr_t physicalAddress,
									addr_t* virtualAddress, void** handle);
	inline	status_t			PutPageDebug(addr_t virtualAddress,
									void* handle);

			status_t			GetSlot(bool canWait,
									PhysicalPageSlot*& slot);
			void				PutSlot(PhysicalPageSlot* slot);

	inline	PhysicalPageSlotQueue* GetSlotQueue(int32 cpu, bool user);

private:
	static	status_t			_AllocatePool(
									PhysicalPageSlotPool*& _pool);

private:
	typedef DoublyLinkedList<PhysicalPageSlotPool> PoolList;

			mutex				fLock;
			PoolList			fEmptyPools;
			PoolList			fNonEmptyPools;
			PhysicalPageSlot* fDebugSlot;
			PhysicalPageSlotPool fInitialPool;
			LargeMemoryTranslationMapPhysicalPageMapper	fKernelMapper;
			PhysicalPageOpsCPUData fPerCPUData[B_MAX_CPU_COUNT];
};

static LargeMemoryPhysicalPageMapper sPhysicalPageMapper;


// #pragma mark - PhysicalPageSlot / PhysicalPageSlotPool


inline void
PhysicalPageSlot::Map(addr_t physicalAddress)
{
	page_table_entry& pte = pool->pageTable[
		(address - pool->virtualBase) / B_PAGE_SIZE];
	init_page_table_entry(&pte);
	pte.addr = ADDR_SHIFT(physicalAddress);
	pte.user = 0;
	pte.rw = 1;
	pte.present = 1;
	pte.global = 1;

	invalidate_TLB(address);
}


void
PhysicalPageSlotPool::Init(area_id dataArea, void* data,
	area_id virtualArea, addr_t virtualBase)
{
	this->dataArea = dataArea;
	this->virtualArea = virtualArea;
	this->virtualBase = virtualBase;
	pageTable = (page_table_entry*)data;

	// init slot list
	slots = (PhysicalPageSlot*)(pageTable + 1024);
	addr_t slotAddress = virtualBase;
	for (int32 i = 0; i < 1024; i++, slotAddress += B_PAGE_SIZE) {
		PhysicalPageSlot* slot = &slots[i];
		slot->next = slot + 1;
		slot->pool = this;
		slot->address = slotAddress;
	}

	slots[1023].next = NULL;
		// terminate list
}


inline bool
PhysicalPageSlotPool::IsEmpty() const
{
	return slots == NULL;
}


inline PhysicalPageSlot*
PhysicalPageSlotPool::GetSlot()
{
	PhysicalPageSlot* slot = slots;
	slots = slot->next;
	return slot;
}


inline void
PhysicalPageSlotPool::PutSlot(PhysicalPageSlot* slot)
{
	slot->next = slots;
	slots = slot;
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
		fSlots[i].physicalAddress = ~(addr_t)0;
	}

	return B_OK;
}


void
LargeMemoryTranslationMapPhysicalPageMapper::Delete()
{
	delete this;
}


page_table_entry*
LargeMemoryTranslationMapPhysicalPageMapper::GetPageTableAt(
	addr_t physicalAddress)
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
				invalidate_TLB(slot.slot->address);
				slot.valid |= 1 << currentCPU;
			}
			return (page_table_entry*)slot.slot->address;
		}
	}

	// not found -- need to map a fresh one
	page_slot& slot = fSlots[fNextSlot];
	fNextSlot = (fNextSlot + 1) & (fSlotCount - 1);

	slot.physicalAddress = physicalAddress;
	slot.slot->Map(physicalAddress);
	slot.valid = 1 << currentCPU;

	return (page_table_entry*)slot.slot->address;
}


// #pragma mark - LargeMemoryPhysicalPageMapper


LargeMemoryPhysicalPageMapper::LargeMemoryPhysicalPageMapper()
{
	mutex_init(&fLock, "large memory physical page mapper");
}


status_t
LargeMemoryPhysicalPageMapper::Init(kernel_args* args)
{
	// We reserve more, so that we can guarantee to align the base address
	// to page table ranges.
	addr_t virtualBase = vm_allocate_early(args,
		1024 * B_PAGE_SIZE + kPageTableAlignment - B_PAGE_SIZE, 0, 0);
	if (virtualBase == 0) {
		panic("LargeMemoryPhysicalPageMapper::Init(): Failed to reserve "
			"physical page pool space in virtual address space!");
		return B_ERROR;
	}
	virtualBase = (virtualBase + kPageTableAlignment - 1)
		/ kPageTableAlignment * kPageTableAlignment;

	// allocate memory for the page table and data
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[1024]);
	page_table_entry* pageTable = (page_table_entry*)vm_allocate_early(args,
		areaSize, ~0L, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	// prepare the page table
	x86_early_prepare_page_tables(pageTable, virtualBase,
		1024 * B_PAGE_SIZE);

	// init the pool structure and add the initial pool
	fInitialPool.Init(-1, pageTable, -1, (addr_t)virtualBase);
	fNonEmptyPools.Add(&fInitialPool);

	// get the debug slot
	GetSlot(true, fDebugSlot);

	// init the kernel translation map physical page mapper
	status_t error = fKernelMapper.Init();
	if (error != B_OK) {
		panic("LargeMemoryPhysicalPageMapper::Init(): Failed to init "
			"kernel translation map physical page mapper!");
		return error;
	}
	gKernelPhysicalPageMapper = &fKernelMapper;

	// init the per-CPU data
	int32 cpuCount = smp_get_num_cpus();
	for (int32 i = 0; i < cpuCount; i++)
		fPerCPUData[i].Init();

	return B_OK;
}


status_t
LargeMemoryPhysicalPageMapper::InitPostArea(kernel_args* args)
{
	// create an area for the (already allocated) data
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[1024]);
	void* temp = fInitialPool.pageTable;
	area_id area = create_area("physical page pool", &temp,
		B_EXACT_ADDRESS, areaSize, B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK) {
		panic("LargeMemoryPhysicalPageMapper::InitPostArea(): Failed to "
			"create area for physical page pool.");
		return area;
	}
	fInitialPool.dataArea = area;

	// create an area for the virtual address space
	temp = (void*)fInitialPool.virtualBase;
	area = vm_create_null_area(vm_kernel_address_space_id(),
		"physical page pool space", &temp, B_EXACT_ADDRESS,
		1024 * B_PAGE_SIZE);
	if (area < B_OK) {
		panic("LargeMemoryPhysicalPageMapper::InitPostArea(): Failed to "
			"create area for physical page pool space.");
		return area;
	}
	fInitialPool.virtualArea = area;

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


page_table_entry*
LargeMemoryPhysicalPageMapper::InterruptGetPageTableAt(addr_t physicalAddress)
{
	ASSERT(physicalAddress % B_PAGE_SIZE == 0);

	PhysicalPageSlot* slot = fPerCPUData[smp_get_current_cpu()].interruptSlot;
	slot->Map(physicalAddress);
	return (page_table_entry*)slot->address;
}


status_t
LargeMemoryPhysicalPageMapper::GetPage(addr_t physicalAddress,
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
LargeMemoryPhysicalPageMapper::GetPageCurrentCPU(addr_t physicalAddress,
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
LargeMemoryPhysicalPageMapper::GetPageDebug(addr_t physicalAddress,
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
LargeMemoryPhysicalPageMapper::GetSlot(bool canWait,
	PhysicalPageSlot*& slot)
{
	MutexLocker locker(fLock);

	PhysicalPageSlotPool* pool = fNonEmptyPools.Head();
	if (pool == NULL) {
		if (!canWait)
			return B_WOULD_BLOCK;

		// allocate new pool
		locker.Unlock();
		status_t error = _AllocatePool(pool);
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


/* static */ status_t
LargeMemoryPhysicalPageMapper::_AllocatePool(PhysicalPageSlotPool*& _pool)
{
	// create the pool structure
	PhysicalPageSlotPool* pool
		= new(std::nothrow) PhysicalPageSlotPool;
	if (pool == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<PhysicalPageSlotPool> poolDeleter(pool);

	// create an area that can contain the page table and the slot
	// structures
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[1024]);
	void* data;
	area_id dataArea = create_area("physical page pool", &data,
		B_ANY_KERNEL_ADDRESS, PAGE_ALIGN(areaSize), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (dataArea < 0)
		return dataArea;

	// create the null area for the virtual address space
	void* virtualBase;
	area_id virtualArea = vm_create_null_area(
		vm_kernel_address_space_id(), "physical page pool space",
		&virtualBase, B_ANY_KERNEL_BLOCK_ADDRESS, 1024 * B_PAGE_SIZE);
	if (virtualArea < 0) {
		delete_area(dataArea);
		return virtualArea;
	}

	// prepare the page table
	memset(data, 0, B_PAGE_SIZE);

	// get the page table's physical address
	addr_t physicalTable;
	vm_translation_map* map = &vm_kernel_address_space()->translation_map;
	uint32 dummyFlags;
	cpu_status state = disable_interrupts();
	map->ops->query_interrupt(map, (addr_t)data, &physicalTable,
		&dummyFlags);
	restore_interrupts(state);

	// put the page table into the page directory
	int32 index = (addr_t)virtualBase / (B_PAGE_SIZE * 1024);
	page_directory_entry* entry = &map->arch_data->pgdir_virt[index];
	x86_put_pgtable_in_pgdir(entry, physicalTable,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	x86_update_all_pgdirs(index, *entry);

	// init the pool structure
	pool->Init(dataArea, data, virtualArea, (addr_t)virtualBase);
	poolDeleter.Detach();
	_pool = pool;
	return B_OK;
}


// #pragma mark - physical page operations


static status_t
large_memory_get_physical_page(addr_t physicalAddress, addr_t *_virtualAddress,
	void **_handle)
{
	return sPhysicalPageMapper.GetPage(physicalAddress, _virtualAddress,
		_handle);
}


static status_t
large_memory_put_physical_page(addr_t virtualAddress, void *handle)
{
	return sPhysicalPageMapper.PutPage(virtualAddress, handle);
}


static status_t
large_memory_get_physical_page_current_cpu(addr_t physicalAddress,
	addr_t *_virtualAddress, void **_handle)
{
	return sPhysicalPageMapper.GetPageCurrentCPU(physicalAddress,
		_virtualAddress, _handle);
}


static status_t
large_memory_put_physical_page_current_cpu(addr_t virtualAddress, void *handle)
{
	return sPhysicalPageMapper.PutPageCurrentCPU(virtualAddress, handle);
}


static status_t
large_memory_get_physical_page_debug(addr_t physicalAddress,
	addr_t *_virtualAddress, void **_handle)
{
	return sPhysicalPageMapper.GetPageDebug(physicalAddress,
		_virtualAddress, _handle);
}


static status_t
large_memory_put_physical_page_debug(addr_t virtualAddress, void *handle)
{
	return sPhysicalPageMapper.PutPageDebug(virtualAddress, handle);
}


static status_t
large_memory_memset_physical(addr_t address, int value,
	size_t length)
{
	addr_t pageOffset = address % B_PAGE_SIZE;

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner _(thread);

	PhysicalPageSlotQueue* slotQueue = sPhysicalPageMapper.GetSlotQueue(
		thread->cpu->cpu_num, false);
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


static status_t
large_memory_memcpy_from_physical(void* _to, addr_t from, size_t length,
	bool user)
{
	uint8* to = (uint8*)_to;
	addr_t pageOffset = from % B_PAGE_SIZE;

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner _(thread);

	PhysicalPageSlotQueue* slotQueue = sPhysicalPageMapper.GetSlotQueue(
		thread->cpu->cpu_num, user);
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


static status_t
large_memory_memcpy_to_physical(addr_t to, const void* _from, size_t length,
	bool user)
{
	const uint8* from = (const uint8*)_from;
	addr_t pageOffset = to % B_PAGE_SIZE;

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner _(thread);

	PhysicalPageSlotQueue* slotQueue = sPhysicalPageMapper.GetSlotQueue(
		thread->cpu->cpu_num, user);
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


static void
large_memory_memcpy_physical_page(addr_t to, addr_t from)
{
	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner _(thread);

	PhysicalPageSlotQueue* slotQueue = sPhysicalPageMapper.GetSlotQueue(
		thread->cpu->cpu_num, false);
	PhysicalPageSlot* fromSlot;
	PhysicalPageSlot* toSlot;
	slotQueue->GetSlots(fromSlot, toSlot);

	fromSlot->Map(from);
	toSlot->Map(to);

	memcpy((void*)toSlot->address, (void*)fromSlot->address, B_PAGE_SIZE);

	slotQueue->PutSlots(fromSlot, toSlot);
}


// #pragma mark - Initialization


status_t
large_memory_physical_page_ops_init(kernel_args* args,
	vm_translation_map_ops* ops)
{
	gPhysicalPageMapper
		= new(&sPhysicalPageMapper) LargeMemoryPhysicalPageMapper;
	sPhysicalPageMapper.Init(args);

	// init physical ops
	ops->get_physical_page = &large_memory_get_physical_page;
	ops->put_physical_page = &large_memory_put_physical_page;
	ops->get_physical_page_current_cpu
		= &large_memory_get_physical_page_current_cpu;
	ops->put_physical_page_current_cpu
		= &large_memory_put_physical_page_current_cpu;
	ops->get_physical_page_debug = &large_memory_get_physical_page_debug;
	ops->put_physical_page_debug = &large_memory_put_physical_page_debug;

	ops->memset_physical = &large_memory_memset_physical;
	ops->memcpy_from_physical = &large_memory_memcpy_from_physical;
	ops->memcpy_to_physical = &large_memory_memcpy_to_physical;
	ops->memcpy_physical_page = &large_memory_memcpy_physical_page;

	return B_OK;
}
