/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <tracing.h>

#include <stdlib.h>

#include <debug.h>
#include <util/AutoLock.h>


#if ENABLE_TRACING

//#define TRACE_TRACING
#ifdef TRACE_TRACING
#	define TRACE(x) dprintf x
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
static trace_entry* sBufferStart;
static trace_entry* sBufferEnd;
static uint32 sEntries;
static spinlock sLock;


static trace_entry*
next_entry(trace_entry* entry)
{
	entry += entry->size >> 2;
	if ((entry->flags & WRAP_ENTRY) != 0)
		entry = sBuffer;

	if (entry == sBufferEnd)
		return NULL;

	return entry;
}


static void
make_space(size_t needed)
{
	if (sBufferEnd + needed / 4 > sBuffer + kBufferSize - 1) {
		sBufferEnd->size = 0;
		sBufferEnd->flags = WRAP_ENTRY;
		sBufferEnd = sBuffer;
	}

	int32 space = (sBufferStart - sBufferEnd) * 4;
	TRACE(("make_space(%lu), left %ld\n", needed, space));
	if (space < 0)
		sBufferEnd = sBuffer;
	else if ((size_t)space < needed)
		needed -= space;
	else
		return;

	while (true) {
		// TODO: If the entry is not ENTRY_INITIALIZED yet, we must not
		// discard it, or the owner might overwrite memory we're allocating.
		trace_entry* removed = sBufferStart;
		uint16 freed = sBufferStart->size;
		TRACE(("  skip start %p, %u bytes\n", sBufferStart, freed));

		sBufferStart = next_entry(sBufferStart);
		if (sBufferStart == NULL)
			sBufferStart = sBufferEnd;

		if (!(removed->flags & BUFFER_ENTRY)) {
			((TraceEntry*)removed)->~TraceEntry();
			sEntries--;
		}

		if (needed <= freed)
			break;

		needed -= freed;
	}

	TRACE(("  out: start %p, entries %ld\n", sBufferStart, sEntries));
}


static trace_entry*
allocate_entry(size_t size, uint16 flags)
{
	if (sBuffer == NULL)
		return NULL;

	InterruptsSpinLocker _(sLock);

	size = (size + 3) & ~3;

	TRACE(("allocate_entry(%lu), start %p, end %p, buffer %p\n", size,
		sBufferStart, sBufferEnd, sBuffer));

	if (sBufferStart < sBufferEnd || sEntries == 0) {
		// the buffer ahead of us is still empty
		uint32 space = (sBuffer + kBufferSize - sBufferEnd) * 4;
		TRACE(("  free after end %p: %lu\n", sBufferEnd, space));
		if (space < size)
			make_space(size);
	} else {
		// we need to overwrite existing entries
		make_space(size);
	}

	trace_entry* entry = sBufferEnd;
	entry->size = size;
	entry->flags = flags;
	sBufferEnd += size >> 2;

	if (!(flags & BUFFER_ENTRY))
		sEntries++;

	TRACE(("  entry: %p, end %p, start %p, entries %ld\n", entry, sBufferEnd,
		sBufferStart, sEntries));

	return entry;
}


#endif	// ENABLE_TRACING


//	#pragma mark -


TraceEntry::TraceEntry()
{
}


TraceEntry::~TraceEntry()
{
}


void
TraceEntry::Dump(char* buffer, size_t bufferSize)
{
#if ENABLE_TRACING
	// to be overloaded by subclasses
	snprintf(buffer, bufferSize, "ENTRY %p", this);
#endif
}


void
TraceEntry::Initialized()
{
#if ENABLE_TRACING
	flags |= ENTRY_INITIALIZED;
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


#if ENABLE_TRACING


int
dump_tracing(int argc, char** argv)
{
	const char *pattern = NULL;
	if (argv[argc - 1][0] == '#')
		pattern = argv[--argc] + 1;

	// Note: start and index are Pascal-like indices (i.e. in [1, sEntries]).
	int32 count = 30;
	int32 start = 0;	// special index: print the last count entries
	int32 cont = 0;

	if (argc == 2) {
		if (strcmp(argv[1], "forward") == 0)
			cont = 1;
		else if (strcmp(argv[1], "backward") == 0)
			cont = -1;
		else
			start = parse_expression(argv[1]);
	}

	if (argc == 3) {
		start = parse_expression(argv[1]);
		count = parse_expression(argv[2]);
	} else if (argc > 3) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (cont != 0) {
		start = get_debug_variable("_tracingStart", start);
		count = get_debug_variable("_tracingCount", count);
		start = max_c(1, start + count * cont);
	}

	if ((uint32)count > sEntries)
		count = sEntries;
	if (start <= 0)
		start = max_c(1, sEntries - count + 1);
	if (uint32(start + count) > sEntries)
		count = sEntries - start + 1;

	int32 index = 1;
	int32 dumped = 0;

	for (trace_entry* current = sBufferStart; current != NULL;
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
			buffer[0] = '\0';
			((TraceEntry*)current)->Dump(buffer, sizeof(buffer));

			if (pattern != NULL && strstr(buffer, pattern) == NULL)
				continue;

			kprintf("%5ld. %s\n", index, buffer);
		} else
			kprintf("%5ld. ** uninitialized entry **\n", index);

		dumped++;
	}

	kprintf("entries %ld to %ld (%ld of %ld).\n", start, start + count - 1,
		dumped, sEntries);

	set_debug_variable("_tracingStart", start);
	set_debug_variable("_tracingCount", count);

	return cont != 0 ? B_KDEBUG_CONT : 0;
}


#endif	// ENABLE_TRACING


extern "C" uint8*
alloc_tracing_buffer(size_t size)
{
	if (size == 0)
		return NULL;

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
	if (maxSize == 0)
		return NULL;

	// there's no user_strnlen(), so always allocate the full buffer size
	// in this case
	if (!user)
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

	sBufferStart = sBuffer;
	sBufferEnd = sBuffer;

	add_debugger_command_etc("traced", &dump_tracing,
		"Dump recorded trace entries",
		"(\"forward\" | \"backward\") | ([ <start> [ <count> ] ] "
			"[ #<pattern> ])\n"
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
		"               printed.\n", 0);
#endif	// ENABLE_TRACING
	return B_OK;
}

