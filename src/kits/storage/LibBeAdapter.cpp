// LibBeAdapter.cpp

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <string.h>

#include <syscalls.h>


extern "C" status_t
_kern_dir_node_ref_to_path(dev_t device, ino_t inode, char *buffer, size_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	node_ref nodeRef;
	nodeRef.device = device;
	nodeRef.node = inode;

	BDirectory directory;
	status_t error = directory.SetTo(&nodeRef);

	BEntry entry;
	if (error == B_OK)
		error = directory.GetEntry(&entry);

	BPath path;
	if (error == B_OK)
		error = entry.GetPath(&path);

	if (error == B_OK) {
		if (size >= strlen(path.Path()) + 1)
			strcpy(buffer, path.Path());
		else
			error = B_BAD_VALUE;
	}
	return error;
}


// Syscall mapping between R5 and us

// private libroot.so functions

extern "C" status_t _kwfsstat_(dev_t device, const struct fs_info *info, long mask);

extern "C" status_t _kstart_watching_vnode_(dev_t device, ino_t node,
											uint32 flags, port_id port,
											int32 handlerToken);

extern "C" status_t _kstop_watching_vnode_(dev_t device, ino_t node,
										   port_id port, int32 handlerToken);


extern "C" status_t _kstop_notifying_(port_id port, int32 handlerToken);


status_t
_kern_write_fs_info(dev_t device, const struct fs_info *info, int mask)
{
	return _kwfsstat_(device, info, mask);
}


status_t
_kern_stop_notifying(port_id port, uint32 token)
{
	return _kstop_notifying_(port, token);
}


status_t
_kern_start_watching(dev_t device, ino_t node, uint32 flags,
	port_id port, uint32 token)
{
	return _kstart_watching_vnode_(device, node, flags, port, token);
}

			
status_t
_kern_stop_watching(dev_t device, ino_t node, uint32 flags,
	port_id port, uint32 token)
{
	(void)flags;
	return _kstop_watching_vnode_(device, node, port, token);
}

