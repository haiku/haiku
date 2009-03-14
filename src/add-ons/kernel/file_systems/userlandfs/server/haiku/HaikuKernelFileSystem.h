// HaikuKernelFileSystem.h

#ifndef USERLAND_FS_HAIKU_KERNEL_FILE_SYSTEM_H
#define USERLAND_FS_HAIKU_KERNEL_FILE_SYSTEM_H

#include "Locker.h"

#include "../FileSystem.h"

struct file_system_module_info;

namespace UserlandFS {

struct HaikuKernelIORequest;

class HaikuKernelFileSystem : public FileSystem {
public:
								HaikuKernelFileSystem(
									file_system_module_info* fsModule);
	virtual						~HaikuKernelFileSystem();

			status_t			Init();

	virtual	status_t			CreateVolume(Volume** volume, dev_t id);
	virtual	status_t			DeleteVolume(Volume* volume);

			status_t			AddIORequest(HaikuKernelIORequest* request);
			HaikuKernelIORequest* GetIORequest(int32 requestID);
			void				PutIORequest(HaikuKernelIORequest* request,
									int32 refCount = 1);

private:
	class IORequestHashDefinition;
	class IORequestTable;

private:
			void				_InitCapabilities();

private:
			file_system_module_info* fFSModule;
			IORequestTable*		fIORequests;
			Locker				fLock;
};

}	// namespace UserlandFS

using UserlandFS::HaikuKernelFileSystem;

#endif	// USERLAND_FS_HAIKU_KERNEL_FILE_SYSTEM_H
