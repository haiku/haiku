// RequestThread.h

#ifndef USERLAND_FS_REQUEST_THREAD_H
#define USERLAND_FS_REQUEST_THREAD_H

#include "RequestPort.h"

namespace UserlandFS {

class FileSystem;
class RequestThread;
class Volume;

// RequestThreadContext
class RequestThreadContext {
public:
								RequestThreadContext(Volume* volume);
								~RequestThreadContext();

			RequestThread*		GetThread() const;
			Volume*				GetVolume() const;

private:
			RequestThreadContext*	fPreviousContext;
			RequestThread*		fThread;
			Volume*				fVolume;
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
