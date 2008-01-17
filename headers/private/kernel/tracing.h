#ifndef KERNEL_UTIL_TRACING_H
#define KERNEL_UTIL_TRACING_H


#include <SupportDefs.h>
#include <KernelExport.h>

#include <stdio.h>


#define ENABLE_TRACING 0
#define MAX_TRACE_SIZE 1024 * 1024

struct trace_entry {
	uint16  size;
	uint16  flags;
};

#ifdef __cplusplus

#include <new>

class TraceEntry : trace_entry {
	public:
		TraceEntry();
		virtual ~TraceEntry();

		virtual void Dump(char* buffer, size_t bufferSize);

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

		virtual void Dump(char* buffer, size_t bufferSize)
		{
			int length = snprintf(buffer, bufferSize, "[%6ld] %Ld: ",
				fThread, fTime);
			AddDump(buffer + length, bufferSize - length);
		}

		virtual void AddDump(char* buffer, size_t bufferSize)
		{
		}

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

#endif	/* KERNEL_UTIL_TRACING_H */
