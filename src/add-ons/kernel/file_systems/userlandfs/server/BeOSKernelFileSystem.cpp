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
	_InitCapabilities(fsOps);
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
BeOSKernelFileSystem::_InitCapabilities(beos_vnode_ops* fsOps)
{
	fCapabilities.ClearAll();

	// FS interface type
	fCapabilities.clientFSType = CLIENT_FS_BEOS_KERNEL;

	// FS operations
	fCapabilities.Set(FS_CAPABILITY_MOUNT, fsOps->mount);
	fCapabilities.Set(FS_CAPABILITY_UNMOUNT, fsOps->unmount);

	fCapabilities.Set(FS_CAPABILITY_READ_FS_INFO, fsOps->rfsstat);
	fCapabilities.Set(FS_CAPABILITY_WRITE_FS_INFO, fsOps->wfsstat);
	fCapabilities.Set(FS_CAPABILITY_SYNC, fsOps->sync);

	// vnode operations
	fCapabilities.Set(FS_CAPABILITY_LOOKUP, fsOps->walk);
	// missing: FS_CAPABILITY_GET_VNODE_NAME,

	fCapabilities.Set(FS_CAPABILITY_GET_VNODE, fsOps->read_vnode);
	fCapabilities.Set(FS_CAPABILITY_PUT_VNODE, fsOps->write_vnode);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_VNODE, fsOps->remove_vnode);

	// VM file access
	// missing: FS_CAPABILITY_CAN_PAGE,
	// missing: FS_CAPABILITY_READ_PAGES,
	// missing: FS_CAPABILITY_WRITE_PAGES,

	// cache file access
	// missing: FS_CAPABILITY_GET_FILE_MAP,

	// common operations
	fCapabilities.Set(FS_CAPABILITY_IOCTL, fsOps->ioctl);
	fCapabilities.Set(FS_CAPABILITY_SET_FLAGS, fsOps->setflags);
	fCapabilities.Set(FS_CAPABILITY_SELECT, fsOps->select);
	fCapabilities.Set(FS_CAPABILITY_DESELECT, fsOps->deselect);
	fCapabilities.Set(FS_CAPABILITY_FSYNC, fsOps->fsync);

	fCapabilities.Set(FS_CAPABILITY_READ_SYMLINK, fsOps->readlink);
	fCapabilities.Set(FS_CAPABILITY_CREATE_SYMLINK, fsOps->symlink);

	fCapabilities.Set(FS_CAPABILITY_LINK, fsOps->link);
	fCapabilities.Set(FS_CAPABILITY_UNLINK, fsOps->unlink);
	fCapabilities.Set(FS_CAPABILITY_RENAME, fsOps->rename);

	fCapabilities.Set(FS_CAPABILITY_ACCESS, fsOps->access);
	fCapabilities.Set(FS_CAPABILITY_READ_STAT, fsOps->rstat);
	fCapabilities.Set(FS_CAPABILITY_WRITE_STAT, fsOps->wstat);

	// file operations
	fCapabilities.Set(FS_CAPABILITY_CREATE, fsOps->create);
	fCapabilities.Set(FS_CAPABILITY_OPEN, fsOps->open);
	fCapabilities.Set(FS_CAPABILITY_CLOSE, fsOps->close);
	fCapabilities.Set(FS_CAPABILITY_FREE_COOKIE, fsOps->free_cookie);
	fCapabilities.Set(FS_CAPABILITY_READ, fsOps->read);
	fCapabilities.Set(FS_CAPABILITY_WRITE, fsOps->write);

	// directory operations
	fCapabilities.Set(FS_CAPABILITY_CREATE_DIR, fsOps->mkdir);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_DIR, fsOps->rmdir);
	fCapabilities.Set(FS_CAPABILITY_OPEN_DIR, fsOps->opendir);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_DIR, fsOps->closedir);
	fCapabilities.Set(FS_CAPABILITY_FREE_DIR_COOKIE, fsOps->free_dircookie);
	fCapabilities.Set(FS_CAPABILITY_READ_DIR, fsOps->readdir);
	fCapabilities.Set(FS_CAPABILITY_REWIND_DIR, fsOps->rewinddir);

	// attribute directory operations
	fCapabilities.Set(FS_CAPABILITY_OPEN_ATTR_DIR, fsOps->open_attrdir);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_ATTR_DIR, fsOps->close_attrdir);
	fCapabilities.Set(FS_CAPABILITY_FREE_ATTR_DIR_COOKIE,
		fsOps->free_attrdircookie);
	fCapabilities.Set(FS_CAPABILITY_READ_ATTR_DIR, fsOps->read_attrdir);
	fCapabilities.Set(FS_CAPABILITY_REWIND_ATTR_DIR, fsOps->rewind_attrdir);

	// attribute operations
	// we emulate open_attr() and free_attr_dir_cookie() if either read_attr()
	// or write_attr() is present
	bool hasAttributes = (fsOps->read_attr || fsOps->write_attr);
	fCapabilities.Set(FS_CAPABILITY_CREATE_ATTR, hasAttributes);
	fCapabilities.Set(FS_CAPABILITY_OPEN_ATTR, hasAttributes);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_ATTR, false);
	fCapabilities.Set(FS_CAPABILITY_FREE_ATTR_COOKIE, hasAttributes);
	fCapabilities.Set(FS_CAPABILITY_READ_ATTR, fsOps->read_attr);
	fCapabilities.Set(FS_CAPABILITY_WRITE_ATTR, fsOps->write_attr);

	fCapabilities.Set(FS_CAPABILITY_READ_ATTR_STAT, fsOps->stat_attr);
	// missing: FS_CAPABILITY_WRITE_ATTR_STAT
	fCapabilities.Set(FS_CAPABILITY_RENAME_ATTR, fsOps->rename_attr);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_ATTR, fsOps->remove_attr);

	// index directory & index operations
	fCapabilities.Set(FS_CAPABILITY_OPEN_INDEX_DIR, fsOps->open_indexdir);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_INDEX_DIR, fsOps->close_indexdir);
	fCapabilities.Set(FS_CAPABILITY_FREE_INDEX_DIR_COOKIE,
		fsOps->free_indexdircookie);
	fCapabilities.Set(FS_CAPABILITY_READ_INDEX_DIR, fsOps->read_indexdir);
	fCapabilities.Set(FS_CAPABILITY_REWIND_INDEX_DIR, fsOps->rewind_indexdir);

	fCapabilities.Set(FS_CAPABILITY_CREATE_INDEX, fsOps->create_index);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_INDEX, fsOps->remove_index);
	fCapabilities.Set(FS_CAPABILITY_READ_INDEX_STAT, fsOps->stat_index);

	// query operations
	fCapabilities.Set(FS_CAPABILITY_OPEN_QUERY, fsOps->open_query);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_QUERY, fsOps->close_query);
	fCapabilities.Set(FS_CAPABILITY_FREE_QUERY_COOKIE, fsOps->free_querycookie);
	fCapabilities.Set(FS_CAPABILITY_READ_QUERY, fsOps->read_query);
	// missing: FS_CAPABILITY_REWIND_QUERY,
}
