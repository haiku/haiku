#include <tracing.h>

#include <util/AutoLock.h>

#include <stdlib.h>


#if ENABLE_TRACING

enum {
	WRAP_ENTRY			= 0x01,
	ENTRY_INITIALIZED	= 0x02
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
make_space(size_t size)
{
	if (sBufferEnd + size / 4 > sBuffer + kBufferSize - 1) {
		sBufferEnd->size = 0;
		sBufferEnd->flags = WRAP_ENTRY;
		sBufferEnd = sBuffer;
	}

	int32 diff = sBufferStart - sBufferEnd;
	if (diff < 0)
		sBufferEnd = sBuffer;
	else
		size -= diff;

	while (true) {
		uint16 skip = sBufferStart->size;
		sBufferStart = next_entry(sBufferStart);
		if (sBufferStart == NULL)
			sBufferStart = sBuffer;
		sEntries--;

		if (size <= skip)
			break;

		size -= skip;
	}
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
TraceEntry::Dump()
{
#if ENABLE_TRACING
	// to be overloaded by subclasses
	kprintf("ENTRY %p\n", this);
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
TraceEntry::operator new(size_t size) throw()
{
#if ENABLE_TRACING
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
#else	// !ENABLE_TRACING
	return NULL;
#endif
}


//	#pragma mark -


#if ENABLE_TRACING


int
dump_tracing(int argc, char** argv)
{
	int32 count = 30;
	int32 start = sEntries - count;

	if (argc == 2) {
		count = strtol(argv[1], NULL, 0);
	} else if (argc == 3) {
		start = strtol(argv[1], NULL, 0);
		count = strtol(argv[2], NULL, 0);
	} else if (argc > 3) {
		kprintf("usage: %s [start] [count]\n", argv[0]);
		return 0;
	}

	if (start < 0)
		start = 0;

	int32 index = 0;

	for (trace_entry* current = sBufferStart; current != NULL;
			current = next_entry(current), index++) {
		if (index < start)
			continue;
		if (index > start + count)
			break;

		if ((current->flags & ENTRY_INITIALIZED) != 0)
			((TraceEntry*)current)->Dump();
		else
			kprintf("** uninitialized entry **\n");
	}

	return 0;
}


#endif	// ENABLE_TRACING


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

