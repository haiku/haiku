/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <tracing.h>

#include <stdarg.h>
#include <stdlib.h>

#include <debug.h>
#include <kernel.h>
#include <util/AutoLock.h>


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
	BUFFER_ENTRY		= 0x04
};

static const size_t kBufferSize = MAX_TRACE_SIZE / 4 - 1;

static trace_entry* sBuffer;
static trace_entry* sFirstEntry;
static trace_entry* sAfterLastEntry;
static uint32 sEntries;
static uint32 sWritten;
static spinlock sLock;


static trace_entry*
next_entry(trace_entry* entry)
{
	entry += entry->size >> 2;
	if ((entry->flags & WRAP_ENTRY) != 0)
		entry = sBuffer;

	if (entry == sAfterLastEntry)
		return NULL;

	return entry;
}


static bool
free_first_entry()
{
	TRACE(("  skip start %p, %u bytes\n", sFirstEntry, sFirstEntry->size));

	trace_entry* newFirst = next_entry(sFirstEntry);

	if (sFirstEntry->flags & BUFFER_ENTRY) {
		// a buffer entry -- just skip it
	} else if (sFirstEntry->flags & ENTRY_INITIALIZED) {
		// fully initialized TraceEntry -- destroy it
		((TraceEntry*)sFirstEntry)->~TraceEntry();
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


static bool
make_space(size_t needed)
{
	// we need space for sAfterLastEntry, too (in case we need to wrap around
	// later)
	needed += 4;

	// If there's not enough space (free or occupied) after sAfterLastEntry,
	// we free all entries in that region and wrap around.
	if (sAfterLastEntry + needed / 4 > sBuffer + kBufferSize) {
		TRACE(("make_space(%lu), wrapping around: after last: %p\n", needed,
			sAfterLastEntry));

		// Free all entries after sAfterLastEntry and one more at the beginning
		// of the buffer.
		do {
			if (!free_first_entry())
				return false;
		} while (sFirstEntry > sAfterLastEntry);

		// just in case free_first_entry() freed the very last existing entry
		if (sAfterLastEntry == sBuffer)
			return true;

		// mark as wrap entry and actually wrap around
		sAfterLastEntry->size = 0;
		sAfterLastEntry->flags = WRAP_ENTRY;
		sAfterLastEntry = sBuffer;
	}

	if (sFirstEntry <= sAfterLastEntry) {
		// buffer is empty or the space after sAfterLastEntry is unoccupied
		return true;
	}

	// free the first entries, until there's enough space
	size_t space = (sFirstEntry - sAfterLastEntry) * 4;

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
	if (sBuffer == NULL || size == 0 || size >= 65532)
		return NULL;

	InterruptsSpinLocker _(sLock);

	size = (size + 3) & ~3;

	TRACE(("allocate_entry(%lu), start %p, end %p, buffer %p\n", size,
		sFirstEntry, sAfterLastEntry, sBuffer));

	if (!make_space(size))
		return NULL;

	trace_entry* entry = sAfterLastEntry;
	entry->size = size;
	entry->flags = flags;
	sAfterLastEntry += size >> 2;

	if (!(flags & BUFFER_ENTRY))
		sEntries++;

	TRACE(("  entry: %p, end %p, start %p, entries %ld\n", entry,
		sAfterLastEntry, sFirstEntry, sEntries));

	return entry;
}


#endif	// ENABLE_TRACING


// #pragma mark -


TraceOutput::TraceOutput(char* buffer, size_t bufferSize)
	: fBuffer(buffer),
	  fCapacity(bufferSize)
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
	if (IsFull())
		return;

	va_list args;
	va_start(args, format);
	fSize += vsnprintf(fBuffer + fSize, fCapacity - fSize, format, args);
	va_end(args);
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
TraceEntry::Initialized()
{
#if ENABLE_TRACING
	flags |= ENTRY_INITIALIZED;
	sWritten++;
#endif
}


void*
TraceEntry::operator new(size_t size, const std::nothrow_t&) throw()
{
#if ENABLE_TRACING
	return allocate_entry(size, 0);
#else
	return NULL;
#endif
}


//	#pragma mark -


AbstractTraceEntry::~AbstractTraceEntry()
{
}


void
AbstractTraceEntry::Dump(TraceOutput& out)
{
	out.Print("[%6ld] %Ld: ", fThread, fTime);
	AddDump(out);
}


void
AbstractTraceEntry::AddDump(TraceOutput& out)
{
}


//	#pragma mark - trace filters


class LazyTraceOutput : public TraceOutput {
public:
	LazyTraceOutput(char* buffer, size_t bufferSize)
		: TraceOutput(buffer, bufferSize)
	{
	}

	const char* DumpEntry(const TraceEntry* entry)
	{
		if (Size() == 0) {
			const_cast<TraceEntry*>(entry)->Dump(*this);
				// Dump() should probably be const
		}

		return Buffer();
	}
};


class TraceFilter {
public:
	virtual ~TraceFilter()
	{
	}

	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return false;
	}

public:
	union {
		thread_id	fThread;
		const char*	fString;
		struct {
			TraceFilter*	first;
			TraceFilter*	second;
		} fSubFilters;
	};
};


class ThreadTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* _entry, LazyTraceOutput& out)
	{
		const AbstractTraceEntry* entry
			= dynamic_cast<const AbstractTraceEntry*>(_entry);
		return (entry != NULL && entry->Thread() == fThread);
	}
};


class PatternTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return strstr(out.DumpEntry(entry), fString) != NULL;
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

	bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return fFilters[0].Filter(entry, out);
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


int
dump_tracing(int argc, char** argv)
{
	int argi = 1;

	// Note: start and index are Pascal-like indices (i.e. in [1, sEntries]).
	int32 count = 30;
	int32 start = 0;	// special index: print the last count entries
	int32 cont = 0;

	bool hasFilter = false;

	if (argi < argc) {
		if (strcmp(argv[argi], "forward") == 0) {
			cont = 1;
			argi++;
		} else if (strcmp(argv[argi], "backward") == 0) {
			cont = -1;
			argi++;
		}
	}

	if (cont != 0 && argi < argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	// start
	if (argi < argc) {
		if (strcmp(argv[argi], "filter") == 0) {
			hasFilter = true;
			argi++;
		} else if (argv[argi][0] == '#') {
			hasFilter = true;
		} else {
			start = parse_expression(argv[argi]);
			argi++;
		}
	}

	// count
	if (!hasFilter && argi < argc) {
		if (strcmp(argv[argi], "filter") == 0) {
			hasFilter = true;
			argi++;
		} else if (argv[argi][0] == '#') {
			hasFilter = true;
		} else {
			count = parse_expression(argv[argi]);
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

	if (cont != 0) {
		start = get_debug_variable("_tracingStart", start);
		count = get_debug_variable("_tracingCount", count);
		start = max_c(1, start + count * cont);
		hasFilter = get_debug_variable("_tracingFilter", count);
	}

	if ((uint32)count > sEntries)
		count = sEntries;
	if (start <= 0)
		start = max_c(1, sEntries - count + 1);
	if (uint32(start + count) > sEntries)
		count = sEntries - start + 1;

	int32 index = 1;
	int32 dumped = 0;

	for (trace_entry* current = sFirstEntry; current != NULL;
			current = next_entry(current), index++) {
		if ((current->flags & BUFFER_ENTRY) != 0) {
			// skip buffer entries
			index--;
			continue;
		}
		if (index < start)
			continue;
		if (index >= start + count)
			break;

		if ((current->flags & ENTRY_INITIALIZED) != 0) {
			char buffer[256];
			LazyTraceOutput out(buffer, sizeof(buffer));
			
			TraceEntry* entry = (TraceEntry*)current;
			if (hasFilter && !TraceFilterParser::Default()->Filter(entry, out))
				continue;

			kprintf("%5ld. %s\n", index, out.DumpEntry(entry));
		} else
			kprintf("%5ld. ** uninitialized entry **\n", index);

		dumped++;
	}

	kprintf("entries %ld to %ld (%ld of %ld). %ld entries written\n", start,
		start + count - 1, dumped, sEntries, sWritten);

	set_debug_variable("_tracingStart", start);
	set_debug_variable("_tracingCount", count);
	set_debug_variable("_tracingFilter", hasFilter);

	return cont != 0 ? B_KDEBUG_CONT : 0;
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


extern "C" status_t
tracing_init(void)
{
#if	ENABLE_TRACING
	area_id area = create_area("tracing log", (void**)&sBuffer,
		B_ANY_KERNEL_ADDRESS, MAX_TRACE_SIZE, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	sFirstEntry = sBuffer;
	sAfterLastEntry = sBuffer;

	add_debugger_command_etc("traced", &dump_tracing,
		"Dump recorded trace entries",
		"(\"forward\" | \"backward\") | ([ <start> [ <count> ] ] "
			"[ #<pattern> | (\"filter\" <filter>) ])\n"
		"Prints recorded trace entries. If \"backward\" or \"forward\" is\n"
		"specified, the command continues where the previous invocation left\n"
		"off, i.e. printing the previous respectively next entries (as many\n"
		"as printed before). In this case the command is continuable, that is\n"
		"afterwards entering an empty line in the debugger will reinvoke it.\n"
		"  <start>    - The index of the first entry to print. The index of\n"
		"               the first recorded entry is 1. If 0 is specified, the\n"
		"               last <count> recorded entries are printed. Defaults \n"
		"               to 0.\n"
		"  <count>    - The number of entries to be printed. Defaults to 30.\n"
		"  <pattern>  - If specified only entries containing this string are\n"
		"               printed.\n"
		"  <filter>   - If specified only entries matching this filter\n"
		"               expression are printed. The expression can consist of\n"
		"               prefix operators \"not\", \"and\", \"or\", filters of\n"
		"               the kind \"'thread' <thread>\" (matching entries\n"
		"               with the given thread ID), or filter of the kind\n"
		"               \"#<pattern>\" (matching entries containing the given\n"
		"               string.\n", 0);
#endif	// ENABLE_TRACING
	return B_OK;
}

