/*
 * Copyright 2007-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "HaikuKernelFileSystem.h"

#include <string.h>

#include <new>

#include <fs_interface.h>

#include <AutoLocker.h>

#include <block_cache.h>
#include <condition_variable.h>
#include <file_cache.h>

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
	HaikuKernelIORequest*& GetLink(HaikuKernelIORequest* value) const
			{ return value->hashLink; }
};


// IORequestTable
struct HaikuKernelFileSystem::IORequestTable
	: public BOpenHashTable<IORequestHashDefinition> {
	typedef int32					KeyType;
	typedef	HaikuKernelIORequest	ValueType;

	size_t HashKey(int32 key) const
		{ return key; }
	size_t Hash(const HaikuKernelIORequest* value) const
		{ return value->id; }
	bool Compare(int32 key, const HaikuKernelIORequest* value) const
		{ return value->id == key; }
	HaikuKernelIORequest*& GetLink(HaikuKernelIORequest* value) const
			{ return value->hashLink; }
};


// NodeCapabilitiesHashDefinition
struct HaikuKernelFileSystem::NodeCapabilitiesHashDefinition {
	typedef fs_vnode_ops*					KeyType;
	typedef	HaikuKernelNode::Capabilities	ValueType;

	size_t HashKey(fs_vnode_ops* key) const
		{ return (size_t)(addr_t)key; }
	size_t Hash(const ValueType* value) const
		{ return HashKey(value->ops); }
	bool Compare(fs_vnode_ops* key, const ValueType* value) const
		{ return value->ops == key; }
	ValueType*& GetLink(ValueType* value) const
		{ return value->hashLink; }
};


// NodeCapabilitiesTable
struct HaikuKernelFileSystem::NodeCapabilitiesTable
	: public BOpenHashTable<NodeCapabilitiesHashDefinition> {
};


// constructor
HaikuKernelFileSystem::HaikuKernelFileSystem(const char* fsName,
	file_system_module_info* fsModule)
	:
	FileSystem(fsName),
	fFSModule(fsModule),
	fIORequests(NULL),
	fNodeCapabilities(NULL),
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
	delete fNodeCapabilities;

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

	// init file map
	error = file_map_init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// create I/O request map
	fIORequests = new(std::nothrow) IORequestTable;
	if (fIORequests == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	error = fIORequests->Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// create the node capabilites map
	fNodeCapabilities = new(std::nothrow) NodeCapabilitiesTable;
	if (fNodeCapabilities == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	error = fNodeCapabilities->Init();
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

	*_volume = volume;
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


// GetVNodeCapabilities
HaikuKernelNode::Capabilities*
HaikuKernelFileSystem::GetNodeCapabilities(fs_vnode_ops* ops)
{
	AutoLocker<Locker> locker(fLock);

	// check whether the ops are already known
	HaikuKernelNode::Capabilities* capabilities
		= fNodeCapabilities->Lookup(ops);
	if (capabilities != NULL) {
		capabilities->refCount++;
		return capabilities;
	}

	// get the capabilities implied by the ops vector
	FSVNodeCapabilities nodeCapabilities;
	_InitNodeCapabilities(ops, nodeCapabilities);

	// create a new object
	capabilities = new(std::nothrow) HaikuKernelNode::Capabilities(ops,
		nodeCapabilities);
	if (capabilities == NULL)
		return NULL;

	fNodeCapabilities->Insert(capabilities);

	return capabilities;
}


// PutVNodeCapabilities
void
HaikuKernelFileSystem::PutNodeCapabilities(
	HaikuKernelNode::Capabilities* capabilities)
{
	AutoLocker<Locker> locker(fLock);

	if (--capabilities->refCount == 0) {
		fNodeCapabilities->Remove(capabilities);
		delete capabilities;
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


/*static*/ void
HaikuKernelFileSystem::_InitNodeCapabilities(fs_vnode_ops* ops,
	FSVNodeCapabilities& capabilities)
{
	capabilities.ClearAll();

	// vnode operations
	capabilities.Set(FS_VNODE_CAPABILITY_LOOKUP, ops->lookup);
	capabilities.Set(FS_VNODE_CAPABILITY_GET_VNODE_NAME, ops->get_vnode_name);
	capabilities.Set(FS_VNODE_CAPABILITY_PUT_VNODE, ops->put_vnode);
	capabilities.Set(FS_VNODE_CAPABILITY_REMOVE_VNODE, ops->remove_vnode);

	// asynchronous I/O
	capabilities.Set(FS_VNODE_CAPABILITY_IO, ops->io);
	capabilities.Set(FS_VNODE_CAPABILITY_CANCEL_IO, ops->cancel_io);

	// cache file access
	capabilities.Set(FS_VNODE_CAPABILITY_GET_FILE_MAP, ops->get_file_map);

	// common operations
	capabilities.Set(FS_VNODE_CAPABILITY_IOCTL, ops->ioctl);
	capabilities.Set(FS_VNODE_CAPABILITY_SET_FLAGS, ops->set_flags);
	capabilities.Set(FS_VNODE_CAPABILITY_SELECT, ops->select);
	capabilities.Set(FS_VNODE_CAPABILITY_DESELECT, ops->deselect);
	capabilities.Set(FS_VNODE_CAPABILITY_FSYNC, ops->fsync);

	capabilities.Set(FS_VNODE_CAPABILITY_READ_SYMLINK, ops->read_symlink);
	capabilities.Set(FS_VNODE_CAPABILITY_CREATE_SYMLINK, ops->create_symlink);

	capabilities.Set(FS_VNODE_CAPABILITY_LINK, ops->link);
	capabilities.Set(FS_VNODE_CAPABILITY_UNLINK, ops->unlink);
	capabilities.Set(FS_VNODE_CAPABILITY_RENAME, ops->rename);

	capabilities.Set(FS_VNODE_CAPABILITY_ACCESS, ops->access);
	capabilities.Set(FS_VNODE_CAPABILITY_READ_STAT, ops->read_stat);
	capabilities.Set(FS_VNODE_CAPABILITY_WRITE_STAT, ops->write_stat);

	// file operations
	capabilities.Set(FS_VNODE_CAPABILITY_CREATE, ops->create);
	capabilities.Set(FS_VNODE_CAPABILITY_OPEN, ops->open);
	capabilities.Set(FS_VNODE_CAPABILITY_CLOSE, ops->close);
	capabilities.Set(FS_VNODE_CAPABILITY_FREE_COOKIE, ops->free_cookie);
	capabilities.Set(FS_VNODE_CAPABILITY_READ, ops->read);
	capabilities.Set(FS_VNODE_CAPABILITY_WRITE, ops->write);

	// directory operations
	capabilities.Set(FS_VNODE_CAPABILITY_CREATE_DIR, ops->create_dir);
	capabilities.Set(FS_VNODE_CAPABILITY_REMOVE_DIR, ops->remove_dir);
	capabilities.Set(FS_VNODE_CAPABILITY_OPEN_DIR, ops->open_dir);
	capabilities.Set(FS_VNODE_CAPABILITY_CLOSE_DIR, ops->close_dir);
	capabilities.Set(FS_VNODE_CAPABILITY_FREE_DIR_COOKIE, ops->free_dir_cookie);
	capabilities.Set(FS_VNODE_CAPABILITY_READ_DIR, ops->read_dir);
	capabilities.Set(FS_VNODE_CAPABILITY_REWIND_DIR, ops->rewind_dir);

	// attribute directory operations
	capabilities.Set(FS_VNODE_CAPABILITY_OPEN_ATTR_DIR, ops->open_attr_dir);
	capabilities.Set(FS_VNODE_CAPABILITY_CLOSE_ATTR_DIR, ops->close_attr_dir);
	capabilities.Set(FS_VNODE_CAPABILITY_FREE_ATTR_DIR_COOKIE,
		ops->free_attr_dir_cookie);
	capabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR_DIR, ops->read_attr_dir);
	capabilities.Set(FS_VNODE_CAPABILITY_REWIND_ATTR_DIR, ops->rewind_attr_dir);

	// attribute operations
	capabilities.Set(FS_VNODE_CAPABILITY_CREATE_ATTR, ops->create_attr);
	capabilities.Set(FS_VNODE_CAPABILITY_OPEN_ATTR, ops->open_attr);
	capabilities.Set(FS_VNODE_CAPABILITY_CLOSE_ATTR, ops->close_attr);
	capabilities.Set(FS_VNODE_CAPABILITY_FREE_ATTR_COOKIE,
		ops->free_attr_cookie);
	capabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR, ops->read_attr);
	capabilities.Set(FS_VNODE_CAPABILITY_WRITE_ATTR, ops->write_attr);

	capabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR_STAT, ops->read_attr_stat);
	capabilities.Set(FS_VNODE_CAPABILITY_WRITE_ATTR_STAT,
		ops->write_attr_stat);
	capabilities.Set(FS_VNODE_CAPABILITY_RENAME_ATTR, ops->rename_attr);
	capabilities.Set(FS_VNODE_CAPABILITY_REMOVE_ATTR, ops->remove_attr);

	// support for node and FS layers
	capabilities.Set(FS_VNODE_CAPABILITY_CREATE_SPECIAL_NODE,
		ops->create_special_node);
	capabilities.Set(FS_VNODE_CAPABILITY_GET_SUPER_VNODE, ops->get_super_vnode);
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
		= new(std::nothrow) HaikuKernelFileSystem(fsName, module);
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
