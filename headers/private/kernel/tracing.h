#ifndef KERNEL_TRACING_H
#define KERNEL_TRACING_H


#include <SupportDefs.h>
#include <KernelExport.h>

#include <stdio.h>

#include <tracing_config.h>


struct trace_entry {
	uint16  size;
	uint16  flags;
};

#ifdef __cplusplus

#include <new>

class TraceOutput {
	public:
		TraceOutput(char* buffer, size_t bufferSize);

		void Clear();
		void Print(const char* format,...);
		bool IsFull() const	{ return fSize >= fCapacity; }

		char* Buffer() const	{ return fBuffer; }
		size_t Capacity() const	{ return fCapacity; }
		size_t Size() const		{ return fSize; }

	private:
		char*	fBuffer;
		size_t	fCapacity;
		size_t	fSize;
};

class TraceEntry : trace_entry {
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
		AbstractTraceEntry()
			:
			fThread(find_thread(NULL)),
			fTime(system_time())
		{
		}

		virtual ~AbstractTraceEntry();

		virtual void Dump(TraceOutput& out);

		virtual void AddDump(TraceOutput& out);

		thread_id Thread() const { return fThread; }
		bigtime_t Time() const { return fTime; }

	protected:
		thread_id	fThread;
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

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_TRACING_H */
