/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_FUSE_FILE_SYSTEM_H
#define USERLAND_FS_FUSE_FILE_SYSTEM_H

#include "../FileSystem.h"

#include "fuse_api.h"
#include "fuse_config.h"


namespace UserlandFS {

class FUSEFileSystem : public FileSystem {
public:
								FUSEFileSystem(const char* fsName,
									int (*mainFunction)(int,
										const char* const*));
	virtual						~FUSEFileSystem();

	static	FUSEFileSystem*		GetInstance();

			void				GetVolumeCapabilities(
									FSVolumeCapabilities& capabilities) const
									{ capabilities = fVolumeCapabilities; }
			void				GetNodeCapabilities(
									FSVNodeCapabilities& capabilities) const
									{ capabilities = fNodeCapabilities; }
			const FSVNodeCapabilities& GetNodeCapabilities() const
									{ return fNodeCapabilities; }

			fuse_fs*			GetFS() const	{ return fFS; }

			const fuse_config&	GetFUSEConfig() const	{ return fFUSEConfig; }

	virtual	status_t			CreateVolume(Volume** _volume, dev_t id);
	virtual	status_t			DeleteVolume(Volume* volume);

	virtual	void				InitRequestThreadContext(
									RequestThreadContext* context);

			status_t			InitClientFS(const char* parameters);
			void				ExitClientFS(status_t status);

			status_t			FinishInitClientFS(fuse_config* config,
									const fuse_operations* ops, size_t opSize,
									void* userData);
			status_t			MainLoop(bool multithreaded);

private:
			class ArgumentVector;

private:
	static	status_t			_InitializationThreadEntry(void* data);
			status_t			_InitializationThread();

			status_t			_InitClientFS(const fuse_operations* ops,
									size_t opSize, void* userData);

			void				_InitCapabilities();

private:
			int					(*fMainFunction)(int, const char* const*);
			thread_id			fInitThread;
			status_t			fInitStatus;
			status_t			fExitStatus;
			sem_id				fInitSemaphore;
			sem_id				fExitSemaphore;
			fuse_operations		fOps;
			const char*			fInitParameters;
			fuse_fs*			fFS;
			fuse_conn_info		fConnectionInfo;
			fuse_config			fFUSEConfig;

			FSVolumeCapabilities fVolumeCapabilities;
			FSVNodeCapabilities	fNodeCapabilities;
};

}	// namespace UserlandFS

using UserlandFS::FUSEFileSystem;


/*static*/ inline FUSEFileSystem*
FUSEFileSystem::GetInstance()
{
	return static_cast<FUSEFileSystem*>(sInstance);
}


#endif	// USERLAND_FS_FUSE_FILE_SYSTEM_H
