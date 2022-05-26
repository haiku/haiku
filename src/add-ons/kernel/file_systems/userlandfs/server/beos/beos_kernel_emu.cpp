// beos_kernel_emu.cpp

#include "beos_kernel_emu.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <AppDefs.h>
#include <NodeMonitor.h>

#include <legacy/cache.h>
#include <legacy/fsproto.h>
#include <legacy/lock.h>

#include "Debug.h"

#include "../kernel_emu.h"

#include "fs_cache.h"
#include "lock.h"


// #pragma mark - Paths


// new_path
int
new_path(const char *path, char **copy)
{
	return UserlandFS::KernelEmu::new_path(path, copy);
}

// free_path
void
free_path(char *p)
{
	UserlandFS::KernelEmu::free_path(p);
}


// #pragma mark - Notifications


// notify_listener
int
notify_listener(int op, nspace_id nsid, ino_t vnida, ino_t vnidb,
	ino_t vnidc, const char *name)
{
	switch (op) {
		case B_ENTRY_CREATED:
		case B_ENTRY_REMOVED:
			if (!name)
				name = "";
			return UserlandFS::KernelEmu::notify_listener(op, 0, nsid, 0,
				vnida, vnidc, NULL, name);

		case B_ENTRY_MOVED:
			if (!name)
				name = "";
			// the old entry name is not available with the old interface
			return UserlandFS::KernelEmu::notify_listener(op, 0, nsid, vnida,
				vnidb, vnidc, "", name);

		case B_STAT_CHANGED:
		{
			// we don't know what stat field changed, so we mark them all
			uint32 statFields = B_STAT_MODE | B_STAT_UID | B_STAT_GID
				| B_STAT_SIZE | B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME
				| B_STAT_CREATION_TIME | B_STAT_CHANGE_TIME;
			return UserlandFS::KernelEmu::notify_listener(op, statFields, nsid,
				0, vnida, vnidc, NULL, NULL);
		}

		case B_ATTR_CHANGED:
			if (!name)
				name = "";
			return UserlandFS::KernelEmu::notify_listener(op, B_ATTR_CHANGED,
				nsid, 0, vnida, vnidc, NULL, name);

		default:
			return B_BAD_VALUE;
	}
}

// notify_select_event
void
notify_select_event(selectsync *sync, uint32 ref)
{
	UserlandFS::KernelEmu::notify_select_event(sync, 0, true);
}

// send_notification
int
send_notification(port_id port, long token, ulong what, long op,
	nspace_id nsida, nspace_id /*nsidb*/, ino_t vnida, ino_t vnidb,
	ino_t vnidc, const char *name)
{
	if (what != B_QUERY_UPDATE)
		return B_BAD_VALUE;

	// check the name
	if (!name)
		name = "";

	switch (op) {
		case B_ENTRY_CREATED:
		case B_ENTRY_REMOVED:
			return UserlandFS::KernelEmu::notify_query(port, token, op, nsida,
				vnida, name, vnidc);
		case B_ENTRY_MOVED:
		{
			// translate to a B_ENTRY_REMOVED + B_ENTRY_CREATED pair
			// We do at least miss the original name though.
			status_t error = UserlandFS::KernelEmu::notify_query(port, token,
				B_ENTRY_REMOVED, nsida, vnida, "", vnidc);
			if (error != B_OK)
				return error;

			return UserlandFS::KernelEmu::notify_query(port, token,
				B_ENTRY_CREATED, nsida, vnidb, name, vnidc);
		}

		default:
			return B_BAD_VALUE;
	}
}


// #pragma mark - VNodes


// get_vnode
int
get_vnode(nspace_id nsid, ino_t vnid, void **data)
{
	return UserlandFS::KernelEmu::get_vnode(nsid, vnid, data);
}

// put_vnode
int
put_vnode(nspace_id nsid, ino_t vnid)
{
	return UserlandFS::KernelEmu::put_vnode(nsid, vnid);
}

// new_vnode
int
new_vnode(nspace_id nsid, ino_t vnid, void *data)
{
	// get the node capabilities
	FSVNodeCapabilities capabilities;
	get_beos_file_system_node_capabilities(capabilities);

	// The semantics of new_vnode() has changed. The new publish_vnode()
	// should work like the former new_vnode().
	return UserlandFS::KernelEmu::publish_vnode(nsid, vnid, data,
		capabilities);
}

// remove_vnode
int
remove_vnode(nspace_id nsid, ino_t vnid)
{
	return UserlandFS::KernelEmu::remove_vnode(nsid, vnid);
}

// unremove_vnode
int
unremove_vnode(nspace_id nsid, ino_t vnid)
{
	return UserlandFS::KernelEmu::unremove_vnode(nsid, vnid);
}

// is_vnode_removed
int
is_vnode_removed(nspace_id nsid, ino_t vnid)
{
	bool removed;
	status_t error = UserlandFS::KernelEmu::get_vnode_removed(nsid, vnid,
		&removed);
	if (error != B_OK)
		return error;
	return (removed ? 1 : 0);
}


// #pragma mark - Locking


int
new_lock(lock *l, const char *name)
{
	return beos_new_lock((beos_lock*)l, name);
}

int
free_lock(lock *l)
{
	return beos_free_lock((beos_lock*)l);
}

int
new_mlock(mlock *l, long c, const char *name)
{
	return beos_new_mlock((beos_mlock*)l, c, name);
}

int
free_mlock(mlock *l)
{
	return beos_free_mlock((beos_mlock*)l);
}


// #pragma mark - Block Cache


// init_block_cache
int
init_block_cache(int max_blocks, int flags)
{
	return beos_init_block_cache(max_blocks, flags);
}

void
shutdown_block_cache(void)
{
	beos_shutdown_block_cache();
}

void
force_cache_flush(int dev, int prefer_log_blocks)
{
	beos_force_cache_flush(dev, prefer_log_blocks);
}

int
flush_blocks(int dev, off_t bnum, int nblocks)
{
	return beos_flush_blocks(dev, bnum, nblocks);
}

int
flush_device(int dev, int warn_locked)
{
	return beos_flush_device(dev, warn_locked);
}

int
init_cache_for_device(int fd, off_t max_blocks)
{
	return beos_init_cache_for_device(fd, max_blocks);
}

int
remove_cached_device_blocks(int dev, int allow_write)
{
	return beos_remove_cached_device_blocks(dev, allow_write);
}

void *
get_block(int dev, off_t bnum, int bsize)
{
	return beos_get_block(dev, bnum, bsize);
}

void *
get_empty_block(int dev, off_t bnum, int bsize)
{
	return beos_get_empty_block(dev, bnum, bsize);
}

int
release_block(int dev, off_t bnum)
{
	return beos_release_block(dev, bnum);
}

int
mark_blocks_dirty(int dev, off_t bnum, int nblocks)
{
	return beos_mark_blocks_dirty(dev, bnum, nblocks);
}

int
cached_read(int dev, off_t bnum, void *data, off_t num_blocks, int bsize)
{
	return beos_cached_read(dev, bnum, data, num_blocks, bsize);
}

int
cached_write(int dev, off_t bnum, const void *data, off_t num_blocks, int bsize)
{
	return beos_cached_write(dev, bnum, data, num_blocks, bsize);
}

int
cached_write_locked(int dev, off_t bnum, const void *data, off_t num_blocks,
	int bsize)
{
	return beos_cached_write_locked(dev, bnum, data, num_blocks, bsize);
}

int
set_blocks_info(int dev, off_t *blocks, int nblocks,
	void (*func)(off_t bnum, size_t nblocks, void *arg), void *arg)
{
	return beos_set_blocks_info(dev, blocks, nblocks, func, arg);
}

size_t
read_phys_blocks(int fd, off_t bnum, void *data, uint num_blocks, int bsize)
{
	return beos_read_phys_blocks(fd, bnum, data, num_blocks, bsize);
}

size_t
write_phys_blocks(int fd, off_t bnum, void *data, uint num_blocks, int bsize)
{
	return beos_write_phys_blocks(fd, bnum, data, num_blocks, bsize);
}


// #pragma mark - Misc


// kernel_debugger
void
kernel_debugger(const char *message)
{
	UserlandFS::KernelEmu::kernel_debugger(message);
}

// panic
void
panic(const char *format, ...)
{
	char buffer[1024];
	strcpy(buffer, "PANIC: ");
	int32 prefixLen = strlen(buffer);

	va_list args;
	va_start(args, format);
	vsnprintf(buffer + prefixLen, sizeof(buffer) - prefixLen, format, args);
	va_end(args);

	debugger(buffer);
}

// parse_expression
//ulong
//parse_expression(char *str)
//{
//	return 0;
//}

// add_debugger_command
int
add_debugger_command(char *name,
	int (*func)(int argc, char **argv), char *help)
{
	return UserlandFS::KernelEmu::add_debugger_command(name, func, help);
}

// remove_debugger_command
int
remove_debugger_command(char *name,
	int (*func)(int argc, char **argv))
{
	return UserlandFS::KernelEmu::remove_debugger_command(name, func);
}

// parse_expression
uint32
parse_expression(const char *string)
{
	return UserlandFS::KernelEmu::parse_expression(string);
}

// dprintf
void
dprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	UserlandFS::KernelEmu::vdprintf(format, args);
	va_end(args);
}

// kprintf
void
kprintf(const char *format, ...)
{
}

// spawn_kernel_thread
thread_id
spawn_kernel_thread(thread_entry function, const char *threadName,
	long priority, void *arg)
{
	return UserlandFS::KernelEmu::spawn_kernel_thread(function,	threadName,
		priority, arg);
}
