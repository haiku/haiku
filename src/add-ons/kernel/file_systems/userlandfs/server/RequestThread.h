// RequestThread.h

#ifndef USERLAND_FS_REQUEST_THREAD_H
#define USERLAND_FS_REQUEST_THREAD_H

#include "RequestPort.h"

namespace UserlandFS {

class RequestThread;
class UserFileSystem;
class UserVolume;

// RequestThreadContext
class RequestThreadContext {
public:
								RequestThreadContext(UserVolume* volume);
								~RequestThreadContext();

			RequestThread*		GetThread() const;
			UserVolume*			GetVolume() const;

private:
			RequestThreadContext*	fPreviousContext;
			RequestThread*		fThread;
			UserVolume*			fVolume;
};

// RequestThread
class RequestThread {
public:
								RequestThread();
								~RequestThread();

			status_t			Init(UserFileSystem* fileSystem);
			void				Run();
			void				PrepareTermination();
			void				Terminate();

			const Port::Info*	GetPortInfo() const;
			UserFileSystem*		GetFileSystem() const;
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
			UserFileSystem*		fFileSystem;
			RequestPort*		fPort;
			RequestThreadContext* fContext;
			bool				fTerminating;
};

}	// namespace UserlandFS

using UserlandFS::RequestThreadContext;
using UserlandFS::RequestThread;

#endif	// USERLAND_FS_REQUEST_THREAD_H
