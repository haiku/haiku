/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_REQUEST_THREAD_H
#define USERLAND_FS_REQUEST_THREAD_H

#include "RequestPort.h"

namespace UserlandFS {

class FileSystem;
class RequestThread;
class Volume;

#define REQUEST_THREAD_CONTEXT_FS_DATA_SIZE	256

// RequestThreadContext
class RequestThreadContext {
public:
								RequestThreadContext(Volume* volume,
									KernelRequest* request);
								~RequestThreadContext();

			RequestThread*		GetThread() const;
			Volume*				GetVolume() const;
			KernelRequest*		GetRequest() const	{ return fRequest; }
			void*				GetFSData() 		{ return fFSData; }

private:
			RequestThreadContext*	fPreviousContext;
			RequestThread*		fThread;
			Volume*				fVolume;
			KernelRequest*		fRequest;
			uint8				fFSData[REQUEST_THREAD_CONTEXT_FS_DATA_SIZE];
};

// RequestThread
class RequestThread {
public:
								RequestThread();
								~RequestThread();

			status_t			Init(FileSystem* fileSystem);
			void				Run();
			void				PrepareTermination();
			void				Terminate();

			const Port::Info*	GetPortInfo() const;
			FileSystem*			GetFileSystem() const;
			RequestPort*		GetPort() const;
			RequestThreadContext*	GetContext() const;

	static	RequestThread*		GetCurrentThread();

private:
			void				SetContext(RequestThreadContext* context);

private:
	static	int32				_ThreadEntry(void* data);
			int32				_ThreadLoop();

private:
			friend class RequestThreadContext;

			thread_id			fThread;
			FileSystem*			fFileSystem;
			RequestPort*		fPort;
			RequestThreadContext* fContext;
			bool				fTerminating;
};

}	// namespace UserlandFS

using UserlandFS::RequestThreadContext;
using UserlandFS::RequestThread;

#endif	// USERLAND_FS_REQUEST_THREAD_H
