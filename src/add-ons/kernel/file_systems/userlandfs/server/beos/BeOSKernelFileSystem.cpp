// BeOSKernelFileSystem.cpp

#include "BeOSKernelFileSystem.h"

#include <new>

#include "beos_kernel_emu.h"
#include "BeOSKernelVolume.h"
#include "fs_cache.h"
#include "fs_interface.h"


static const int32 kMaxBlockCacheBlocks = 16384;


// constructor
BeOSKernelFileSystem::BeOSKernelFileSystem(const char* fsName,
	beos_vnode_ops* fsOps)
	:
	FileSystem(fsName),
	fFSOps(fsOps),
	fBlockCacheInitialized(false)
{
	_InitCapabilities();
}


// destructor
BeOSKernelFileSystem::~BeOSKernelFileSystem()
{
	if (fBlockCacheInitialized)
		beos_shutdown_block_cache();
}


// Init
status_t
BeOSKernelFileSystem::Init()
{
	// init the block cache
	status_t error = beos_init_block_cache(kMaxBlockCacheBlocks, 0);
	if (error != B_OK)
		RETURN_ERROR(error);

	fBlockCacheInitialized = true;
	return B_OK;
}


// CreateVolume
status_t
BeOSKernelFileSystem::CreateVolume(Volume** volume, dev_t id)
{
	// check initialization and parameters
	if (!fFSOps || !volume)
		return B_BAD_VALUE;

	// create the volume
	*volume = new(std::nothrow) BeOSKernelVolume(this, id, fFSOps,
		fVolumeCapabilities);
	if (!*volume)
		return B_NO_MEMORY;
	return B_OK;
}

// DeleteVolume
status_t
BeOSKernelFileSystem::DeleteVolume(Volume* volume)
{
	if (!volume || !dynamic_cast<BeOSKernelVolume*>(volume))
		return B_BAD_VALUE;
	delete volume;
	return B_OK;
}

// _InitCapabilities
void
BeOSKernelFileSystem::_InitCapabilities()
{
	fCapabilities.ClearAll();
	fVolumeCapabilities.ClearAll();
	fNodeCapabilities.ClearAll();

	// FS interface type
	fClientFSType = CLIENT_FS_BEOS_KERNEL;

	// FS operations
	fCapabilities.Set(FS_CAPABILITY_MOUNT, fFSOps->mount);


	// Volume operations
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_UNMOUNT, fFSOps->unmount);

	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_READ_FS_INFO, fFSOps->rfsstat);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_WRITE_FS_INFO,
		fFSOps->wfsstat);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_SYNC, fFSOps->sync);

	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_GET_VNODE, fFSOps->read_vnode);

	// index directory & index operations
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_OPEN_INDEX_DIR,
		fFSOps->open_indexdir);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_CLOSE_INDEX_DIR,
		fFSOps->close_indexdir);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_FREE_INDEX_DIR_COOKIE,
		fFSOps->free_indexdircookie);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_READ_INDEX_DIR,
		fFSOps->read_indexdir);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_REWIND_INDEX_DIR,
		fFSOps->rewind_indexdir);

	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_CREATE_INDEX,
		fFSOps->create_index);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_REMOVE_INDEX,
		fFSOps->remove_index);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_READ_INDEX_STAT,
		fFSOps->stat_index);

	// query operations
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_OPEN_QUERY,
		fFSOps->open_query);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_CLOSE_QUERY,
		fFSOps->close_query);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_FREE_QUERY_COOKIE,
		fFSOps->free_querycookie);
	fVolumeCapabilities.Set(FS_VOLUME_CAPABILITY_READ_QUERY,
		fFSOps->read_query);
	// missing: FS_VOLUME_CAPABILITY_REWIND_QUERY,

	// vnode operations
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_LOOKUP, fFSOps->walk);
	// missing: FS_VNODE_CAPABILITY_GET_VNODE_NAME,

	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_PUT_VNODE, fFSOps->write_vnode);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REMOVE_VNODE,
		fFSOps->remove_vnode);

	// VM file access
	// missing: FS_VNODE_CAPABILITY_CAN_PAGE,
	// missing: FS_VNODE_CAPABILITY_READ_PAGES,
	// missing: FS_VNODE_CAPABILITY_WRITE_PAGES,

	// cache file access
	// missing: FS_VNODE_CAPABILITY_GET_FILE_MAP,

	// common operations
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_IOCTL, fFSOps->ioctl);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_SET_FLAGS, fFSOps->setflags);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_SELECT, fFSOps->select);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_DESELECT, fFSOps->deselect);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FSYNC, fFSOps->fsync);

	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_SYMLINK, fFSOps->readlink);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CREATE_SYMLINK, fFSOps->symlink);

	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_LINK, fFSOps->link);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_UNLINK, fFSOps->unlink);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_RENAME, fFSOps->rename);

	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_ACCESS, fFSOps->access);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_STAT, fFSOps->rstat);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_WRITE_STAT, fFSOps->wstat);

	// file operations
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CREATE, fFSOps->create);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_OPEN, fFSOps->open);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE, fFSOps->close);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FREE_COOKIE, fFSOps->free_cookie);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ, fFSOps->read);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_WRITE, fFSOps->write);

	// directory operations
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CREATE_DIR, fFSOps->mkdir);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REMOVE_DIR, fFSOps->rmdir);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_OPEN_DIR, fFSOps->opendir);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE_DIR, fFSOps->closedir);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FREE_DIR_COOKIE,
		fFSOps->free_dircookie);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_DIR, fFSOps->readdir);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REWIND_DIR, fFSOps->rewinddir);

	// attribute directory operations
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_OPEN_ATTR_DIR,
		fFSOps->open_attrdir);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE_ATTR_DIR,
		fFSOps->close_attrdir);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FREE_ATTR_DIR_COOKIE,
		fFSOps->free_attrdircookie);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR_DIR,
		fFSOps->read_attrdir);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REWIND_ATTR_DIR,
		fFSOps->rewind_attrdir);

	// attribute operations
	// we emulate open_attr() and free_attr_dir_cookie() if either read_attr()
	// or write_attr() is present
	bool hasAttributes = (fFSOps->read_attr || fFSOps->write_attr);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CREATE_ATTR, hasAttributes);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_OPEN_ATTR, hasAttributes);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE_ATTR, false);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_FREE_ATTR_COOKIE, hasAttributes);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR, fFSOps->read_attr);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_WRITE_ATTR, fFSOps->write_attr);

	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR_STAT,
		fFSOps->stat_attr);
	// missing: FS_VNODE_CAPABILITY_WRITE_ATTR_STAT
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_RENAME_ATTR, fFSOps->rename_attr);
	fNodeCapabilities.Set(FS_VNODE_CAPABILITY_REMOVE_ATTR, fFSOps->remove_attr);
}


// #pragma mark -


// get_beos_file_system_node_capabilities
//
// Service function for beos_kernel_emu.cpp. Declared in beos_kernel_emu.h.
void
get_beos_file_system_node_capabilities(FSVNodeCapabilities& capabilities)
{
	BeOSKernelFileSystem* fileSystem
		= static_cast<BeOSKernelFileSystem*>(FileSystem::GetInstance());
	fileSystem->GetNodeCapabilities(capabilities);
}


// #pragma mark - bootstrapping


status_t
userlandfs_create_file_system(const char* fsName, image_id image,
	FileSystem** _fileSystem)
{
	// get the symbols "fs_entry" and "api_version"
	beos_vnode_ops* fsOps;
	status_t error = get_image_symbol(image, "fs_entry", B_SYMBOL_TYPE_DATA,
		(void**)&fsOps);
	if (error != B_OK)
		RETURN_ERROR(error);
	int32* apiVersion;
	error = get_image_symbol(image, "api_version", B_SYMBOL_TYPE_DATA,
		(void**)&apiVersion);
	if (error != B_OK)
		RETURN_ERROR(error);

	// check api version
	if (*apiVersion != BEOS_FS_API_VERSION)
		RETURN_ERROR(B_ERROR);

	// create the file system
	BeOSKernelFileSystem* fileSystem
		= new(std::nothrow) BeOSKernelFileSystem(fsName, fsOps);
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
