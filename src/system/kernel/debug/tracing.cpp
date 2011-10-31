/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <tracing.h>

#include <stdarg.h>
#include <stdlib.h>

#include <algorithm>

#include <arch/debug.h>
#include <debug.h>
#include <elf.h>
#include <int.h>
#include <kernel.h>
#include <team.h>
#include <thread.h>
#include <util/AutoLock.h>
#include <vm/vm.h>


struct tracing_stack_trace {
	int32	depth;
	addr_t	return_addresses[0];
};


#if ENABLE_TRACING

//#define TRACE_TRACING
#ifdef TRACE_TRACING
#	define TRACE(x) dprintf_no_syslog x
#else
#	define TRACE(x) ;
#endif


enum {
	WRAP_ENTRY			= 0x01,
	ENTRY_INITIALIZED	= 0x02,
	BUFFER_ENTRY		= 0x04,
	FILTER_MATCH		= 0x08,
	INVALID_ENTRY		= 0x10,
	CHECK_ENTRY			= 0x20,
};


static const size_t kTraceOutputBufferSize = 10240;
static const size_t kBufferSize = MAX_TRACE_SIZE / sizeof(trace_entry);

static const uint32 kMaxRecoveringErrorCount	= 100;
static const addr_t kMetaDataBaseAddress		= 32 * 1024 * 1024;
static const addr_t kMetaDataBaseEndAddress		= 128 * 1024 * 1024;
static const addr_t kMetaDataAddressIncrement	= 8 * 1024 * 1024;
static const uint32 kMetaDataMagic1 = 'Vali';
static const uint32 kMetaDataMagic2 = 'dTra';
static const uint32 kMetaDataMagic3 = 'cing';

// the maximum we can address with the trace_entry::[previous_]size fields
static const size_t kMaxTracingEntryByteSize
	= ((1 << 13) - 1) * sizeof(trace_entry);


class TracingMetaData {
public:
	static	status_t			Create(TracingMetaData*& _metaData);

	inline	bool				Lock();
	inline	void				Unlock();

	inline	trace_entry*		FirstEntry() const;
	inline	trace_entry*		AfterLastEntry() const;

	inline	uint32				Entries() const;
	inline	uint32				EntriesEver() const;

	inline	void				IncrementEntriesEver();

	inline	char*				TraceOutputBuffer() const;

			trace_entry*		NextEntry(trace_entry* entry);
			trace_entry*		PreviousEntry(trace_entry* entry);

			trace_entry*		AllocateEntry(size_t size, uint16 flags);

private:
			bool				_FreeFirstEntry();
			bool				_MakeSpace(size_t needed);

	static	status_t			_CreateMetaDataArea(bool findPrevious,
									area_id& _area,
									TracingMetaData*& _metaData);
			bool				_InitPreviousTracingData();

private:
			uint32				fMagic1;
			trace_entry*		fBuffer;
			trace_entry*		fFirstEntry;
			trace_entry*		fAfterLastEntry;
			uint32				fEntries;
			uint32				fMagic2;
			uint32				fEntriesEver;
			spinlock			fLock;
			char*				fTraceOutputBuffer;
			phys_addr_t			fPhysicalAddress;
			uint32				fMagic3;
};

static TracingMetaData sDummyTracingMetaData;
static TracingMetaData* sTracingMetaData = &sDummyTracingMetaData;
static bool sTracingDataRecovered = false;


// #pragma mark -


// #pragma mark - TracingMetaData


bool
TracingMetaData::Lock()
{
	acquire_spinlock(&fLock);
	return true;
}


void
TracingMetaData::Unlock()
{
	release_spinlock(&fLock);
}


trace_entry*
TracingMetaData::FirstEntry() const
{
	return fFirstEntry;
}


trace_entry*
TracingMetaData::AfterLastEntry() const
{
	return fAfterLastEntry;
}


uint32
TracingMetaData::Entries() const
{
	return fEntries;
}


uint32
TracingMetaData::EntriesEver() const
{
	return fEntriesEver;
}


void
TracingMetaData::IncrementEntriesEver()
{
	fEntriesEver++;
		// NOTE: Race condition on SMP machines! We should use atomic_add(),
		// though that costs some performance and the information is for
		// informational purpose anyway.
}


char*
TracingMetaData::TraceOutputBuffer() const
{
	return fTraceOutputBuffer;
}


trace_entry*
TracingMetaData::NextEntry(trace_entry* entry)
{
	entry += entry->size;
	if ((entry->flags & WRAP_ENTRY) != 0)
		entry = fBuffer;

	if (entry == fAfterLastEntry)
		return NULL;

	return entry;
}


trace_entry*
TracingMetaData::PreviousEntry(trace_entry* entry)
{
	if (entry == fFirstEntry)
		return NULL;

	if (entry == fBuffer) {
		// beginning of buffer -- previous entry is a wrap entry
		entry = fBuffer + kBufferSize - entry->previous_size;
	}

	return entry - entry->previous_size;
}


trace_entry*
TracingMetaData::AllocateEntry(size_t size, uint16 flags)
{
	if (fAfterLastEntry == NULL || size == 0
		|| size >= kMaxTracingEntryByteSize) {
		return NULL;
	}

	InterruptsSpinLocker _(fLock);

	size = (size + 3) >> 2;
		// 4 byte aligned, don't store the lower 2 bits

	TRACE(("AllocateEntry(%lu), start %p, end %p, buffer %p\n", size * 4,
		fFirstEntry, fAfterLastEntry, fBuffer));

	if (!_MakeSpace(size))
		return NULL;

	trace_entry* entry = fAfterLastEntry;
	entry->size = size;
	entry->flags = flags;
	fAfterLastEntry += size;
	fAfterLastEntry->previous_size = size;

	if (!(flags & BUFFER_ENTRY))
		fEntries++;

	TRACE(("  entry: %p, end %p, start %p, entries %ld\n", entry,
		fAfterLastEntry, fFirstEntry, fEntries));

	return entry;
}


bool
TracingMetaData::_FreeFirstEntry()
{
	TRACE(("  skip start %p, %lu*4 bytes\n", fFirstEntry, fFirstEntry->size));

	trace_entry* newFirst = NextEntry(fFirstEntry);

	if (fFirstEntry->flags & BUFFER_ENTRY) {
		// a buffer entry -- just skip it
	} else if (fFirstEntry->flags & ENTRY_INITIALIZED) {
		// Fully initialized TraceEntry: We could destroy it, but don't do so
		// for sake of robustness. The destructors of tracing entry classes
		// should be empty anyway.
		fEntries--;
	} else {
		// Not fully initialized TraceEntry. We can't free it, since
		// then it's constructor might still write into the memory and
		// overwrite data of the entry we're going to allocate.
		// We can't do anything until this entry can be discarded.
		return false;
	}

	if (newFirst == NULL) {
		// everything is freed -- practically this can't happen, if
		// the buffer is large enough to hold three max-sized entries
		fFirstEntry = fAfterLastEntry = fBuffer;
		TRACE(("_FreeFirstEntry(): all entries freed!\n"));
	} else
		fFirstEntry = newFirst;

	return true;
}


/*!	Makes sure we have needed * 4 bytes of memory at fAfterLastEntry.
	Returns \c false, if unable to free that much.
*/
bool
TracingMetaData::_MakeSpace(size_t needed)
{
	// we need space for fAfterLastEntry, too (in case we need to wrap around
	// later)
	needed++;

	// If there's not enough space (free or occupied) after fAfterLastEntry,
	// we free all entries in that region and wrap around.
	if (fAfterLastEntry + needed > fBuffer + kBufferSize) {
		TRACE(("_MakeSpace(%lu), wrapping around: after last: %p\n", needed,
			fAfterLastEntry));

		// Free all entries after fAfterLastEntry and one more at the beginning
		// of the buffer.
		while (fFirstEntry > fAfterLastEntry) {
			if (!_FreeFirstEntry())
				return false;
		}
		if (fAfterLastEntry != fBuffer && !_FreeFirstEntry())
			return false;

		// just in case _FreeFirstEntry() freed the very last existing entry
		if (fAfterLastEntry == fBuffer)
			return true;

		// mark as wrap entry and actually wrap around
		trace_entry* wrapEntry = fAfterLastEntry;
		wrapEntry->size = 0;
		wrapEntry->flags = WRAP_ENTRY;
		fAfterLastEntry = fBuffer;
		fAfterLastEntry->previous_size = fBuffer + kBufferSize - wrapEntry;
	}

	if (fFirstEntry <= fAfterLastEntry) {
		// buffer is empty or the space after fAfterLastEntry is unoccupied
		return true;
	}

	// free the first entries, until there's enough space
	size_t space = fFirstEntry - fAfterLastEntry;

	if (space < needed) {
		TRACE(("_MakeSpace(%lu), left %ld\n", needed, space));
	}

	while (space < needed) {
		space += fFirstEntry->size;

		if (!_FreeFirstEntry())
			return false;
	}

	TRACE(("  out: start %p, entries %ld\n", fFirstEntry, fEntries));

	return true;
}


/*static*/ status_t
TracingMetaData::Create(TracingMetaData*& _metaData)
{
	// search meta data in memory (from previous session)
	area_id area;
	TracingMetaData* metaData;
	status_t error = _CreateMetaDataArea(true, area, metaData);
	if (error == B_OK) {
		if (metaData->_InitPreviousTracingData()) {
			_metaData = metaData;
			return B_OK;
		}

		dprintf("Found previous tracing meta data, but failed to init.\n");

		// invalidate the meta data
		metaData->fMagic1 = 0;
		metaData->fMagic2 = 0;
		metaData->fMagic3 = 0;
		delete_area(area);
	} else
		dprintf("No previous tracing meta data found.\n");

	// no previous tracing data found -- create new one
	error = _CreateMetaDataArea(false, area, metaData);
	if (error != B_OK)
		return error;

	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	area = create_area_etc(B_SYSTEM_TEAM, "tracing log",
		kTraceOutputBufferSize + MAX_TRACE_SIZE, B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, CREATE_AREA_DONT_WAIT,
		&virtualRestrictions, &physicalRestrictions,
		(void**)&metaData->fTraceOutputBuffer);
	if (area < 0)
		return area;

	// get the physical address
	physical_entry physicalEntry;
	if (get_memory_map(metaData->fTraceOutputBuffer, B_PAGE_SIZE,
			&physicalEntry, 1) == B_OK) {
		metaData->fPhysicalAddress = physicalEntry.address;
	} else {
		dprintf("TracingMetaData::Create(): failed to get physical address "
			"of tracing buffer\n");
		metaData->fPhysicalAddress = 0;
	}

	metaData->fBuffer = (trace_entry*)(metaData->fTraceOutputBuffer
		+ kTraceOutputBufferSize);
	metaData->fFirstEntry = metaData->fBuffer;
	metaData->fAfterLastEntry = metaData->fBuffer;

	metaData->fEntries = 0;
	metaData->fEntriesEver = 0;
	B_INITIALIZE_SPINLOCK(&metaData->fLock);

	metaData->fMagic1 = kMetaDataMagic1;
	metaData->fMagic2 = kMetaDataMagic2;
	metaData->fMagic3 = kMetaDataMagic3;

	_metaData = metaData;
	return B_OK;
}


/*static*/ status_t
TracingMetaData::_CreateMetaDataArea(bool findPrevious, area_id& _area,
	TracingMetaData*& _metaData)
{
	// search meta data in memory (from previous session)
	TracingMetaData* metaData;
	phys_addr_t metaDataAddress = kMetaDataBaseAddress;
	for (; metaDataAddress <= kMetaDataBaseEndAddress;
			metaDataAddress += kMetaDataAddressIncrement) {
		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions = {};
		physicalRestrictions.low_address = metaDataAddress;
		physicalRestrictions.high_address = metaDataAddress + B_PAGE_SIZE;
		area_id area = create_area_etc(B_SYSTEM_TEAM, "tracing metadata",
			B_PAGE_SIZE, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			CREATE_AREA_DONT_CLEAR, &virtualRestrictions, &physicalRestrictions,
			(void**)&metaData);
		if (area < 0)
			continue;

		if (!findPrevious) {
			_area = area;
			_metaData = metaData;
			return B_OK;
		}

		if (metaData->fMagic1 == kMetaDataMagic1
			&& metaData->fMagic2 == kMetaDataMagic2
			&& metaData->fMagic3 == kMetaDataMagic3) {
			_area = area;
			_metaData = metaData;
			return B_OK;
		}

		delete_area(area);
	}

	return B_ENTRY_NOT_FOUND;
}


bool
TracingMetaData::_InitPreviousTracingData()
{
	// TODO: ATM re-attaching the previous tracing buffer doesn't work very
	// well. The entries should checked more thoroughly for validity -- e.g. the
	// pointers to the entries' vtable pointers could be invalid, which can
	// make the "traced" command quite unusable. The validity of the entries
	// could be checked in a safe environment (i.e. with a fault handler) with
	// typeid() and call of a virtual function.
	return false;

	addr_t bufferStart
		= (addr_t)fTraceOutputBuffer + kTraceOutputBufferSize;
	addr_t bufferEnd = bufferStart + MAX_TRACE_SIZE;

	if (bufferStart > bufferEnd || (addr_t)fBuffer != bufferStart
		|| (addr_t)fFirstEntry % sizeof(trace_entry) != 0
		|| (addr_t)fFirstEntry < bufferStart
		|| (addr_t)fFirstEntry + sizeof(trace_entry) >= bufferEnd
		|| (addr_t)fAfterLastEntry % sizeof(trace_entry) != 0
		|| (addr_t)fAfterLastEntry < bufferStart
		|| (addr_t)fAfterLastEntry > bufferEnd
		|| fPhysicalAddress == 0) {
		dprintf("Failed to init tracing meta data: Sanity checks "
			"failed.\n");
		return false;
	}

	// re-map the previous tracing buffer
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address = fTraceOutputBuffer;
	virtualRestrictions.address_specification = B_EXACT_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	physicalRestrictions.low_address = fPhysicalAddress;
	physicalRestrictions.high_address = fPhysicalAddress
		+ ROUNDUP(kTraceOutputBufferSize + MAX_TRACE_SIZE, B_PAGE_SIZE);
	area_id area = create_area_etc(B_SYSTEM_TEAM, "tracing log",
		kTraceOutputBufferSize + MAX_TRACE_SIZE, B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, CREATE_AREA_DONT_CLEAR,
		&virtualRestrictions, &physicalRestrictions, NULL);
	if (area < 0) {
		dprintf("Failed to init tracing meta data: Mapping tracing log "
			"buffer failed: %s\n", strerror(area));
		return false;
	}

	dprintf("ktrace: Remapped tracing buffer at %p, size: %" B_PRIuSIZE "\n",
		fTraceOutputBuffer, kTraceOutputBufferSize + MAX_TRACE_SIZE);

	// verify/repair the tracing entry list
	uint32 errorCount = 0;
	uint32 entryCount = 0;
	uint32 nonBufferEntryCount = 0;
	uint32 previousEntrySize = 0;
	trace_entry* entry = fFirstEntry;
	while (errorCount <= kMaxRecoveringErrorCount) {
		// check previous entry size
		if (entry->previous_size != previousEntrySize) {
			if (entry != fFirstEntry) {
				dprintf("ktrace recovering: entry %p: fixing previous_size "
					"size: %lu (should be %lu)\n", entry, entry->previous_size,
					previousEntrySize);
				errorCount++;
			}
			entry->previous_size = previousEntrySize;
		}

		if (entry == fAfterLastEntry)
			break;

		// check size field
		if ((entry->flags & WRAP_ENTRY) == 0 && entry->size == 0) {
			dprintf("ktrace recovering: entry %p: non-wrap entry size is 0\n",
				entry);
			errorCount++;
			fAfterLastEntry = entry;
			break;
		}

		if (entry->size > uint32(fBuffer + kBufferSize - entry)) {
			dprintf("ktrace recovering: entry %p: size too big: %lu\n", entry,
				entry->size);
			errorCount++;
			fAfterLastEntry = entry;
			break;
		}

		if (entry < fAfterLastEntry && entry + entry->size > fAfterLastEntry) {
			dprintf("ktrace recovering: entry %p: entry crosses "
				"fAfterLastEntry (%p)\n", entry, fAfterLastEntry);
			errorCount++;
			fAfterLastEntry = entry;
			break;
		}

		// check for wrap entry
		if ((entry->flags & WRAP_ENTRY) != 0) {
			if ((uint32)(fBuffer + kBufferSize - entry)
					> kMaxTracingEntryByteSize / sizeof(trace_entry)) {
				dprintf("ktrace recovering: entry %p: wrap entry at invalid "
					"buffer location\n", entry);
				errorCount++;
			}

			if (entry->size != 0) {
				dprintf("ktrace recovering: entry %p: invalid wrap entry "
					"size: %lu\n", entry, entry->size);
				errorCount++;
				entry->size = 0;
			}

			previousEntrySize = fBuffer + kBufferSize - entry;
			entry = fBuffer;
			continue;
		}

		if ((entry->flags & BUFFER_ENTRY) == 0) {
			entry->flags |= CHECK_ENTRY;
			nonBufferEntryCount++;
		}

		entryCount++;
		previousEntrySize = entry->size;

		entry += entry->size;
	}

	if (errorCount > kMaxRecoveringErrorCount) {
		dprintf("ktrace recovering: Too many errors.\n");
		fAfterLastEntry = entry;
		fAfterLastEntry->previous_size = previousEntrySize;
	}

	dprintf("ktrace recovering: Recovered %lu entries + %lu buffer entries "
		"from previous session. Expected %lu entries.\n", nonBufferEntryCount,
		entryCount - nonBufferEntryCount, fEntries);
	fEntries = nonBufferEntryCount;

	B_INITIALIZE_SPINLOCK(&fLock);

	// TODO: Actually check the entries! Do that when first accessing the
	// tracing buffer from the kernel debugger (when sTracingDataRecovered is
	// true).
	sTracingDataRecovered = true;
	return true;
}


#endif	// ENABLE_TRACING


// #pragma mark -


TraceOutput::TraceOutput(char* buffer, size_t bufferSize, uint32 flags)
	: fBuffer(buffer),
	  fCapacity(bufferSize),
	  fFlags(flags)
{
	Clear();
}


void
TraceOutput::Clear()
{
	if (fCapacity > 0)
		fBuffer[0] = '\0';
	fSize = 0;
}


void
TraceOutput::Print(const char* format,...)
{
#if ENABLE_TRACING
	if (IsFull())
		return;

	if (fSize < fCapacity) {
		va_list args;
		va_start(args, format);
		size_t length = vsnprintf(fBuffer + fSize, fCapacity - fSize, format,
			args);
		fSize += std::min(length, fCapacity - fSize - 1);
		va_end(args);
	}
#endif
}


void
TraceOutput::PrintStackTrace(tracing_stack_trace* stackTrace)
{
#if ENABLE_TRACING
	if (stackTrace == NULL || stackTrace->depth <= 0)
		return;

	for (int32 i = 0; i < stackTrace->depth; i++) {
		addr_t address = stackTrace->return_addresses[i];

		const char* symbol;
		const char* imageName;
		bool exactMatch;
		addr_t baseAddress;

		if (elf_debug_lookup_symbol_address(address, &baseAddress, &symbol,
				&imageName, &exactMatch) == B_OK) {
			Print("  %p  %s + 0x%lx (%s)%s\n", (void*)address, symbol,
				address - baseAddress, imageName,
				exactMatch ? "" : " (nearest)");
		} else
			Print("  %p\n", (void*)address);
	}
#endif
}


void
TraceOutput::SetLastEntryTime(bigtime_t time)
{
	fLastEntryTime = time;
}


bigtime_t
TraceOutput::LastEntryTime() const
{
	return fLastEntryTime;
}


//	#pragma mark -


TraceEntry::TraceEntry()
{
}


TraceEntry::~TraceEntry()
{
}


void
TraceEntry::Dump(TraceOutput& out)
{
#if ENABLE_TRACING
	// to be overridden by subclasses
	out.Print("ENTRY %p", this);
#endif
}


void
TraceEntry::DumpStackTrace(TraceOutput& out)
{
}


void
TraceEntry::Initialized()
{
#if ENABLE_TRACING
	ToTraceEntry()->flags |= ENTRY_INITIALIZED;
	sTracingMetaData->IncrementEntriesEver();
#endif
}


void*
TraceEntry::operator new(size_t size, const std::nothrow_t&) throw()
{
#if ENABLE_TRACING
	trace_entry* entry = sTracingMetaData->AllocateEntry(
		size + sizeof(trace_entry), 0);
	return entry != NULL ? entry + 1 : NULL;
#endif
	return NULL;
}


//	#pragma mark -


AbstractTraceEntry::~AbstractTraceEntry()
{
}


void
AbstractTraceEntry::Dump(TraceOutput& out)
{
	bigtime_t time = (out.Flags() & TRACE_OUTPUT_DIFF_TIME)
		? fTime - out.LastEntryTime()
		: fTime;

	if (out.Flags() & TRACE_OUTPUT_TEAM_ID)
		out.Print("[%6ld:%6ld] %10Ld: ", fThread, fTeam, time);
	else
		out.Print("[%6ld] %10Ld: ", fThread, time);

	AddDump(out);

	out.SetLastEntryTime(fTime);
}


void
AbstractTraceEntry::AddDump(TraceOutput& out)
{
}


void
AbstractTraceEntry::_Init()
{
	Thread* thread = thread_get_current_thread();
	if (thread != NULL) {
		fThread = thread->id;
		if (thread->team)
			fTeam = thread->team->id;
	}
	fTime = system_time();
}


//	#pragma mark - AbstractTraceEntryWithStackTrace



AbstractTraceEntryWithStackTrace::AbstractTraceEntryWithStackTrace(
	size_t stackTraceDepth, size_t skipFrames, bool kernelOnly)
{
	fStackTrace = capture_tracing_stack_trace(stackTraceDepth, skipFrames + 1,
		kernelOnly);
}


void
AbstractTraceEntryWithStackTrace::DumpStackTrace(TraceOutput& out)
{
	out.PrintStackTrace(fStackTrace);
}


//	#pragma mark -


#if ENABLE_TRACING

class KernelTraceEntry : public AbstractTraceEntry {
	public:
		KernelTraceEntry(const char* message)
		{
			fMessage = alloc_tracing_buffer_strcpy(message, 256, false);

#if KTRACE_PRINTF_STACK_TRACE
			fStackTrace = capture_tracing_stack_trace(
				KTRACE_PRINTF_STACK_TRACE, 1, false);
#endif
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("kern: %s", fMessage);
		}

#if KTRACE_PRINTF_STACK_TRACE
		virtual void DumpStackTrace(TraceOutput& out)
		{
			out.PrintStackTrace(fStackTrace);
		}
#endif

	private:
		char*	fMessage;
#if KTRACE_PRINTF_STACK_TRACE
		tracing_stack_trace* fStackTrace;
#endif
};


class UserTraceEntry : public AbstractTraceEntry {
	public:
		UserTraceEntry(const char* message)
		{
			fMessage = alloc_tracing_buffer_strcpy(message, 256, true);

#if KTRACE_PRINTF_STACK_TRACE
			fStackTrace = capture_tracing_stack_trace(
				KTRACE_PRINTF_STACK_TRACE, 1, false);
#endif
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("user: %s", fMessage);
		}

#if KTRACE_PRINTF_STACK_TRACE
		virtual void DumpStackTrace(TraceOutput& out)
		{
			out.PrintStackTrace(fStackTrace);
		}
#endif

	private:
		char*	fMessage;
#if KTRACE_PRINTF_STACK_TRACE
		tracing_stack_trace* fStackTrace;
#endif
};


class TracingLogStartEntry : public AbstractTraceEntry {
	public:
		TracingLogStartEntry()
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("ktrace start");
		}
};

#endif	// ENABLE_TRACING


//	#pragma mark - trace filters


TraceFilter::~TraceFilter()
{
}


bool
TraceFilter::Filter(const TraceEntry* entry, LazyTraceOutput& out)
{
	return false;
}



class ThreadTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* _entry, LazyTraceOutput& out)
	{
		const AbstractTraceEntry* entry
			= dynamic_cast<const AbstractTraceEntry*>(_entry);
		return (entry != NULL && entry->ThreadID() == fThread);
	}
};


class TeamTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* _entry, LazyTraceOutput& out)
	{
		const AbstractTraceEntry* entry
			= dynamic_cast<const AbstractTraceEntry*>(_entry);
		return (entry != NULL && entry->TeamID() == fTeam);
	}
};


class PatternTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return strstr(out.DumpEntry(entry), fString) != NULL;
	}
};


class DecimalPatternTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		// TODO: this is *very* slow
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%Ld", fValue);
		return strstr(out.DumpEntry(entry), buffer) != NULL;
	}
};

class HexPatternTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		// TODO: this is *very* slow
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%Lx", fValue);
		return strstr(out.DumpEntry(entry), buffer) != NULL;
	}
};

class StringPatternTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		if (IS_KERNEL_ADDRESS(fValue))
			return strstr(out.DumpEntry(entry), (const char*)fValue) != NULL;

		// TODO: this is *very* slow
		char buffer[64];
		user_strlcpy(buffer, (const char*)fValue, sizeof(buffer));
		return strstr(out.DumpEntry(entry), buffer) != NULL;
	}
};

class NotTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return !fSubFilters.first->Filter(entry, out);
	}
};


class AndTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return fSubFilters.first->Filter(entry, out)
			&& fSubFilters.second->Filter(entry, out);
	}
};


class OrTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return fSubFilters.first->Filter(entry, out)
			|| fSubFilters.second->Filter(entry, out);
	}
};


class TraceFilterParser {
public:
	static TraceFilterParser* Default()
	{
		return &sParser;
	}

	bool Parse(int argc, const char* const* argv)
	{
		fTokens = argv;
		fTokenCount = argc;
		fTokenIndex = 0;
		fFilterCount = 0;

		TraceFilter* filter = _ParseExpression();
		return fTokenIndex == fTokenCount && filter != NULL;
	}

	TraceFilter* Filter()
	{
		return &fFilters[0];
	}

private:
	TraceFilter* _ParseExpression()
	{
		const char* token = _NextToken();
		if (!token) {
			// unexpected end of expression
			return NULL;
		}

		if (fFilterCount == MAX_FILTERS) {
			// too many filters
			return NULL;
		}

		if (token[0] == '#') {
			TraceFilter* filter = new(&fFilters[fFilterCount++])
				PatternTraceFilter;
			filter->fString = token + 1;
			return filter;
		} else if (token[0] == 'd' && token[1] == '#') {
			TraceFilter* filter = new(&fFilters[fFilterCount++])
				DecimalPatternTraceFilter;
			filter->fValue = parse_expression(token + 2);
			return filter;
		} else if (token[0] == 'x' && token[1] == '#') {
			TraceFilter* filter = new(&fFilters[fFilterCount++])
				HexPatternTraceFilter;
			filter->fValue = parse_expression(token + 2);
			return filter;
		} else if (token[0] == 's' && token[1] == '#') {
			TraceFilter* filter = new(&fFilters[fFilterCount++])
				StringPatternTraceFilter;
			filter->fValue = parse_expression(token + 2);
			return filter;
		} else if (strcmp(token, "not") == 0) {
			TraceFilter* filter = new(&fFilters[fFilterCount++]) NotTraceFilter;
			if ((filter->fSubFilters.first = _ParseExpression()) != NULL)
				return filter;
			return NULL;
		} else if (strcmp(token, "and") == 0) {
			TraceFilter* filter = new(&fFilters[fFilterCount++]) AndTraceFilter;
			if ((filter->fSubFilters.first = _ParseExpression()) != NULL
				&& (filter->fSubFilters.second = _ParseExpression()) != NULL) {
				return filter;
			}
			return NULL;
		} else if (strcmp(token, "or") == 0) {
			TraceFilter* filter = new(&fFilters[fFilterCount++]) OrTraceFilter;
			if ((filter->fSubFilters.first = _ParseExpression()) != NULL
				&& (filter->fSubFilters.second = _ParseExpression()) != NULL) {
				return filter;
			}
			return NULL;
		} else if (strcmp(token, "thread") == 0) {
			const char* arg = _NextToken();
			if (arg == NULL) {
				// unexpected end of expression
				return NULL;
			}

			TraceFilter* filter = new(&fFilters[fFilterCount++])
				ThreadTraceFilter;
			filter->fThread = strtol(arg, NULL, 0);
			return filter;
		} else if (strcmp(token, "team") == 0) {
			const char* arg = _NextToken();
			if (arg == NULL) {
				// unexpected end of expression
				return NULL;
			}

			TraceFilter* filter = new(&fFilters[fFilterCount++])
				TeamTraceFilter;
			filter->fTeam = strtol(arg, NULL, 0);
			return filter;
		} else {
			// invalid token
			return NULL;
		}
	}

	const char* _CurrentToken() const
	{
		if (fTokenIndex >= 1 && fTokenIndex <= fTokenCount)
			return fTokens[fTokenIndex - 1];
		return NULL;
	}

	const char* _NextToken()
	{
		if (fTokenIndex >= fTokenCount)
			return NULL;
		return fTokens[fTokenIndex++];
	}

private:
	enum { MAX_FILTERS = 32 };

	const char* const*			fTokens;
	int							fTokenCount;
	int							fTokenIndex;
	TraceFilter					fFilters[MAX_FILTERS];
	int							fFilterCount;

	static TraceFilterParser	sParser;
};


TraceFilterParser TraceFilterParser::sParser;


//	#pragma mark -


#if ENABLE_TRACING


TraceEntry*
TraceEntryIterator::Next()
{
	if (fIndex == 0) {
		fEntry = _NextNonBufferEntry(sTracingMetaData->FirstEntry());
		fIndex = 1;
	} else if (fEntry != NULL) {
		fEntry = _NextNonBufferEntry(sTracingMetaData->NextEntry(fEntry));
		fIndex++;
	}

	return Current();
}


TraceEntry*
TraceEntryIterator::Previous()
{
	if (fIndex == (int32)sTracingMetaData->Entries() + 1)
		fEntry = sTracingMetaData->AfterLastEntry();

	if (fEntry != NULL) {
		fEntry = _PreviousNonBufferEntry(
			sTracingMetaData->PreviousEntry(fEntry));
		fIndex--;
	}

	return Current();
}


TraceEntry*
TraceEntryIterator::MoveTo(int32 index)
{
	if (index == fIndex)
		return Current();

	if (index <= 0 || index > (int32)sTracingMetaData->Entries()) {
		fIndex = (index <= 0 ? 0 : sTracingMetaData->Entries() + 1);
		fEntry = NULL;
		return NULL;
	}

	// get the shortest iteration path
	int32 distance = index - fIndex;
	int32 direction = distance < 0 ? -1 : 1;
	distance *= direction;

	if (index < distance) {
		distance = index;
		direction = 1;
		fEntry = NULL;
		fIndex = 0;
	}
	if ((int32)sTracingMetaData->Entries() + 1 - fIndex < distance) {
		distance = sTracingMetaData->Entries() + 1 - fIndex;
		direction = -1;
		fEntry = NULL;
		fIndex = sTracingMetaData->Entries() + 1;
	}

	// iterate to the index
	if (direction < 0) {
		while (fIndex != index)
			Previous();
	} else {
		while (fIndex != index)
			Next();
	}

	return Current();
}


trace_entry*
TraceEntryIterator::_NextNonBufferEntry(trace_entry* entry)
{
	while (entry != NULL && (entry->flags & BUFFER_ENTRY) != 0)
		entry = sTracingMetaData->NextEntry(entry);

	return entry;
}


trace_entry*
TraceEntryIterator::_PreviousNonBufferEntry(trace_entry* entry)
{
	while (entry != NULL && (entry->flags & BUFFER_ENTRY) != 0)
		entry = sTracingMetaData->PreviousEntry(entry);

	return entry;
}


int
dump_tracing_internal(int argc, char** argv, WrapperTraceFilter* wrapperFilter)
{
	int argi = 1;

	// variables in which we store our state to be continuable
	static int32 _previousCount = 0;
	static bool _previousHasFilter = false;
	static bool _previousPrintStackTrace = false;
	static int32 _previousMaxToCheck = 0;
	static int32 _previousFirstChecked = 1;
	static int32 _previousLastChecked = -1;
	static int32 _previousDirection = 1;
	static uint32 _previousEntriesEver = 0;
	static uint32 _previousEntries = 0;
	static uint32 _previousOutputFlags = 0;
	static TraceEntryIterator iterator;

	uint32 entriesEver = sTracingMetaData->EntriesEver();

	// Note: start and index are Pascal-like indices (i.e. in [1, Entries()]).
	int32 start = 0;	// special index: print the last count entries
	int32 count = 0;
	int32 maxToCheck = 0;
	int32 cont = 0;

	bool hasFilter = false;
	bool printStackTrace = false;

	uint32 outputFlags = 0;
	while (argi < argc) {
		if (strcmp(argv[argi], "--difftime") == 0) {
			outputFlags |= TRACE_OUTPUT_DIFF_TIME;
			argi++;
		} else if (strcmp(argv[argi], "--printteam") == 0) {
			outputFlags |= TRACE_OUTPUT_TEAM_ID;
			argi++;
		} else if (strcmp(argv[argi], "--stacktrace") == 0) {
			printStackTrace = true;
			argi++;
		} else
			break;
	}

	if (argi < argc) {
		if (strcmp(argv[argi], "forward") == 0) {
			cont = 1;
			argi++;
		} else if (strcmp(argv[argi], "backward") == 0) {
			cont = -1;
			argi++;
		}
	} else
		cont = _previousDirection;

	if (cont != 0) {
		if (argi < argc) {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
		if (entriesEver == 0 || entriesEver != _previousEntriesEver
			|| sTracingMetaData->Entries() != _previousEntries) {
			kprintf("Can't continue iteration. \"%s\" has not been invoked "
				"before, or there were new entries written since the last "
				"invocation.\n", argv[0]);
			return 0;
		}
	}

	// get start, count, maxToCheck
	int32* params[3] = { &start, &count, &maxToCheck };
	for (int i = 0; i < 3 && !hasFilter && argi < argc; i++) {
		if (strcmp(argv[argi], "filter") == 0) {
			hasFilter = true;
			argi++;
		} else if (argv[argi][0] == '#') {
			hasFilter = true;
		} else {
			*params[i] = parse_expression(argv[argi]);
			argi++;
		}
	}

	// filter specification
	if (argi < argc) {
		hasFilter = true;
		if (strcmp(argv[argi], "filter") == 0)
			argi++;

		if (!TraceFilterParser::Default()->Parse(argc - argi, argv + argi)) {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	int32 direction;
	int32 firstToCheck;
	int32 lastToCheck;

	if (cont != 0) {
		// get values from the previous iteration
		direction = cont;
		count = _previousCount;
		maxToCheck = _previousMaxToCheck;
		hasFilter = _previousHasFilter;
		outputFlags = _previousOutputFlags;
		printStackTrace = _previousPrintStackTrace;

		if (direction < 0)
			start = _previousFirstChecked - 1;
		else
			start = _previousLastChecked + 1;
	} else {
		// defaults for count and maxToCheck
		if (count == 0)
			count = 30;
		if (maxToCheck == 0 || !hasFilter)
			maxToCheck = count;
		else if (maxToCheck < 0)
			maxToCheck = sTracingMetaData->Entries();

		// determine iteration direction
		direction = (start <= 0 || count < 0 ? -1 : 1);

		// validate count and maxToCheck
		if (count < 0)
			count = -count;
		if (maxToCheck < 0)
			maxToCheck = -maxToCheck;
		if (maxToCheck > (int32)sTracingMetaData->Entries())
			maxToCheck = sTracingMetaData->Entries();
		if (count > maxToCheck)
			count = maxToCheck;

		// validate start
		if (start <= 0 || start > (int32)sTracingMetaData->Entries())
			start = max_c(1, sTracingMetaData->Entries());
	}

	if (direction < 0) {
		firstToCheck = max_c(1, start - maxToCheck + 1);
		lastToCheck = start;
	} else {
		firstToCheck = start;
		lastToCheck = min_c((int32)sTracingMetaData->Entries(),
			start + maxToCheck - 1);
	}

	// reset the iterator, if something changed in the meantime
	if (entriesEver == 0 || entriesEver != _previousEntriesEver
		|| sTracingMetaData->Entries() != _previousEntries) {
		iterator.Reset();
	}

	LazyTraceOutput out(sTracingMetaData->TraceOutputBuffer(),
		kTraceOutputBufferSize, outputFlags);

	bool markedMatching = false;
	int32 firstToDump = firstToCheck;
	int32 lastToDump = lastToCheck;

	TraceFilter* filter = NULL;
	if (hasFilter)
		filter = TraceFilterParser::Default()->Filter();

	if (wrapperFilter != NULL) {
		wrapperFilter->Init(filter, direction, cont != 0);
		filter = wrapperFilter;
	}

	if (direction < 0 && filter && lastToCheck - firstToCheck >= count) {
		// iteration direction is backwards
		markedMatching = true;

		// From the last entry to check iterate backwards to check filter
		// matches.
		int32 matching = 0;

		// move to the entry after the last entry to check
		iterator.MoveTo(lastToCheck + 1);

		// iterate backwards
		firstToDump = -1;
		lastToDump = -1;
		while (iterator.Index() > firstToCheck) {
			TraceEntry* entry = iterator.Previous();
			if ((entry->Flags() & ENTRY_INITIALIZED) != 0) {
				out.Clear();
				if (filter->Filter(entry, out)) {
					entry->ToTraceEntry()->flags |= FILTER_MATCH;
					if (lastToDump == -1)
						lastToDump = iterator.Index();
					firstToDump = iterator.Index();

					matching++;
					if (matching >= count)
						break;
				} else
					entry->ToTraceEntry()->flags &= ~FILTER_MATCH;
			}
		}

		firstToCheck = iterator.Index();

		// iterate to the previous entry, so that the next loop starts at the
		// right one
		iterator.Previous();
	}

	out.SetLastEntryTime(0);

	// set the iterator to the entry before the first one to dump
	iterator.MoveTo(firstToDump - 1);

	// dump the entries matching the filter in the range
	// [firstToDump, lastToDump]
	int32 dumped = 0;

	while (TraceEntry* entry = iterator.Next()) {
		int32 index = iterator.Index();
		if (index < firstToDump)
			continue;
		if (index > lastToDump || dumped >= count) {
			if (direction > 0)
				lastToCheck = index - 1;
			break;
		}

		if ((entry->Flags() & ENTRY_INITIALIZED) != 0) {
			out.Clear();
			if (filter &&  (markedMatching
					? (entry->Flags() & FILTER_MATCH) == 0
					: !filter->Filter(entry, out))) {
				continue;
			}

			// don't print trailing new line
			const char* dump = out.DumpEntry(entry);
			int len = strlen(dump);
			if (len > 0 && dump[len - 1] == '\n')
				len--;

			kprintf("%5ld. %.*s\n", index, len, dump);

			if (printStackTrace) {
				out.Clear();
				entry->DumpStackTrace(out);
				if (out.Size() > 0)
					kputs(out.Buffer());
			}
		} else if (!filter)
			kprintf("%5ld. ** uninitialized entry **\n", index);

		dumped++;
	}

	kprintf("printed %ld entries within range %ld to %ld (%ld of %ld total, "
		"%ld ever)\n", dumped, firstToCheck, lastToCheck,
		lastToCheck - firstToCheck + 1, sTracingMetaData->Entries(),
		entriesEver);

	// store iteration state
	_previousCount = count;
	_previousMaxToCheck = maxToCheck;
	_previousHasFilter = hasFilter;
	_previousPrintStackTrace = printStackTrace;
	_previousFirstChecked = firstToCheck;
	_previousLastChecked = lastToCheck;
	_previousDirection = direction;
	_previousEntriesEver = entriesEver;
	_previousEntries = sTracingMetaData->Entries();
	_previousOutputFlags = outputFlags;

	return cont != 0 ? B_KDEBUG_CONT : 0;
}


static int
dump_tracing_command(int argc, char** argv)
{
	return dump_tracing_internal(argc, argv, NULL);
}


#endif	// ENABLE_TRACING


extern "C" uint8*
alloc_tracing_buffer(size_t size)
{
#if	ENABLE_TRACING
	trace_entry* entry = sTracingMetaData->AllocateEntry(
		size + sizeof(trace_entry), BUFFER_ENTRY);
	if (entry == NULL)
		return NULL;

	return (uint8*)(entry + 1);
#else
	return NULL;
#endif
}


uint8*
alloc_tracing_buffer_memcpy(const void* source, size_t size, bool user)
{
	if (user && !IS_USER_ADDRESS(source))
		return NULL;

	uint8* buffer = alloc_tracing_buffer(size);
	if (buffer == NULL)
		return NULL;

	if (user) {
		if (user_memcpy(buffer, source, size) != B_OK)
			return NULL;
	} else
		memcpy(buffer, source, size);

	return buffer;
}


char*
alloc_tracing_buffer_strcpy(const char* source, size_t maxSize, bool user)
{
	if (source == NULL || maxSize == 0)
		return NULL;

	if (user && !IS_USER_ADDRESS(source))
		return NULL;

	// limit maxSize to the actual source string len
	if (user) {
		ssize_t size = user_strlcpy(NULL, source, 0);
			// there's no user_strnlen()
		if (size < 0)
			return 0;
		maxSize = min_c(maxSize, (size_t)size + 1);
	} else
		maxSize = strnlen(source, maxSize - 1) + 1;

	char* buffer = (char*)alloc_tracing_buffer(maxSize);
	if (buffer == NULL)
		return NULL;

	if (user) {
		if (user_strlcpy(buffer, source, maxSize) < B_OK)
			return NULL;
	} else
		strlcpy(buffer, source, maxSize);

	return buffer;
}


tracing_stack_trace*
capture_tracing_stack_trace(int32 maxCount, int32 skipFrames, bool kernelOnly)
{
#if	ENABLE_TRACING
	// page_fault_exception() doesn't allow us to gracefully handle a bad
	// address in the stack trace, if interrupts are disabled, so we always
	// restrict the stack traces to the kernel only in this case. A bad address
	// in the kernel stack trace would still cause a panic(), but this is
	// probably even desired.
	if (!are_interrupts_enabled())
		kernelOnly = true;

	tracing_stack_trace* stackTrace
		= (tracing_stack_trace*)alloc_tracing_buffer(
			sizeof(tracing_stack_trace) + maxCount * sizeof(addr_t));

	if (stackTrace != NULL) {
		stackTrace->depth = arch_debug_get_stack_trace(
			stackTrace->return_addresses, maxCount, 0, skipFrames + 1,
			STACK_TRACE_KERNEL | (kernelOnly ? 0 : STACK_TRACE_USER));
	}

	return stackTrace;
#else
	return NULL;
#endif
}


int
dump_tracing(int argc, char** argv, WrapperTraceFilter* wrapperFilter)
{
#if	ENABLE_TRACING
	return dump_tracing_internal(argc, argv, wrapperFilter);
#else
	return 0;
#endif
}


void
lock_tracing_buffer()
{
#if ENABLE_TRACING
	sTracingMetaData->Lock();
#endif
}


void
unlock_tracing_buffer()
{
#if ENABLE_TRACING
	sTracingMetaData->Unlock();
#endif
}


extern "C" status_t
tracing_init(void)
{
#if	ENABLE_TRACING
	status_t result = TracingMetaData::Create(sTracingMetaData);
	if (result != B_OK) {
		sTracingMetaData = &sDummyTracingMetaData;
		return result;
	}

	new(nothrow) TracingLogStartEntry();

	add_debugger_command_etc("traced", &dump_tracing_command,
		"Dump recorded trace entries",
		"[ --printteam ] [ --difftime ] [ --stacktrace ] "
			"(\"forward\" | \"backward\") "
			"| ([ <start> [ <count> [ <range> ] ] ] "
			"[ #<pattern> | (\"filter\" <filter>) ])\n"
		"Prints recorded trace entries. If \"backward\" or \"forward\" is\n"
		"specified, the command continues where the previous invocation left\n"
		"off, i.e. printing the previous respectively next entries (as many\n"
		"as printed before). In this case the command is continuable, that is\n"
		"afterwards entering an empty line in the debugger will reinvoke it.\n"
		"If no arguments are given, the command continues in the direction\n"
		"of the last invocation.\n"
		"--printteam  - enables printing the entries' team IDs.\n"
		"--difftime   - print difference times for all but the first entry.\n"
		"--stacktrace - print stack traces for entries that captured one.\n"
		"  <start>    - The base index of the entries to print. Depending on\n"
		"               whether the iteration direction is forward or\n"
		"               backward this will be the first or last entry printed\n"
		"               (potentially, if a filter is specified). The index of\n"
		"               the first entry in the trace buffer is 1. If 0 is\n"
		"               specified, the last <count> recorded entries are\n"
		"               printed (iteration direction is backward). Defaults \n"
		"               to 0.\n"
		"  <count>    - The number of entries to be printed. Defaults to 30.\n"
		"               If negative, the -<count> entries before and\n"
		"               including <start> will be printed.\n"
		"  <range>    - Only relevant if a filter is specified. Specifies the\n"
		"               number of entries to be filtered -- depending on the\n"
		"               iteration direction the entries before or after\n"
		"               <start>. If more than <count> entries match the\n"
		"               filter, only the first (forward) or last (backward)\n"
		"               <count> matching entries will be printed. If 0 is\n"
		"               specified <range> will be set to <count>. If -1,\n"
		"               <range> will be set to the number of recorded\n"
		"               entries.\n"
		"  <pattern>  - If specified only entries containing this string are\n"
		"               printed.\n"
		"  <filter>   - If specified only entries matching this filter\n"
		"               expression are printed. The expression can consist of\n"
		"               prefix operators \"not\", \"and\", \"or\", and\n"
		"               filters \"'thread' <thread>\" (matching entries\n"
		"               with the given thread ID), \"'team' <team>\"\n"
						"(matching entries with the given team ID), and\n"
		"               \"#<pattern>\" (matching entries containing the given\n"
		"               string).\n", 0);
#endif	// ENABLE_TRACING
	return B_OK;
}


void
ktrace_printf(const char *format, ...)
{
#if	ENABLE_TRACING
	va_list list;
	va_start(list, format);

	char buffer[256];
	vsnprintf(buffer, sizeof(buffer), format, list);

	va_end(list);

	new(nothrow) KernelTraceEntry(buffer);
#endif	// ENABLE_TRACING
}


void
_user_ktrace_output(const char *message)
{
#if	ENABLE_TRACING
	new(nothrow) UserTraceEntry(message);
#endif	// ENABLE_TRACING
}

