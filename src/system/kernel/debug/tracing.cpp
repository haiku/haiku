#include <tracing.h>

#include <util/AutoLock.h>

#include <stdlib.h>


#if ENABLE_TRACING

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

	int32 diff = sBufferStart - sBufferEnd;
	if (diff < 0)
		sBufferEnd = sBuffer;
	else
		needed -= diff;

	while (true) {
		uint16 freed = sBufferStart->size;
		sBufferStart = next_entry(sBufferStart);
		if (sBufferStart == NULL)
			sBufferStart = sBufferEnd;
		sEntries--;

		if (needed <= freed)
			break;

		needed -= freed;
	}
}


trace_entry*
allocate_entry(size_t size)
{
	if (sBuffer == NULL)
		return NULL;

	InterruptsSpinLocker _(sLock);

	size = (size + 3) & ~3;

	if (sBufferStart < sBufferEnd || sEntries == 0) {
		// the buffer ahead of us is still empty
		uint32 space = (sBuffer + kBufferSize - sBufferEnd) * 4;
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
	if (argc == 2)
		count = strtol(argv[1], NULL, 0);

	int32 start = sEntries - count;

	if (argc == 3) {
		start = strtol(argv[1], NULL, 0);
		count = strtol(argv[2], NULL, 0);
	} else if (argc > 3) {
		kprintf("usage: %s [start] [count] [#pattern]\n", argv[0]);
		return 0;
	}
	// TODO: add pattern matching mechanism for the class name

	if (start < 0)
		start = 0;

	int32 index = 0;

	for (trace_entry* current = sBufferStart; current != NULL;
			current = next_entry(current), index++) {
		if (index < start)
			continue;
		if (index > start + count)
			break;
		if ((entry->flags & BUFFER_ENTRY) != 0) {
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

			if (pattern != NULL)
				kprintf("%5ld. %s\n", index, buffer);
			else
				kprintf("%s\n", buffer);
		} else
			kprintf("%5ld. ** uninitialized entry **\n", index);
	}

	kprintf("%ld of %ld entries.\n", count, sEntries);
	return 0;
}


#endif	// ENABLE_TRACING

extern "C" uint8*
alloc_tracing_buffer(size_t size)
{
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
	if (area < B_OK) {
		panic("OH");
		return area;
	}

	sBufferStart = sBuffer;
	sBufferEnd = sBuffer;

	add_debugger_command("traced", &dump_tracing,
		"Dump recorded trace entries");
#endif	// ENABLE_TRACING
	return B_OK;
}

