#ifndef KERNEL_TRACING_H
#define KERNEL_TRACING_H


#include <SupportDefs.h>
#include <KernelExport.h>

#include <stdio.h>

#include <tracing_config.h>


struct trace_entry {
	uint32	size			: 14;		// actual size is *4
	uint32	previous_size	: 14;		// actual size is *4
	uint32	flags			: 4;
};

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

class TraceEntry : public trace_entry {
	public:
		TraceEntry();
		virtual ~TraceEntry();

		virtual void Dump(TraceOutput& out);

		size_t Size() const { return size; }
		uint16 Flags() const { return flags; }

		void Initialized();

		void* operator new(size_t size, const std::nothrow_t&) throw();
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

#endif	// __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

uint8* alloc_tracing_buffer(size_t size);
uint8* alloc_tracing_buffer_memcpy(const void* source, size_t size, bool user);
char* alloc_tracing_buffer_strcpy(const char* source, size_t maxSize,
			bool user);
status_t tracing_init(void);

void _user_ktrace_output(const char *message);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_TRACING_H */
