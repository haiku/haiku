/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_TRACING_H
#define KERNEL_TRACING_H


#include <SupportDefs.h>
#include <KernelExport.h>

#include <stdio.h>

#include "tracing_config.h"


struct trace_entry {
	uint32	size			: 13;		// actual size is *4
	uint32	previous_size	: 13;		// actual size is *4
	uint32	flags			: 6;
};

struct tracing_stack_trace;

#ifdef __cplusplus

#include <new>


// trace output flags
#define TRACE_OUTPUT_TEAM_ID	0x01
	// print the team ID
#define TRACE_OUTPUT_DIFF_TIME	0x02
	// print the difference time to the previously printed entry instead of the
	// absolute time


class TraceOutput {
	public:
		TraceOutput(char* buffer, size_t bufferSize, uint32 flags);

		void Clear();
		void Print(const char* format,...)
			__attribute__ ((format (__printf__, 2, 3)));
		void PrintStackTrace(tracing_stack_trace* stackTrace);
		bool IsFull() const	{ return fSize >= fCapacity; }

		char* Buffer() const	{ return fBuffer; }
		size_t Capacity() const	{ return fCapacity; }
		size_t Size() const		{ return fSize; }

		uint32 Flags() const	{ return fFlags; }

		void SetLastEntryTime(bigtime_t time);
		bigtime_t LastEntryTime() const;

	private:
		char*		fBuffer;
		size_t		fCapacity;
		size_t		fSize;
		uint32		fFlags;
		bigtime_t	fLastEntryTime;
};


class TraceEntry {
	public:
		TraceEntry();
		virtual ~TraceEntry();

		virtual void Dump(TraceOutput& out);
		virtual void DumpStackTrace(TraceOutput& out);

		size_t Size() const		{ return ToTraceEntry()->size; }
		uint16 Flags() const	{ return ToTraceEntry()->flags; }

		void Initialized();

		void* operator new(size_t size, const std::nothrow_t&) throw();

		trace_entry* ToTraceEntry() const
		{
			return (trace_entry*)this - 1;
		}

		static TraceEntry* FromTraceEntry(trace_entry* entry)
		{
			return (TraceEntry*)(entry + 1);
		}
};


class AbstractTraceEntry : public TraceEntry {
public:
	AbstractTraceEntry()
	{
		_Init();
	}

	// dummy, ignores all arguments
	AbstractTraceEntry(size_t, size_t, bool)
	{
		_Init();
	}

	virtual ~AbstractTraceEntry();

	virtual void Dump(TraceOutput& out);

	virtual void AddDump(TraceOutput& out);

	thread_id ThreadID() const	{ return fThread; }
	thread_id TeamID() const	{ return fTeam; }
	bigtime_t Time() const		{ return fTime; }

protected:
	typedef AbstractTraceEntry TraceEntryBase;

private:
	void _Init();

protected:
	thread_id	fThread;
	team_id		fTeam;
	bigtime_t	fTime;
};


class AbstractTraceEntryWithStackTrace : public AbstractTraceEntry {
public:
	AbstractTraceEntryWithStackTrace(size_t stackTraceDepth,
		size_t skipFrames, bool kernelOnly);

	virtual void DumpStackTrace(TraceOutput& out);

protected:
	typedef AbstractTraceEntryWithStackTrace TraceEntryBase;

private:
	tracing_stack_trace* fStackTrace;
};


template<bool stackTraceDepth>
struct AbstractTraceEntrySelector {
	typedef AbstractTraceEntryWithStackTrace Type;
};


template<>
struct AbstractTraceEntrySelector<0> {
	typedef AbstractTraceEntry Type;
};


#define TRACE_ENTRY_SELECTOR(stackTraceDepth) \
	AbstractTraceEntrySelector<stackTraceDepth>::Type


class LazyTraceOutput : public TraceOutput {
public:
	LazyTraceOutput(char* buffer, size_t bufferSize, uint32 flags)
		: TraceOutput(buffer, bufferSize, flags)
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
	virtual ~TraceFilter();

	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out);

public:
	union {
		thread_id	fThread;
		team_id		fTeam;
		const char*	fString;
		uint64		fValue;
		struct {
			TraceFilter*	first;
			TraceFilter*	second;
		} fSubFilters;
	};
};


class WrapperTraceFilter : public TraceFilter {
public:
	virtual void Init(TraceFilter* filter, int direction, bool continued) = 0;
};


class TraceEntryIterator {
public:
	TraceEntryIterator()
		:
 		fEntry(NULL),
		fIndex(0)
	{
	}

	void Reset()
	{
		fEntry = NULL;
		fIndex = 0;
	}

	int32 Index() const
	{
		return fIndex;
	}

	TraceEntry* Current() const
	{
		return fEntry != NULL ? TraceEntry::FromTraceEntry(fEntry) : NULL;
	}

	TraceEntry* Next();
	TraceEntry* Previous();
	TraceEntry* MoveTo(int32 index);

private:
	trace_entry* _NextNonBufferEntry(trace_entry* entry);
	trace_entry* _PreviousNonBufferEntry(trace_entry* entry);

private:
	trace_entry*	fEntry;
	int32			fIndex;
};


int dump_tracing(int argc, char** argv, WrapperTraceFilter* wrapperFilter);

#endif	// __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

uint8* alloc_tracing_buffer(size_t size);
uint8* alloc_tracing_buffer_memcpy(const void* source, size_t size, bool user);
char* alloc_tracing_buffer_strcpy(const char* source, size_t maxSize,
			bool user);
struct tracing_stack_trace* capture_tracing_stack_trace(int32 maxCount,
			int32 skipFrames, bool kernelOnly);
void lock_tracing_buffer();
void unlock_tracing_buffer();
status_t tracing_init(void);

void _user_ktrace_output(const char *message);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_TRACING_H */
