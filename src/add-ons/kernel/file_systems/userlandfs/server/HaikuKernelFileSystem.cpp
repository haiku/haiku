// HaikuKernelFileSystem.cpp

#include "HaikuKernelFileSystem.h"

#include <new>

#include <fs_interface.h>

#include "HaikuKernelVolume.h"

using std::nothrow;

// constructor
HaikuKernelFileSystem::HaikuKernelFileSystem(file_system_module_info* fsModule)
	: FileSystem(),
	  fFSModule(fsModule)
{
	_InitCapabilities();
}

// destructor
HaikuKernelFileSystem::~HaikuKernelFileSystem()
{
	// call the kernel module uninitialization
	if (fFSModule->info.std_ops)
		fFSModule->info.std_ops(B_MODULE_UNINIT);
}

// Init
status_t
HaikuKernelFileSystem::Init()
{
	// call the kernel module initialization
	if (!fFSModule->info.std_ops)
		return B_OK;
	return fFSModule->info.std_ops(B_MODULE_INIT);
}

// CreateVolume
status_t
HaikuKernelFileSystem::CreateVolume(Volume** volume, mount_id id)
{
	// check initialization and parameters
	if (!fFSModule || !volume)
		return B_BAD_VALUE;

	// create the volume
	*volume = new(nothrow) HaikuKernelVolume(this, id, fFSModule);
	if (!*volume)
		return B_NO_MEMORY;
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

// _InitCapabilities
void
HaikuKernelFileSystem::_InitCapabilities()
{
	fCapabilities.ClearAll();

	// FS interface type
	fCapabilities.clientFSType = CLIENT_FS_HAIKU_KERNEL;

	// FS operations
	fCapabilities.Set(FS_CAPABILITY_MOUNT, fFSModule->mount);
	fCapabilities.Set(FS_CAPABILITY_UNMOUNT, fFSModule->unmount);

	fCapabilities.Set(FS_CAPABILITY_READ_FS_INFO, fFSModule->read_fs_info);
	fCapabilities.Set(FS_CAPABILITY_WRITE_FS_INFO, fFSModule->write_fs_info);
	fCapabilities.Set(FS_CAPABILITY_SYNC, fFSModule->sync);

	// vnode operations
	fCapabilities.Set(FS_CAPABILITY_LOOKUP, fFSModule->lookup);
	fCapabilities.Set(FS_CAPABILITY_GET_VNODE_NAME, fFSModule->get_vnode_name);

	fCapabilities.Set(FS_CAPABILITY_GET_VNODE, fFSModule->get_vnode);
	fCapabilities.Set(FS_CAPABILITY_PUT_VNODE, fFSModule->put_vnode);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_VNODE, fFSModule->remove_vnode);

	// VM file access
	fCapabilities.Set(FS_CAPABILITY_CAN_PAGE, fFSModule->can_page);
	fCapabilities.Set(FS_CAPABILITY_READ_PAGES, fFSModule->read_pages);
	fCapabilities.Set(FS_CAPABILITY_WRITE_PAGES, fFSModule->write_pages);

	// cache file access
	fCapabilities.Set(FS_CAPABILITY_GET_FILE_MAP, fFSModule->get_file_map);

	// common operations
	fCapabilities.Set(FS_CAPABILITY_IOCTL, fFSModule->ioctl);
	fCapabilities.Set(FS_CAPABILITY_SET_FLAGS, fFSModule->set_flags);
	fCapabilities.Set(FS_CAPABILITY_SELECT, fFSModule->select);
	fCapabilities.Set(FS_CAPABILITY_DESELECT, fFSModule->deselect);
	fCapabilities.Set(FS_CAPABILITY_FSYNC, fFSModule->fsync);

	fCapabilities.Set(FS_CAPABILITY_READ_SYMLINK, fFSModule->read_symlink);
	fCapabilities.Set(FS_CAPABILITY_CREATE_SYMLINK, fFSModule->create_symlink);

	fCapabilities.Set(FS_CAPABILITY_LINK, fFSModule->link);
	fCapabilities.Set(FS_CAPABILITY_UNLINK, fFSModule->unlink);
	fCapabilities.Set(FS_CAPABILITY_RENAME, fFSModule->rename);

	fCapabilities.Set(FS_CAPABILITY_ACCESS, fFSModule->access);
	fCapabilities.Set(FS_CAPABILITY_READ_STAT, fFSModule->read_stat);
	fCapabilities.Set(FS_CAPABILITY_WRITE_STAT, fFSModule->write_stat);

	// file operations
	fCapabilities.Set(FS_CAPABILITY_CREATE, fFSModule->create);
	fCapabilities.Set(FS_CAPABILITY_OPEN, fFSModule->open);
	fCapabilities.Set(FS_CAPABILITY_CLOSE, fFSModule->close);
	fCapabilities.Set(FS_CAPABILITY_FREE_COOKIE, fFSModule->free_cookie);
	fCapabilities.Set(FS_CAPABILITY_READ, fFSModule->read);
	fCapabilities.Set(FS_CAPABILITY_WRITE, fFSModule->write);

	// directory operations
	fCapabilities.Set(FS_CAPABILITY_CREATE_DIR, fFSModule->create_dir);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_DIR, fFSModule->remove_dir);
	fCapabilities.Set(FS_CAPABILITY_OPEN_DIR, fFSModule->open_dir);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_DIR, fFSModule->close_dir);
	fCapabilities.Set(FS_CAPABILITY_FREE_DIR_COOKIE, fFSModule->free_dir_cookie);
	fCapabilities.Set(FS_CAPABILITY_READ_DIR, fFSModule->read_dir);
	fCapabilities.Set(FS_CAPABILITY_REWIND_DIR, fFSModule->rewind_dir);

	// attribute directory operations
	fCapabilities.Set(FS_CAPABILITY_OPEN_ATTR_DIR, fFSModule->open_attr_dir);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_ATTR_DIR, fFSModule->close_attr_dir);
	fCapabilities.Set(FS_CAPABILITY_FREE_ATTR_DIR_COOKIE,
		fFSModule->free_attr_dir_cookie);
	fCapabilities.Set(FS_CAPABILITY_READ_ATTR_DIR, fFSModule->read_attr_dir);
	fCapabilities.Set(FS_CAPABILITY_REWIND_ATTR_DIR, fFSModule->rewind_attr_dir);

	// attribute operations
	fCapabilities.Set(FS_CAPABILITY_CREATE_ATTR, fFSModule->create_attr);
	fCapabilities.Set(FS_CAPABILITY_OPEN_ATTR, fFSModule->open_attr);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_ATTR, fFSModule->close_attr);
	fCapabilities.Set(FS_CAPABILITY_FREE_ATTR_COOKIE,
		fFSModule->free_attr_cookie);
	fCapabilities.Set(FS_CAPABILITY_READ_ATTR, fFSModule->read_attr);
	fCapabilities.Set(FS_CAPABILITY_WRITE_ATTR, fFSModule->write_attr);

	fCapabilities.Set(FS_CAPABILITY_READ_ATTR_STAT, fFSModule->read_attr_stat);
	fCapabilities.Set(FS_CAPABILITY_WRITE_ATTR_STAT,
		fFSModule->write_attr_stat);
	fCapabilities.Set(FS_CAPABILITY_RENAME_ATTR, fFSModule->rename_attr);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_ATTR, fFSModule->remove_attr);

	// index directory & index operations
	fCapabilities.Set(FS_CAPABILITY_OPEN_INDEX_DIR, fFSModule->open_index_dir);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_INDEX_DIR, fFSModule->close_index_dir);
	fCapabilities.Set(FS_CAPABILITY_FREE_INDEX_DIR_COOKIE,
		fFSModule->free_index_dir_cookie);
	fCapabilities.Set(FS_CAPABILITY_READ_INDEX_DIR, fFSModule->read_index_dir);
	fCapabilities.Set(FS_CAPABILITY_REWIND_INDEX_DIR, fFSModule->rewind_index_dir);

	fCapabilities.Set(FS_CAPABILITY_CREATE_INDEX, fFSModule->create_index);
	fCapabilities.Set(FS_CAPABILITY_REMOVE_INDEX, fFSModule->remove_index);
	fCapabilities.Set(FS_CAPABILITY_READ_INDEX_STAT, fFSModule->read_index_stat);

	// query operations
	fCapabilities.Set(FS_CAPABILITY_OPEN_QUERY, fFSModule->open_query);
	fCapabilities.Set(FS_CAPABILITY_CLOSE_QUERY, fFSModule->close_query);
	fCapabilities.Set(FS_CAPABILITY_FREE_QUERY_COOKIE, fFSModule->free_query_cookie);
	fCapabilities.Set(FS_CAPABILITY_READ_QUERY, fFSModule->read_query);
	fCapabilities.Set(FS_CAPABILITY_REWIND_QUERY, fFSModule->rewind_query);
}
