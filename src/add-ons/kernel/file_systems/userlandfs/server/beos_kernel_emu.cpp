// beos_kernel_emu.cpp

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <legacy/cache.h>
#include <legacy/fsproto.h>
#include <legacy/lock.h>

#include "beos_fs_cache.h"
#include "beos_lock.h"
#include "kernel_emu.h"


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
notify_listener(int op, nspace_id nsid, vnode_id vnida,	vnode_id vnidb,
	vnode_id vnidc, const char *name)
{
	return UserlandFS::KernelEmu::notify_listener(op, nsid, vnida, vnidb, vnidc,
		name);
}

// notify_select_event
void
notify_select_event(selectsync *sync, uint32 ref)
{
	// TODO: Check what best to supply as event arg!
	UserlandFS::KernelEmu::notify_select_event(sync, ref, 0);
}

// send_notification
int
send_notification(port_id port, long token, ulong what, long op,
	nspace_id nsida, nspace_id nsidb, vnode_id vnida, vnode_id vnidb,
	vnode_id vnidc, const char *name)
{
	return UserlandFS::KernelEmu::send_notification(port, token, what, op,
		nsida, nsidb, vnida, vnidb, vnidc, name);
}


// #pragma mark - VNodes


// get_vnode
int
get_vnode(nspace_id nsid, vnode_id vnid, void **data)
{
	return UserlandFS::KernelEmu::get_vnode(nsid, vnid, data);
}

// put_vnode
int
put_vnode(nspace_id nsid, vnode_id vnid)
{
	return UserlandFS::KernelEmu::put_vnode(nsid, vnid);
}

// new_vnode
int
new_vnode(nspace_id nsid, vnode_id vnid, void *data)
{
	return UserlandFS::KernelEmu::new_vnode(nsid, vnid, data);
}

// remove_vnode
int
remove_vnode(nspace_id nsid, vnode_id vnid)
{
	return UserlandFS::KernelEmu::remove_vnode(nsid, vnid);
}

// unremove_vnode
int
unremove_vnode(nspace_id nsid, vnode_id vnid)
{
	return UserlandFS::KernelEmu::unremove_vnode(nsid, vnid);
}

// is_vnode_removed
int
is_vnode_removed(nspace_id nsid, vnode_id vnid)
{
	return UserlandFS::KernelEmu::is_vnode_removed(nsid, vnid);
}


// #pragma mark - Block Cache


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
	int bufferSize = sizeof(buffer) - prefixLen;
	va_list args;
	va_start(args, format);
	// no vsnprintf() on PPC
	#if defined(__INTEL__)
		vsnprintf(buffer + prefixLen, bufferSize - 1, format, args);
	#else
		vsprintf(buffer + prefixLen, format, args);
	#endif
	va_end(args);
	buffer[sizeof(buffer) - 1] = '\0';
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
	return B_OK;
}

// remove_debugger_command
int
remove_debugger_command(char *name,
	int (*func)(int argc, char **argv))
{
	return B_OK;
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
