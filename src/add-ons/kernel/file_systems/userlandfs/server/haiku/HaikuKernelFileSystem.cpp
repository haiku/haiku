// HaikuKernelFileSystem.cpp

#include "HaikuKernelFileSystem.h"

#include <string.h>

#include <new>

#include <fs_interface.h>

#include <AutoLocker.h>

#include <block_cache.h>
#include <condition_variable.h>

#include "HaikuKernelIORequest.h"
#include "HaikuKernelVolume.h"


// IORequestHashDefinition
struct HaikuKernelFileSystem::IORequestHashDefinition {
	typedef int32					KeyType;
	typedef	HaikuKernelIORequest	ValueType;

	size_t HashKey(int32 key) const
		{ return key; }
	size_t Hash(const HaikuKernelIORequest* value) const
		{ return value->id; }
	bool Compare(int32 key, const HaikuKernelIORequest* value) const
		{ return value->id == key; }
	HashTableLink<HaikuKernelIORequest>*
		GetLink(HaikuKernelIORequest* value) const
			{ return value; }
};


// IORequestTable
struct HaikuKernelFileSystem::IORequestTable
	: public OpenHashTable<IORequestHashDefinition> {
};


// constructor
HaikuKernelFileSystem::HaikuKernelFileSystem(file_system_module_info* fsModule)
	:
	FileSystem(),
	fFSModule(fsModule),
	fIORequests(NULL),
	fLock("HaikuKernelFileSystem")
{
	_InitCapabilities();
}


// destructor
HaikuKernelFileSystem::~HaikuKernelFileSystem()
{
	// call the kernel module uninitialization
	if (fFSModule->info.std_ops)
		fFSModule->info.std_ops(B_MODULE_UNINIT);

	delete fIORequests;

// TODO: Call the cleanup methods (condition vars, block cache)!
}


// Init
status_t
HaikuKernelFileSystem::Init()
{
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);

	// init condition variables
	condition_variable_init();
// TODO: Call the cleanup methods, if something goes wrong!

	// init block cache
	error = block_cache_init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// create I/O request map
	fIORequests = new(std::nothrow) IORequestTable;
	if (fIORequests == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	error = fIORequests->Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// call the kernel module initialization (if any)
	if (!fFSModule->info.std_ops)
		return B_OK;

	error = fFSModule->info.std_ops(B_MODULE_INIT);
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}


// CreateVolume
status_t
HaikuKernelFileSystem::CreateVolume(Volume** _volume, dev_t id)
{
	// check initialization and parameters
	if (!fFSModule || !_volume)
		return B_BAD_VALUE;

	// create and init the volume
	HaikuKernelVolume* volume
		= new(std::nothrow) HaikuKernelVolume(this, id, fFSModule);
	if (!volume)
		return B_NO_MEMORY;

	status_t error = volume->Init();
	if (error != B_OK) {
		delete volume;
		return error;
	}

	return B_OK;
}


// DeleteVolume
status_t
HaikuKernelFileSystem::DeleteVolume(Volume* volume)
{
	if (!volume || !dynamic_cast<HaikuKernelVolume*>(volume))
		return B_BAD_VALUE;
	delete volume;
	return B_OK;
}


// AddIORequest
status_t
HaikuKernelFileSystem::AddIORequest(HaikuKernelIORequest* request)
{
	AutoLocker<Locker> _(fLock);

	// check, if a request with that ID is already in the map
	if (fIORequests->Lookup(request->id) != NULL)
		RETURN_ERROR(B_BAD_VALUE);

	fIORequests->Insert(request);
	return B_OK;
}


// GetIORequest
HaikuKernelIORequest*
HaikuKernelFileSystem::GetIORequest(int32 requestID)
{
	AutoLocker<Locker> _(fLock);

	HaikuKernelIORequest* request = fIORequests->Lookup(requestID);
	if (request != NULL)
		request->refCount++;

	return request;
}


// PutIORequest
void
HaikuKernelFileSystem::PutIORequest(HaikuKernelIORequest* request,
	int32 refCount)
{
	AutoLocker<Locker> locker(fLock);

	if ((request->refCount -= refCount) <= 0) {
		fIORequests->Remove(request);
		locker.Unlock();
		delete request;
	}
}


// _InitCapabilities
void
HaikuKernelFileSystem::_InitCapabilities()
{
	fCapabilities.ClearAll();

	// FS interface type
	fClientFSType = CLIENT_FS_HAIKU_KERNEL;

	// FS operations
	fCapabilities.Set(FS_CAPABILITY_MOUNT, fFSModule->mount);
}


// #pragma mark - bootstrapping


status_t
userlandfs_create_file_system(const char* fsName, image_id image,
	FileSystem** _fileSystem)
{
	// get the modules
	module_info** modules;
	status_t error = get_image_symbol(image, "modules", B_SYMBOL_TYPE_DATA,
		(void**)&modules);
	if (error != B_OK)
		RETURN_ERROR(error);

	// module name must match "file_systems/<name>/v1"
	char moduleName[B_PATH_NAME_LENGTH];
	snprintf(moduleName, sizeof(moduleName), "file_systems/%s/v1", fsName);

	// find the module
	file_system_module_info* module = NULL;
	for (int32 i = 0; modules[i]->name; i++) {
		if (strcmp(modules[i]->name, moduleName) == 0) {
			module = (file_system_module_info*)modules[i];
			break;
		}
	}
	if (!module)
		RETURN_ERROR(B_ERROR);

	// create the file system
	HaikuKernelFileSystem* fileSystem
		= new(std::nothrow) HaikuKernelFileSystem(module);
	if (!fileSystem)
		RETURN_ERROR(B_NO_MEMORY);

	error = fileSystem->Init();
	if (error != B_OK) {
		delete fileSystem;
		return error;
	}

	*_fileSystem = fileSystem;
	return B_OK;
}
