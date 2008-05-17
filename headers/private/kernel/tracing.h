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
	uint32	size			: 14;		// actual size is *4
	uint32	previous_size	: 14;		// actual size is *4
	uint32	flags			: 4;
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
		void Print(const char* format,...);
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
		AbstractTraceEntry();
		virtual ~AbstractTraceEntry();

		virtual void Dump(TraceOutput& out);

		virtual void AddDump(TraceOutput& out);

		thread_id Thread() const { return fThread; }
		thread_id Team() const { return fTeam; }
		bigtime_t Time() const { return fTime; }

	protected:
		thread_id	fThread;
		team_id		fTeam;
		bigtime_t	fTime;
};

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

#endif	// __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

uint8* alloc_tracing_buffer(size_t size);
uint8* alloc_tracing_buffer_memcpy(const void* source, size_t size, bool user);
char* alloc_tracing_buffer_strcpy(const char* source, size_t maxSize,
			bool user);
tracing_stack_trace* capture_tracing_stack_trace(int32 maxCount,
			int32 skipFrames, bool userOnly);
int dump_tracing(int argc, char** argv, WrapperTraceFilter* wrapperFilter);
status_t tracing_init(void);

void _user_ktrace_output(const char *message);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_TRACING_H */
