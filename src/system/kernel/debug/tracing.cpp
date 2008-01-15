/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <tracing.h>

#include <stdlib.h>

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
		TraceEntry* removed = (TraceEntry*)sBufferStart;
		uint16 freed = sBufferStart->size;
		TRACE(("  skip start %p, %u bytes\n", sBufferStart, freed));

		sBufferStart = next_entry(sBufferStart);
		if (sBufferStart == NULL)
			sBufferStart = sBufferEnd;

		sEntries--;
		removed->~TraceEntry();

		if (needed <= freed)
			break;

		needed -= freed;
	}

	TRACE(("  out: start %p, entries %ld\n", sBufferStart, sEntries));
}


trace_entry*
allocate_entry(size_t size)
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
	entry->flags = 0;
	sBufferEnd += size >> 2;
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
	return allocate_entry(size);
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

	int32 count = 30;
	int32 start = sEntries - count;

	if (argc == 2)
		start = strtol(argv[1], NULL, 0);

	if (argc == 3) {
		start = strtol(argv[1], NULL, 0);
		count = strtol(argv[2], NULL, 0);
	} else if (argc > 3) {
		kprintf("usage: %s [start] [count] [#pattern]\n", argv[0]);
		return 0;
	}

	if (start < 0)
		start = 0;
	if (uint32(start + count) > sEntries)
		count = sEntries - start;

	int32 index = 0;
	int32 dumped = 0;

	for (trace_entry* current = sBufferStart; current != NULL;
			current = next_entry(current), index++) {
		if (index < start)
			continue;
		if (index > start + count)
			break;
		if ((current->flags & BUFFER_ENTRY) != 0) {
			// skip buffer entries
			index--;
			continue;
		}

		if ((current->flags & ENTRY_INITIALIZED) != 0) {
			char buffer[256];
			buffer[0] = '\0';
			((TraceEntry*)current)->Dump(buffer, sizeof(buffer));

			if (pattern != NULL && strstr(buffer, pattern) == NULL)
				continue;

			dumped++;

			kprintf("%5ld. %s\n", index, buffer);
		} else
			kprintf("%5ld. ** uninitialized entry **\n", index);
	}

	kprintf("%ld entries of entries %ld to %ld (total %ld).\n", dumped,
		start + 1, start + count, sEntries);
	return 0;
}


#endif	// ENABLE_TRACING


extern "C" uint8*
alloc_tracing_buffer(size_t size)
{
	if (size == 0)
		return NULL;

#if	ENABLE_TRACING
	trace_entry* entry = allocate_entry(size + sizeof(trace_entry));
	if (entry == NULL)
		return NULL;

	entry->flags = BUFFER_ENTRY;
	return (uint8*)(entry + 1);
#else
	return NULL;
#endif
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

	add_debugger_command("traced", &dump_tracing,
		"Dump recorded trace entries");
#endif	// ENABLE_TRACING
	return B_OK;
}

