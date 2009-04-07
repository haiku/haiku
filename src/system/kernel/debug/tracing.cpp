/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <tracing.h>

#include <stdarg.h>
#include <stdlib.h>

#include <arch/debug.h>
#include <debug.h>
#include <elf.h>
#include <int.h>
#include <kernel.h>
#include <team.h>
#include <thread.h>
#include <util/AutoLock.h>


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
	FILTER_MATCH		= 0x08
};


static const size_t kTraceOutputBufferSize = 10240;
static const size_t kBufferSize = MAX_TRACE_SIZE / 4;

static trace_entry* sBuffer;
static trace_entry* sFirstEntry;
static trace_entry* sAfterLastEntry;
static uint32 sEntries;
static uint32 sWritten;
static spinlock sLock;
static char* sTraceOutputBuffer;


static trace_entry*
next_entry(trace_entry* entry)
{
	entry += entry->size;
	if ((entry->flags & WRAP_ENTRY) != 0)
		entry = sBuffer;

	if (entry == sAfterLastEntry)
		return NULL;

	return entry;
}


static trace_entry*
previous_entry(trace_entry* entry)
{
	if (entry == sFirstEntry)
		return NULL;

	if (entry == sBuffer) {
		// beginning of buffer -- previous entry is a wrap entry
		entry = sBuffer + kBufferSize - entry->previous_size;
	}

	return entry - entry->previous_size;
}


static bool
free_first_entry()
{
	TRACE(("  skip start %p, %lu*4 bytes\n", sFirstEntry, sFirstEntry->size));

	trace_entry* newFirst = next_entry(sFirstEntry);

	if (sFirstEntry->flags & BUFFER_ENTRY) {
		// a buffer entry -- just skip it
	} else if (sFirstEntry->flags & ENTRY_INITIALIZED) {
		// fully initialized TraceEntry -- destroy it
		TraceEntry::FromTraceEntry(sFirstEntry)->~TraceEntry();
		sEntries--;
	} else {
		// Not fully initialized TraceEntry. We can't free it, since
		// then it's constructor might still write into the memory and
		// overwrite data of the entry we're going to allocate.
		// We can't do anything until this entry can be discarded.
		return false;
	}

	if (newFirst == NULL) {
		// everything is freed -- that practically this can't happen, if
		// the buffer is large enough to hold three max-sized entries
		sFirstEntry = sAfterLastEntry = sBuffer;
		TRACE(("free_first_entry(): all entries freed!\n"));
	} else
		sFirstEntry = newFirst;

	return true;
}


/*!	Makes sure we have needed * 4 bytes of memory at sAfterLastEntry.
	Returns \c false, if unable to free that much.
*/
static bool
make_space(size_t needed)
{
	// we need space for sAfterLastEntry, too (in case we need to wrap around
	// later)
	needed++;

	// If there's not enough space (free or occupied) after sAfterLastEntry,
	// we free all entries in that region and wrap around.
	if (sAfterLastEntry + needed > sBuffer + kBufferSize) {
		TRACE(("make_space(%lu), wrapping around: after last: %p\n", needed,
			sAfterLastEntry));

		// Free all entries after sAfterLastEntry and one more at the beginning
		// of the buffer.
		while (sFirstEntry > sAfterLastEntry) {
			if (!free_first_entry())
				return false;
		}
		if (sAfterLastEntry != sBuffer && !free_first_entry())
			return false;

		// just in case free_first_entry() freed the very last existing entry
		if (sAfterLastEntry == sBuffer)
			return true;

		// mark as wrap entry and actually wrap around
		trace_entry* wrapEntry = sAfterLastEntry;
		wrapEntry->size = 0;
		wrapEntry->flags = WRAP_ENTRY;
		sAfterLastEntry = sBuffer;
		sAfterLastEntry->previous_size = sBuffer + kBufferSize - wrapEntry;
	}

	if (sFirstEntry <= sAfterLastEntry) {
		// buffer is empty or the space after sAfterLastEntry is unoccupied
		return true;
	}

	// free the first entries, until there's enough space
	size_t space = sFirstEntry - sAfterLastEntry;

	if (space < needed) {
		TRACE(("make_space(%lu), left %ld\n", needed, space));
	}

	while (space < needed) {
		space += sFirstEntry->size;

		if (!free_first_entry())
			return false;
	}

	TRACE(("  out: start %p, entries %ld\n", sFirstEntry, sEntries));

	return true;
}


static trace_entry*
allocate_entry(size_t size, uint16 flags)
{
	if (sAfterLastEntry == NULL || size == 0 || size >= 65532)
		return NULL;

	InterruptsSpinLocker _(sLock);

	size = (size + 3) >> 2;
		// 4 byte aligned, don't store the lower 2 bits

	TRACE(("allocate_entry(%lu), start %p, end %p, buffer %p\n", size * 4,
		sFirstEntry, sAfterLastEntry, sBuffer));

	if (!make_space(size))
		return NULL;

	trace_entry* entry = sAfterLastEntry;
	entry->size = size;
	entry->flags = flags;
	sAfterLastEntry += size;
	sAfterLastEntry->previous_size = size;

	if (!(flags & BUFFER_ENTRY))
		sEntries++;

	TRACE(("  entry: %p, end %p, start %p, entries %ld\n", entry,
		sAfterLastEntry, sFirstEntry, sEntries));

	return entry;
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

	va_list args;
	va_start(args, format);
	fSize += vsnprintf(fBuffer + fSize, fCapacity - fSize, format, args);
	va_end(args);
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
	sWritten++;
#endif
}


void*
TraceEntry::operator new(size_t size, const std::nothrow_t&) throw()
{
#if ENABLE_TRACING
	trace_entry* entry = allocate_entry(size + sizeof(trace_entry), 0);
	return entry != NULL ? entry + 1 : NULL;
#endif
	return NULL;
}


//	#pragma mark -


AbstractTraceEntry::AbstractTraceEntry()
{
	struct thread* thread = thread_get_current_thread();
	if (thread != NULL) {
		fThread = thread->id;
		if (thread->team)
			fTeam = thread->team->id;
	}
	fTime = system_time();
}

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
		return (entry != NULL && entry->Thread() == fThread);
	}
};


class TeamTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* _entry, LazyTraceOutput& out)
	{
		const AbstractTraceEntry* entry
			= dynamic_cast<const AbstractTraceEntry*>(_entry);
		return (entry != NULL && entry->Team() == fTeam);
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
		fEntry = _NextNonBufferEntry(sFirstEntry);
		fIndex = 1;
	} else if (fEntry != NULL) {
		fEntry = _NextNonBufferEntry(next_entry(fEntry));
		fIndex++;
	}

	return Current();
}


TraceEntry*
TraceEntryIterator::Previous()
{
	if (fIndex == (int32)sEntries + 1)
		fEntry = sAfterLastEntry;

	if (fEntry != NULL) {
		fEntry = _PreviousNonBufferEntry(previous_entry(fEntry));
		fIndex--;
	}

	return Current();
}


TraceEntry*
TraceEntryIterator::MoveTo(int32 index)
{
	if (index == fIndex)
		return Current();

	if (index <= 0 || index > (int32)sEntries) {
		fIndex = (index <= 0 ? 0 : sEntries + 1);
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
	if ((int32)sEntries + 1 - fIndex < distance) {
		distance = sEntries + 1 - fIndex;
		direction = -1;
		fEntry = NULL;
		fIndex = sEntries + 1;
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
		entry = next_entry(entry);

	return entry;
}


trace_entry*
TraceEntryIterator::_PreviousNonBufferEntry(trace_entry* entry)
{
	while (entry != NULL && (entry->flags & BUFFER_ENTRY) != 0)
		entry = previous_entry(entry);

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
	static uint32 _previousWritten = 0;
	static uint32 _previousEntries = 0;
	static uint32 _previousOutputFlags = 0;
	static TraceEntryIterator iterator;


	// Note: start and index are Pascal-like indices (i.e. in [1, sEntries]).
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
		if (sWritten == 0 || sWritten != _previousWritten
			|| sEntries != _previousEntries) {
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
			maxToCheck = sEntries;

		// determine iteration direction
		direction = (start <= 0 || count < 0 ? -1 : 1);

		// validate count and maxToCheck
		if (count < 0)
			count = -count;
		if (maxToCheck < 0)
			maxToCheck = -maxToCheck;
		if (maxToCheck > (int32)sEntries)
			maxToCheck = sEntries;
		if (count > maxToCheck)
			count = maxToCheck;

		// validate start
		if (start <= 0 || start > (int32)sEntries)
			start = max_c(1, sEntries);
	}

	if (direction < 0) {
		firstToCheck = max_c(1, start - maxToCheck + 1);
		lastToCheck = start;
	} else {
		firstToCheck = start;
		lastToCheck = min_c((int32)sEntries, start + maxToCheck - 1);
	}

	// reset the iterator, if something changed in the meantime
	if (sWritten == 0 || sWritten != _previousWritten
		|| sEntries != _previousEntries) {
		iterator.Reset();
	}

	LazyTraceOutput out(sTraceOutputBuffer, kTraceOutputBufferSize,
		outputFlags);

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
		lastToCheck - firstToCheck + 1, sEntries, sWritten);

	// store iteration state
	_previousCount = count;
	_previousMaxToCheck = maxToCheck;
	_previousHasFilter = hasFilter;
	_previousPrintStackTrace = printStackTrace;
	_previousFirstChecked = firstToCheck;
	_previousLastChecked = lastToCheck;
	_previousDirection = direction;
	_previousWritten = sWritten;
	_previousEntries = sEntries;
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
	trace_entry* entry = allocate_entry(size + sizeof(trace_entry),
		BUFFER_ENTRY);
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
capture_tracing_stack_trace(int32 maxCount, int32 skipFrames, bool userOnly)
{
#if	ENABLE_TRACING
	// TODO: page_fault_exception() doesn't allow us to gracefully handle
	// a bad address in the stack trace, if interrupts are disabled.
	if (!are_interrupts_enabled())
		return NULL;

	tracing_stack_trace* stackTrace
		= (tracing_stack_trace*)alloc_tracing_buffer(
			sizeof(tracing_stack_trace) + maxCount * sizeof(addr_t));

	if (stackTrace != NULL) {
		stackTrace->depth = arch_debug_get_stack_trace(
			stackTrace->return_addresses, maxCount, 0, skipFrames + 1,
			userOnly);
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
	acquire_spinlock(&sLock);
#endif
}


void
unlock_tracing_buffer()
{
#if ENABLE_TRACING
	release_spinlock(&sLock);
#endif
}


extern "C" status_t
tracing_init(void)
{
#if	ENABLE_TRACING
	area_id area = create_area("tracing log", (void**)&sTraceOutputBuffer,
		B_ANY_KERNEL_ADDRESS, MAX_TRACE_SIZE + kTraceOutputBufferSize,
		B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	sBuffer = (trace_entry*)(sTraceOutputBuffer + kTraceOutputBufferSize);
	sFirstEntry = sBuffer;
	sAfterLastEntry = sBuffer;

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

