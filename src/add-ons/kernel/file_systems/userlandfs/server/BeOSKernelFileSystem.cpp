// BeOSKernelFileSystem.cpp

#include "BeOSKernelFileSystem.h"

#include <new>

#include "beos_fs_interface.h"
#include "BeOSKernelVolume.h"

using std::nothrow;

// constructor
BeOSKernelFileSystem::BeOSKernelFileSystem(beos_vnode_ops* fsOps)
	: FileSystem(),
	  fFSOps(fsOps)
{
	_InitCapabilities();
}

// destructor
BeOSKernelFileSystem::~BeOSKernelFileSystem()
{
}

// CreateVolume
status_t
BeOSKernelFileSystem::CreateVolume(Volume** volume, mount_id id)
{
	// check initialization and parameters
	if (!fFSOps || !volume)
		return B_BAD_VALUE;

	// create the volume
	*volume = new(nothrow) BeOSKernelVolume(this, id, fFSOps);
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

	// FS interface type
	fCapabilities.clientFSType = CLIENT_FS_BEOS_KERNEL;

	// FS operations
	fCapabilities.Set(FS_CAPABILITY_MOUNT, fFSOps->mount);
	fCapabilities.Set(FS_CAPABILITY_UNMOUNT, fFSOps->unmount);

	fCapabilities.Set(FS_CAPABILITY_READ_FS_INFO, fFSOps->rfsstat);
	fCapabilities.Set(FS_CAPABILITY_WRITE_FS_INFO, fFSOps->wfsstat);
	fCapabilities.Set(FS_CAPABILITY_SYNC, fFSOps->sync);

	// vnode operations
	fCapabilities.Set(FS_CAPABILITY_LOOKUP, fFSOps->walk);
	// missing: FS_CAPABILITY_GET_VNODE_NAME,

	fCapabilities.Set(FS_CAPABILITY_GET_VNODE, fFSOps->read_vnode);
	fCapabilities.Set(FS_CAPABILITY_PUT_VNODE, fFSOps->write_vnode);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_VNODE, fFSOps->remove_vnode);

	// VM file access
	// missing: FS_CAPABILITY_CAN_PAGE,
	// missing: FS_CAPABILITY_READ_PAGES,
	// missing: FS_CAPABILITY_WRITE_PAGES,

	// cache file access
	// missing: FS_CAPABILITY_GET_FILE_MAP,

	// common operations
	fCapabilities.Set(FS_CAPABILITY_IOCTL, fFSOps->ioctl);
	fCapabilities.Set(FS_CAPABILITY_SET_FLAGS, fFSOps->setflags);
	fCapabilities.Set(FS_CAPABILITY_SELECT, fFSOps->select);
	fCapabilities.Set(FS_CAPABILITY_DESELECT, fFSOps->deselect);
	fCapabilities.Set(FS_CAPABILITY_FSYNC, fFSOps->fsync);

	fCapabilities.Set(FS_CAPABILITY_READ_SYMLINK, fFSOps->readlink);
	fCapabilities.Set(FS_CAPABILITY_CREATE_SYMLINK, fFSOps->symlink);

	fCapabilities.Set(FS_CAPABILITY_LINK, fFSOps->link);
	fCapabilities.Set(FS_CAPABILITY_UNLINK, fFSOps->unlink);
	fCapabilities.Set(FS_CAPABILITY_RENAME, fFSOps->rename);

	fCapabilities.Set(FS_CAPABILITY_ACCESS, fFSOps->access);
	fCapabilities.Set(FS_CAPABILITY_READ_STAT, fFSOps->rstat);
	fCapabilities.Set(FS_CAPABILITY_WRITE_STAT, fFSOps->wstat);

	// file operations
	fCapabilities.Set(FS_CAPABILITY_CREATE, fFSOps->create);
	fCapabilities.Set(FS_CAPABILITY_OPEN, fFSOps->open);
	fCapabilities.Set(FS_CAPABILITY_CLOSE, fFSOps->close);
	fCapabilities.Set(FS_CAPABILITY_FREE_COOKIE, fFSOps->free_cookie);
	fCapabilities.Set(FS_CAPABILITY_READ, fFSOps->read);
	fCapabilities.Set(FS_CAPABILITY_WRITE, fFSOps->write);

	// directory operations
	fCapabilities.Set(FS_CAPABILITY_CREATE_DIR, fFSOps->mkdir);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_DIR, fFSOps->rmdir);
	fCapabilities.Set(FS_CAPABILITY_OPEN_DIR, fFSOps->opendir);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_DIR, fFSOps->closedir);
	fCapabilities.Set(FS_CAPABILITY_FREE_DIR_COOKIE, fFSOps->free_dircookie);
	fCapabilities.Set(FS_CAPABILITY_READ_DIR, fFSOps->readdir);
	fCapabilities.Set(FS_CAPABILITY_REWIND_DIR, fFSOps->rewinddir);

	// attribute directory operations
	fCapabilities.Set(FS_CAPABILITY_OPEN_ATTR_DIR, fFSOps->open_attrdir);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_ATTR_DIR, fFSOps->close_attrdir);
	fCapabilities.Set(FS_CAPABILITY_FREE_ATTR_DIR_COOKIE,
		fFSOps->free_attrdircookie);
	fCapabilities.Set(FS_CAPABILITY_READ_ATTR_DIR, fFSOps->read_attrdir);
	fCapabilities.Set(FS_CAPABILITY_REWIND_ATTR_DIR, fFSOps->rewind_attrdir);

	// attribute operations
	// we emulate open_attr() and free_attr_dir_cookie() if either read_attr()
	// or write_attr() is present
	bool hasAttributes = (fFSOps->read_attr || fFSOps->write_attr);
	fCapabilities.Set(FS_CAPABILITY_CREATE_ATTR, hasAttributes);
	fCapabilities.Set(FS_CAPABILITY_OPEN_ATTR, hasAttributes);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_ATTR, false);
	fCapabilities.Set(FS_CAPABILITY_FREE_ATTR_COOKIE, hasAttributes);
	fCapabilities.Set(FS_CAPABILITY_READ_ATTR, fFSOps->read_attr);
	fCapabilities.Set(FS_CAPABILITY_WRITE_ATTR, fFSOps->write_attr);

	fCapabilities.Set(FS_CAPABILITY_READ_ATTR_STAT, fFSOps->stat_attr);
	// missing: FS_CAPABILITY_WRITE_ATTR_STAT
	fCapabilities.Set(FS_CAPABILITY_RENAME_ATTR, fFSOps->rename_attr);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_ATTR, fFSOps->remove_attr);

	// index directory & index operations
	fCapabilities.Set(FS_CAPABILITY_OPEN_INDEX_DIR, fFSOps->open_indexdir);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_INDEX_DIR, fFSOps->close_indexdir);
	fCapabilities.Set(FS_CAPABILITY_FREE_INDEX_DIR_COOKIE,
		fFSOps->free_indexdircookie);
	fCapabilities.Set(FS_CAPABILITY_READ_INDEX_DIR, fFSOps->read_indexdir);
	fCapabilities.Set(FS_CAPABILITY_REWIND_INDEX_DIR, fFSOps->rewind_indexdir);

	fCapabilities.Set(FS_CAPABILITY_CREATE_INDEX, fFSOps->create_index);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_INDEX, fFSOps->remove_index);
	fCapabilities.Set(FS_CAPABILITY_READ_INDEX_STAT, fFSOps->stat_index);

	// query operations
	fCapabilities.Set(FS_CAPABILITY_OPEN_QUERY, fFSOps->open_query);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_QUERY, fFSOps->close_query);
	fCapabilities.Set(FS_CAPABILITY_FREE_QUERY_COOKIE,
		fFSOps->free_querycookie);
	fCapabilities.Set(FS_CAPABILITY_READ_QUERY, fFSOps->read_query);
	// missing: FS_CAPABILITY_REWIND_QUERY,
}
