/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef VFS_TRACING_H
#define VFS_TRACING_H


#include <fs/fd.h>
#include <tracing.h>
#include <vfs.h>


// #pragma mark - File Descriptor Tracing


#if FILE_DESCRIPTOR_TRACING

namespace FileDescriptorTracing {


class FDTraceEntry : public AbstractTraceEntry {
public:
	FDTraceEntry(file_descriptor* descriptor)
		:
		fDescriptor(descriptor),
		fReferenceCount(descriptor->ref_count)
	{
#if FILE_DESCRIPTOR_TRACING_STACK_TRACE
		fStackTrace = capture_tracing_stack_trace(
			FILE_DESCRIPTOR_TRACING_STACK_TRACE, 0, false);
#endif
	}

#if FILE_DESCRIPTOR_TRACING_STACK_TRACE
	virtual void DumpStackTrace(TraceOutput& out)
	{
		out.PrintStackTrace(fStackTrace);
	}
#endif

protected:
	file_descriptor*		fDescriptor;
	int32					fReferenceCount;
#if FILE_DESCRIPTOR_TRACING_STACK_TRACE
	tracing_stack_trace*	fStackTrace;
#endif
};


class NewFD : public FDTraceEntry {
public:
	NewFD(io_context* context, int fd, file_descriptor* descriptor)
		:
		FDTraceEntry(descriptor),
		fContext(context),
		fFD(fd)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("fd new:     descriptor: %p (%" B_PRId32 "), context: %p, "
			"fd: %d", fDescriptor, fReferenceCount, fContext, fFD);
	}

private:
	io_context*	fContext;
	int			fFD;
};


class PutFD : public FDTraceEntry {
public:
	PutFD(file_descriptor* descriptor)
		:
		FDTraceEntry(descriptor)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("fd put:     descriptor: %p (%" B_PRId32 ")", fDescriptor,
			fReferenceCount);
	}
};


class GetFD : public FDTraceEntry {
public:
	GetFD(io_context* context, int fd, file_descriptor* descriptor)
		:
		FDTraceEntry(descriptor),
		fContext(context),
		fFD(fd)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("fd get:     descriptor: %p (%" B_PRId32 "), context: %p, "
			"fd: %d", fDescriptor, fReferenceCount, fContext, fFD);
	}

private:
	io_context*	fContext;
	int			fFD;
};


class RemoveFD : public FDTraceEntry {
public:
	RemoveFD(io_context* context, int fd, file_descriptor* descriptor)
		:
		FDTraceEntry(descriptor),
		fContext(context),
		fFD(fd)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("fd remove:  descriptor: %p (%" B_PRId32 "), context: %p, "
			"fd: %d", fDescriptor, fReferenceCount, fContext, fFD);
	}

private:
	io_context*	fContext;
	int			fFD;
};


class Dup2FD : public FDTraceEntry {
public:
	Dup2FD(io_context* context, int oldFD, int newFD)
		:
		FDTraceEntry(context->fds[oldFD]),
		fContext(context),
		fEvictedDescriptor(context->fds[newFD]),
		fEvictedReferenceCount(
			fEvictedDescriptor != NULL ? fEvictedDescriptor->ref_count : 0),
		fOldFD(oldFD),
		fNewFD(newFD)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("fd dup2:    descriptor: %p (%" B_PRId32 "), context: %p, "
			"fd: %d -> %d, evicted: %p (%" B_PRId32 ")", fDescriptor,
			fReferenceCount, fContext, fOldFD, fNewFD, fEvictedDescriptor,
			fEvictedReferenceCount);
	}

private:
	io_context*			fContext;
	file_descriptor*	fEvictedDescriptor;
	int32				fEvictedReferenceCount;
	int					fOldFD;
	int					fNewFD;
};


class InheritFD : public FDTraceEntry {
public:
	InheritFD(io_context* context, int fd, file_descriptor* descriptor,
		io_context* parentContext)
		:
		FDTraceEntry(descriptor),
		fContext(context),
		fParentContext(parentContext),
		fFD(fd)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("fd inherit: descriptor: %p (%" B_PRId32 "), context: %p, "
			"fd: %d, parentContext: %p", fDescriptor, fReferenceCount, fContext,
			fFD, fParentContext);
	}

private:
	io_context*	fContext;
	io_context*	fParentContext;
	int			fFD;
};


}	// namespace FileDescriptorTracing

#	define TFD(x)	new(std::nothrow) FileDescriptorTracing::x

#else
#	define TFD(x)
#endif	// FILE_DESCRIPTOR_TRACING


// #pragma mark - IO Context Tracing



#if IO_CONTEXT_TRACING

namespace IOContextTracing {


class IOContextTraceEntry : public AbstractTraceEntry {
public:
	IOContextTraceEntry(io_context* context)
		:
		fContext(context)
	{
#if IO_CONTEXT_TRACING_STACK_TRACE
		fStackTrace = capture_tracing_stack_trace(
			IO_CONTEXT_TRACING_STACK_TRACE, 0, false);
#endif
	}

#if IO_CONTEXT_TRACING_STACK_TRACE
	virtual void DumpStackTrace(TraceOutput& out)
	{
		out.PrintStackTrace(fStackTrace);
	}
#endif

protected:
	io_context*				fContext;
#if IO_CONTEXT_TRACING_STACK_TRACE
	tracing_stack_trace*	fStackTrace;
#endif
};


class NewIOContext : public IOContextTraceEntry {
public:
	NewIOContext(io_context* context, io_context* parentContext)
		:
		IOContextTraceEntry(context),
		fParentContext(parentContext)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("iocontext new:    context: %p, parent: %p", fContext,
			fParentContext);
	}

private:
	io_context*	fParentContext;
};


class FreeIOContext : public IOContextTraceEntry {
public:
	FreeIOContext(io_context* context)
		:
		IOContextTraceEntry(context)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("iocontext free:   context: %p", fContext);
	}
};


class ResizeIOContext : public IOContextTraceEntry {
public:
	ResizeIOContext(io_context* context, uint32 newTableSize)
		:
		IOContextTraceEntry(context),
		fOldTableSize(context->table_size),
		fNewTableSize(newTableSize)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("iocontext resize: context: %p, size: %" B_PRIu32	" -> %"
			B_PRIu32, fContext, fOldTableSize, fNewTableSize);
	}

private:
	uint32	fOldTableSize;
	uint32	fNewTableSize;
};


}	// namespace IOContextTracing

#	define TIOC(x)	new(std::nothrow) IOContextTracing::x

#else
#	define TIOC(x)
#endif	// IO_CONTEXT_TRACING


#endif // VFS_TRACING_H
