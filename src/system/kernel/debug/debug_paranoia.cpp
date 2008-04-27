/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <debug_paranoia.h>

#include <sys/param.h>

#include <new>

#include <OS.h>

#include <tracing.h>
#include <util/AutoLock.h>


#if ENABLE_PARANOIA_CHECKS


// #pragma mark - CRC-32


static const uint32 kCRC32Polynom = 0x04c11db7;
static uint32 sCRC32Table[256];


static uint32
crc32_reflect(uint32 value, int32 bits)
{
	uint32 result = 0;
	for (int32 i = 1; i <= bits; i++) {
		if (value & 1)
			result |= 1 << (bits - i);
		value >>= 1;
	}

	return result;
}


static void
init_crc32_table()
{
	for (int32 i = 0; i < 256; i++) {
		sCRC32Table[i] = crc32_reflect(i, 8) << 24;
		for (int32 k = 0; k < 8; k++) {
			sCRC32Table[i] = (sCRC32Table[i] << 1)
				^ (sCRC32Table[i] & (1 << 31) ? kCRC32Polynom : 0);
		}
		sCRC32Table[i] = crc32_reflect(sCRC32Table[i], 32);
	}
}


static uint32
crc32(const void* _data, size_t size)
{
	uint8* data = (uint8*)_data;
	uint32 crc = 0xffffffff;

	while (size-- > 0) {
		crc = (crc >> 8) ^ sCRC32Table[(crc & 0xff) ^ *data];
		data++;
	}

	return crc;
}


// #pragma mark - ParanoiaCheck[Set]


class ParanoiaCheckSet;

class ParanoiaCheck {
public:
	ParanoiaCheck(const void* address, size_t size)
		:
		fAddress(address),
		fSize(size)
	{
		Update();
	}

	const void*		Address() const	{ return fAddress; }
	size_t			Size() const	{ return fSize; }

	void Update()
	{
		fCheckSum = crc32(fAddress, fSize);
	}

	bool Check() const
	{
		return crc32(fAddress, fSize) == fCheckSum;
	}

private:
	const void*		fAddress;
	size_t			fSize;
	uint32			fCheckSum;
	ParanoiaCheck*	fNext;

	friend class ParanoiaCheckSet;
};


class ParanoiaCheckSet {
public:
	ParanoiaCheckSet(const void* object, const char* description)
		:
		fObject(object),
		fDescription(description),
		fChecks(NULL)
	{
	}

	const void* Object() const		{ return fObject; }
	const char* Description() const	{ return fDescription; }

	ParanoiaCheck* FirstCheck() const
	{
		return fChecks;
	}

	ParanoiaCheck* NextCheck(ParanoiaCheck* check) const
	{
		return check->fNext;
	}

	ParanoiaCheck* FindCheck(const void* address) const
	{
		ParanoiaCheck* check = fChecks;
		while (check != NULL && check->Address() != address)
			check = check->fNext;
		return check;
	}

	void AddCheck(ParanoiaCheck* check)
	{
		check->fNext = fChecks;
		fChecks = check;
	}

	void RemoveCheck(ParanoiaCheck* check)
	{
		if (check == fChecks) {
			fChecks = check->fNext;
			return;
		}

		ParanoiaCheck* previous = fChecks;
		while (previous != NULL && previous->fNext != check)
			previous = previous->fNext;

		// if previous is NULL (which it shouldn't be), just crash here
		previous->fNext = check->fNext;
	}

	ParanoiaCheck* RemoveFirstCheck()
	{
		ParanoiaCheck* check = fChecks;
		if (check == NULL)
			return NULL;

		fChecks = check->fNext;
		return check;
	}

	void SetHashNext(ParanoiaCheckSet* next)
	{
		fHashNext = next;
	}

	ParanoiaCheckSet* HashNext() const
	{
		return fHashNext;
	}

private:
	const void*			fObject;
	const char*			fDescription;
	ParanoiaCheck*		fChecks;
	ParanoiaCheckSet*	fHashNext;
};


union paranoia_slot {
	uint8			check[sizeof(ParanoiaCheck)];
	uint8			checkSet[sizeof(ParanoiaCheckSet)];
	paranoia_slot*	nextFree;
};


// #pragma mark - Tracing


#if PARANOIA_TRACING


namespace ParanoiaTracing {

class ParanoiaTraceEntry : public AbstractTraceEntry {
	public:
		ParanoiaTraceEntry(const void* object)
			:
			fObject(object)
		{
#if PARANOIA_TRACING_STACK_TRACE
		fStackTrace = capture_tracing_stack_trace(PARANOIA_TRACING_STACK_TRACE,
			1, false);
#endif
		}

#if PARANOIA_TRACING_STACK_TRACE
		virtual void DumpStackTrace(TraceOutput& out)
		{
			out.PrintStackTrace(fStackTrace);
		}
#endif

	protected:
		const void*	fObject;
#if PARANOIA_TRACING_STACK_TRACE
		tracing_stack_trace* fStackTrace;
#endif
};


class CreateCheckSet : public ParanoiaTraceEntry {
	public:
		CreateCheckSet(const void* object, const char* description)
			:
			ParanoiaTraceEntry(object)
		{
			fDescription = alloc_tracing_buffer_strcpy(description, 64, false);
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("paranoia create check set: object: %p, "
				"description: \"%s\"", fObject, fDescription);
		}

	private:
		const char*	fDescription;
};


class DeleteCheckSet : public ParanoiaTraceEntry {
	public:
		DeleteCheckSet(const void* object)
			:
			ParanoiaTraceEntry(object)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("paranoia delete check set: object: %p", fObject);
		}
};


class SetCheck : public ParanoiaTraceEntry {
	public:
		SetCheck(const void* object, const void* address, size_t size,
				paranoia_set_check_mode mode)
			:
			ParanoiaTraceEntry(object),
			fAddress(address),
			fSize(size),
			fMode(mode)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			const char* mode = "??? op:";
			switch (fMode) {
				case PARANOIA_DONT_FAIL:
					mode = "set:   ";
					break;
				case PARANOIA_FAIL_IF_EXISTS:
					mode = "add:   ";
					break;
				case PARANOIA_FAIL_IF_MISSING:
					mode = "update:";
					break;
			}
			out.Print("paranoia check %s object: %p, address: %p, size: %lu",
				mode, fObject, fAddress, fSize);
		}

	private:
		const void*				fAddress;
		size_t					fSize;
		paranoia_set_check_mode	fMode;
};


class RemoveCheck : public ParanoiaTraceEntry {
	public:
		RemoveCheck(const void* object, const void* address, size_t size)
			:
			ParanoiaTraceEntry(object),
			fAddress(address),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("paranoia check remove: object: %p, address: %p, size: "
				"%lu", fObject, fAddress, fSize);
		}

	private:
		const void*				fAddress;
		size_t					fSize;
		paranoia_set_check_mode	fMode;
};


}	// namespace ParanoiaTracing

#	define T(x)	new(std::nothrow) ParanoiaTracing::x

#else
#	define T(x)
#endif	// PARANOIA_TRACING


// #pragma mark -


#define PARANOIA_HASH_SIZE	PARANOIA_SLOT_COUNT

static paranoia_slot		sSlots[PARANOIA_SLOT_COUNT];
static paranoia_slot*		sSlotFreeList;
static ParanoiaCheckSet*	sCheckSetHash[PARANOIA_HASH_SIZE];
static spinlock				sParanoiaLock;


static paranoia_slot*
allocate_slot()
{
	if (sSlotFreeList == NULL)
		return NULL;

	paranoia_slot* slot = sSlotFreeList;
	sSlotFreeList = slot->nextFree;
	return slot;
}


static void
free_slot(paranoia_slot* slot)
{
	slot->nextFree = sSlotFreeList;
	sSlotFreeList = slot;
}


static void
add_check_set(ParanoiaCheckSet* set)
{
	int slot = (addr_t)set->Object() % PARANOIA_HASH_SIZE;
	set->SetHashNext(sCheckSetHash[slot]);
	sCheckSetHash[slot] = set;
}


static void
remove_check_set(ParanoiaCheckSet* set)
{
	int slot = (addr_t)set->Object() % PARANOIA_HASH_SIZE;
	if (set == sCheckSetHash[slot]) {
		sCheckSetHash[slot] = set->HashNext();
		return;
	}

	ParanoiaCheckSet* previousSet = sCheckSetHash[slot];
	while (previousSet != NULL && previousSet->HashNext() != set)
		previousSet = previousSet->HashNext();

	// if previousSet is NULL (which it shouldn't be), just crash here
	previousSet->SetHashNext(set->HashNext());
}


static ParanoiaCheckSet* 
lookup_check_set(const void* object)
{
	int slot = (addr_t)object % PARANOIA_HASH_SIZE;
	ParanoiaCheckSet* set = sCheckSetHash[slot];
	while (set != NULL && set->Object() != object)
		set = set->HashNext();

	return set;
}

// #pragma mark - public interface


status_t
create_paranoia_check_set(const void* object, const char* description)
{
	T(CreateCheckSet(object, description));

	if (object == NULL) {
		panic("create_paranoia_check_set(): NULL object");
		return B_BAD_VALUE;
	}

	InterruptsSpinLocker _(sParanoiaLock);

	// check, if object is already registered
	ParanoiaCheckSet* set = lookup_check_set(object);
	if (set != NULL) {
		panic("create_paranoia_check_set(): object %p already has a check set",
			object);
		return B_BAD_VALUE;
	}

	// allocate slot
	paranoia_slot* slot = allocate_slot();
	if (slot == NULL) {
		panic("create_paranoia_check_set(): out of free slots");
		return B_NO_MEMORY;
	}

	set = new(slot) ParanoiaCheckSet(object, description);
	add_check_set(set);

	return B_OK;
}


status_t
delete_paranoia_check_set(const void* object)
{
	T(DeleteCheckSet(object));

	InterruptsSpinLocker _(sParanoiaLock);

	// get check set
	ParanoiaCheckSet* set = lookup_check_set(object);
	if (set == NULL) {
		panic("delete_paranoia_check_set(): object %p doesn't have a check set",
			object);
		return B_BAD_VALUE;
	}

	// free all checks
	while (ParanoiaCheck* check = set->RemoveFirstCheck())
		free_slot((paranoia_slot*)check);

	// free check set
	remove_check_set(set);
	free_slot((paranoia_slot*)set);

	return B_OK;
}


status_t
run_paranoia_checks(const void* object)
{
	InterruptsSpinLocker _(sParanoiaLock);

	// get check set
	ParanoiaCheckSet* set = lookup_check_set(object);
	if (set == NULL) {
		panic("run_paranoia_checks(): object %p doesn't have a check set",
			object);
		return B_BAD_VALUE;
	}
	
	status_t error = B_OK;

	ParanoiaCheck* check = set->FirstCheck();
	while (check != NULL) {
		if (!check->Check()) {
			panic("paranoia check failed for object %p (%s), address: %p, "
				"size: %lu", set->Object(), set->Description(),
				check->Address(), check->Size());
			error = B_BAD_DATA;
		}

		check = set->NextCheck(check);
	}

	return error;
}


status_t
set_paranoia_check(const void* object, const void* address, size_t size,
	paranoia_set_check_mode mode)
{
	T(SetCheck(object, address, size, mode));

	InterruptsSpinLocker _(sParanoiaLock);

	// get check set
	ParanoiaCheckSet* set = lookup_check_set(object);
	if (set == NULL) {
		panic("set_paranoia_check(): object %p doesn't have a check set",
			object);
		return B_BAD_VALUE;
	}

	// update check, if already existing
	ParanoiaCheck* check = set->FindCheck(address);
	if (check != NULL) {
		if (mode == PARANOIA_FAIL_IF_EXISTS) {
			panic("set_paranoia_check(): object %p already has a check for "
				"address %p", object, address);
			return B_BAD_VALUE;
		}

		if (check->Size() != size) {
			panic("set_paranoia_check(): changing check sizes not supported");
			return B_BAD_VALUE;
		}

		check->Update();
		return B_OK;
	}

	if (mode == PARANOIA_FAIL_IF_MISSING) {
		panic("set_paranoia_check(): object %p doesn't have a check for "
			"address %p yet", object, address);
		return B_BAD_VALUE;
	}

	// allocate slot
	paranoia_slot* slot = allocate_slot();
	if (slot == NULL) {
		panic("set_paranoia_check(): out of free slots");
		return B_NO_MEMORY;
	}

	check = new(slot) ParanoiaCheck(address, size);
	set->AddCheck(check);

	return B_OK;
}


status_t
remove_paranoia_check(const void* object, const void* address, size_t size)
{
	T(RemoveCheck(object, address, size));

	InterruptsSpinLocker _(sParanoiaLock);

	// get check set
	ParanoiaCheckSet* set = lookup_check_set(object);
	if (set == NULL) {
		panic("remove_paranoia_check(): object %p doesn't have a check set",
			object);
		return B_BAD_VALUE;
	}

	// get check
	ParanoiaCheck* check = set->FindCheck(address);
	if (check == NULL) {
		panic("remove_paranoia_check(): no check for address %p "
			"(object %p (%s))", address, object, set->Description());
		return B_BAD_VALUE;
	}

	if (check->Size() != size) {
		panic("remove_paranoia_check(): changing check sizes not "
			"supported");
		return B_BAD_VALUE;
	}

	set->RemoveCheck(check);
	return B_OK;
}


#endif	// ENABLE_PARANOIA_CHECKS


void
debug_paranoia_init()
{
#if ENABLE_PARANOIA_CHECKS
	// init CRC-32 table
	init_crc32_table();

	// init paranoia slot free list
	for (int32 i = 0; i < PARANOIA_SLOT_COUNT; i++)
		free_slot(&sSlots[i]);
#endif
}
