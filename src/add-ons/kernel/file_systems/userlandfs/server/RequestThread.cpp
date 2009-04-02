/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "RequestThread.h"

#include <new>

#include <TLS.h>

#include "FileSystem.h"
#include "ServerDefs.h"
#include "UserlandRequestHandler.h"
#include "Volume.h"


static const int32 sTLSVariable = tls_allocate();


// constructor
RequestThreadContext::RequestThreadContext(Volume* volume,
	KernelRequest* request)
	:
	fPreviousContext(NULL),
	fThread(NULL),
	fVolume(volume),
	fRequest(request)
{
	fThread = RequestThread::GetCurrentThread();
	if (fThread) {
		fPreviousContext = fThread->GetContext();
		fThread->SetContext(this);
	}

	volume->GetFileSystem()->InitRequestThreadContext(this);
}

// destructor
RequestThreadContext::~RequestThreadContext()
{
	if (fThread)
		fThread->SetContext(fPreviousContext);
}

// GetThread
RequestThread*
RequestThreadContext::GetThread() const
{
	return fThread;
}

// GetVolume
UserlandFS::Volume*
RequestThreadContext::GetVolume() const
{
	return fVolume;
}


// RequestThread

// constructor
RequestThread::RequestThread()
	: fThread(-1),
	  fFileSystem(NULL),
	  fPort(NULL),
	  fContext(NULL),
	  fTerminating(false)
{
}

// destructor
RequestThread::~RequestThread()
{
	PrepareTermination();
	Terminate();
	delete fPort;
}

// Init
status_t
RequestThread::Init(FileSystem* fileSystem)
{
	if (!fileSystem)
		return B_BAD_VALUE;
	// create the port
	fPort = new(std::nothrow) RequestPort(kRequestPortSize);
	if (!fPort)
		return B_NO_MEMORY;
	status_t error = fPort->InitCheck();
	if (error != B_OK)
		return error;
	// spawn the thread
	fThread = spawn_thread(_ThreadEntry, "request thread", B_NORMAL_PRIORITY,
		this);
	if (fThread < 0)
		return fThread;
	fFileSystem = fileSystem;
	return B_OK;
}

// Run
void
RequestThread::Run()
{
	resume_thread(fThread);
}

// PrepareTermination
void
RequestThread::PrepareTermination()
{
	if (fTerminating)
		return;
	fTerminating = true;
	if (fPort)
		fPort->Close();
}

// Terminate
void
RequestThread::Terminate()
{
	if (fThread >= 0) {
		int32 result;
		wait_for_thread(fThread, &result);
		fThread = -1;
	}
}

// GetPortInfo
const Port::Info*
RequestThread::GetPortInfo() const
{
	return (fPort ? fPort->GetPortInfo() : NULL);
}

// GetFileSystem
UserlandFS::FileSystem*
RequestThread::GetFileSystem() const
{
	return fFileSystem;
}

// GetPort
RequestPort*
RequestThread::GetPort() const
{
	return fPort;
}

// GetContext
RequestThreadContext*
RequestThread::GetContext() const
{
	return fContext;
}

// GetCurrentThread
RequestThread*
RequestThread::GetCurrentThread()
{
	return (RequestThread*)tls_get(sTLSVariable);
}

// SetContext
void
RequestThread::SetContext(RequestThreadContext* context)
{
	fContext = context;
}

// _ThreadEntry
int32
RequestThread::_ThreadEntry(void* data)
{
	return ((RequestThread*)data)->_ThreadLoop();
}

// _ThreadLoop
int32
RequestThread::_ThreadLoop()
{
	tls_set(sTLSVariable, this);
	if (!fTerminating) {
		UserlandRequestHandler handler(fFileSystem, false);
		return fPort->HandleRequests(&handler);
	}
	return B_OK;
}

