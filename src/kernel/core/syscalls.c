/* Big case statment for dispatching syscalls */
/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <ksyscalls.h>
#include <int.h>
#include <arch/int.h>
#include <debug.h>
#include <vfs.h>
#include <thread.h>
#include <OS.h>
#include <sem.h>
#include <port.h>
#include <cpu.h>
#include <arch/cpu.h>
#include <resource.h>
#include <fd.h>
#include <sysctl.h>
#include <sys/socket.h>
#include <ksocket.h>
#include <sys/ioccom.h>

#define INT32TOINT64(x, y) ((int64)(x) | ((int64)(y) << 32))

#define arg0  (((uint32 *)arg_buffer)[0])
#define arg1  (((uint32 *)arg_buffer)[1])
#define arg2  (((uint32 *)arg_buffer)[2])
#define arg3  (((uint32 *)arg_buffer)[3])
#define arg4  (((uint32 *)arg_buffer)[4])
#define arg5  (((uint32 *)arg_buffer)[5])
#define arg6  (((uint32 *)arg_buffer)[6])
#define arg7  (((uint32 *)arg_buffer)[7])
#define arg8  (((uint32 *)arg_buffer)[8])
#define arg9  (((uint32 *)arg_buffer)[9])
#define arg10 (((uint32 *)arg_buffer)[10])
#define arg11 (((uint32 *)arg_buffer)[11])
#define arg12 (((uint32 *)arg_buffer)[12])
#define arg13 (((uint32 *)arg_buffer)[13])
#define arg14 (((uint32 *)arg_buffer)[14])
#define arg15 (((uint32 *)arg_buffer)[15])

int syscall_dispatcher(unsigned long call_num, void *arg_buffer, uint64 *call_ret)
{
//	dprintf("syscall_dispatcher: thread 0x%x call 0x%x, arg0 0x%x, arg1 0x%x arg2 0x%x arg3 0x%x arg4 0x%x\n",
//		thread_get_current_thread_id(), call_num, arg0, arg1, arg2, arg3, arg4);

	switch(call_num) {
		case SYSCALL_NULL:
			*call_ret = 0;
			break;
		case SYSCALL_MOUNT:
			*call_ret = user_mount((const char *)arg0, (const char *)arg1, (const char *)arg2, (void *)arg3);
			break;
		case SYSCALL_UNMOUNT:
			*call_ret = user_unmount((const char *)arg0);
			break;
		case SYSCALL_SYNC:
			*call_ret = user_sync();
			break;
		case SYSCALL_OPEN:
			*call_ret = user_open((const char *)arg0, (stream_type)arg1, (int)arg2);
			break;
		case SYSCALL_CLOSE:
			*call_ret = user_close((int)arg0);
			break;
		case SYSCALL_FSYNC:
			*call_ret = user_fsync((int)arg0);
			break;
		case SYSCALL_READ:
			*call_ret = user_read((int)arg0, (void *)arg1, (off_t)INT32TOINT64(arg2, arg3), (ssize_t)arg4);
			break;
		case SYSCALL_WRITE:
			*call_ret = user_write((int)arg0, (const void *)arg1, (off_t)INT32TOINT64(arg2, arg3), (ssize_t)arg4);
			break;
		case SYSCALL_SEEK:
			*call_ret = user_seek((int)arg0, (off_t)INT32TOINT64(arg1, arg2), (int)arg3);
			break;
		case SYSCALL_IOCTL:
			// ToDo: this is not correct; IOCPARM is only valid for calls to the networking stack
			// The socket/fd_ioctl should do this, but I think we have to pass 0 here -- axeld.
			// currently ignoring arg3 (which is supposed to be the length, but currently
			// always 0 - in libc/system/wrappers.c
			*call_ret = user_ioctl((int)arg0, (ulong)arg1, (void *)arg2, (size_t)IOCPARM_LEN((ulong)arg1));
			break;
		case SYSCALL_CREATE:
			*call_ret = user_create((const char *)arg0, (stream_type)arg1);
			break;
		case SYSCALL_UNLINK:
			*call_ret = user_unlink((const char *)arg0);
			break;
		case SYSCALL_RENAME:
			*call_ret = user_rename((const char *)arg0, (const char *)arg1);
			break;
		case SYSCALL_RSTAT:
			*call_ret = user_rstat((const char *)arg0, (struct stat *)arg1);
			break;
		case SYSCALL_FSTAT:
			*call_ret = user_fstat((int)arg0, (struct stat*)arg1);
			break;
		case SYSCALL_WSTAT:
			*call_ret = user_wstat((const char *)arg0, (struct stat *)arg1, (int)arg2);
			break;
		case SYSCALL_SYSTEM_TIME:
			*call_ret = system_time();
			break;
		case SYSCALL_SNOOZE:
			*call_ret = user_thread_snooze((bigtime_t)INT32TOINT64(arg0, arg1));
			break;
		case SYSCALL_SEM_CREATE:
			*call_ret = user_create_sem((int)arg0, (const char *)arg1);
			break;
		case SYSCALL_SEM_DELETE:
			*call_ret = user_delete_sem((sem_id)arg0);
			break;
		case SYSCALL_SEM_ACQUIRE:
			*call_ret = user_acquire_sem_etc((sem_id)arg0, 1, 0, 0);
			break;
		case SYSCALL_SEM_ACQUIRE_ETC:
			*call_ret = user_acquire_sem_etc((sem_id)arg0, (int)arg1, (int)arg2, (bigtime_t)INT32TOINT64(arg3, arg4));
			break;
		case SYSCALL_SEM_RELEASE:
			*call_ret = user_release_sem((sem_id)arg0);
			break;
		case SYSCALL_SEM_RELEASE_ETC:
			*call_ret = user_release_sem_etc((sem_id)arg0, (int)arg1, (int)arg2);
			break;
		case SYSCALL_GET_CURRENT_THREAD_ID:
			*call_ret = thread_get_current_thread_id();
			break;
		case SYSCALL_EXIT_THREAD:
			thread_exit((int)arg0);
			*call_ret = 0;
			break;
		case SYSCALL_PROC_CREATE_PROC:
			*call_ret = user_proc_create_proc((const char *)arg0, (const char *)arg1, (char **)arg2, (int )arg3, (int)arg4);
			break;
		case SYSCALL_THREAD_WAIT_ON_THREAD:
			*call_ret = user_thread_wait_on_thread((thread_id)arg0, (int *)arg1);
			break;
		case SYSCALL_PROC_WAIT_ON_PROC:
			*call_ret = user_proc_wait_on_proc((proc_id)arg0, (int *)arg1);
			break;
		case SYSCALL_VM_CREATE_ANONYMOUS_REGION:
			*call_ret = user_vm_create_anonymous_region(
				(char *)arg0, (void **)arg1, (int)arg2,
				(addr)arg3, (int)arg4, (int)arg5);
			break;
		case SYSCALL_VM_CLONE_REGION:
			*call_ret = user_vm_clone_region(
				(char *)arg0, (void **)arg1, (int)arg2,
				(region_id)arg3, (int)arg4, (int)arg5);
			break;
		case SYSCALL_VM_MAP_FILE:
			*call_ret = user_vm_map_file(
				(char *)arg0, (void **)arg1, (int)arg2,
				(addr)arg3, (int)arg4, (int)arg5, (const char *)arg6,
				(off_t)INT32TOINT64(arg7, arg8));
			break;
		case SYSCALL_VM_FIND_REGION_BY_NAME:
			*call_ret = find_region_by_name((const char *)arg0);
			break;
		case SYSCALL_VM_DELETE_REGION:
			*call_ret = vm_delete_region(vm_get_current_user_aspace_id(), (region_id)arg0);
			break;
		case SYSCALL_VM_GET_REGION_INFO:
			*call_ret = user_vm_get_region_info((region_id)arg0, (vm_region_info *)arg1);
			break;
		case SYSCALL_SPAWN_THREAD:
			*call_ret = user_thread_create_user_thread((addr)arg0, thread_get_current_thread()->proc->id, 
			                                           (const char*)arg1, (int)arg2, (void *)arg3);
			break;
		case SYSCALL_KILL_THREAD:
			*call_ret = thread_kill_thread((thread_id)arg0);
			break;
		case SYSCALL_SUSPEND_THREAD:
			*call_ret = thread_suspend_thread((thread_id)arg0);
			break;
		case SYSCALL_RESUME_THREAD:
			*call_ret = thread_resume_thread((thread_id)arg0);
			break;
		case SYSCALL_PROC_KILL_PROC:
			*call_ret = proc_kill_proc((proc_id)arg0);
			break;
		case SYSCALL_GET_CURRENT_PROC_ID:
			*call_ret = proc_get_current_proc_id();
			break;
		case SYSCALL_GETCWD:
			*call_ret = user_getcwd((char*)arg0, (size_t)arg1);
			break;
		case SYSCALL_SETCWD:
			*call_ret = user_setcwd((const char*)arg0);
			break;
		case SYSCALL_PORT_CREATE:
			*call_ret = user_create_port((int32)arg0, (const char *)arg1);
			break;
		case SYSCALL_PORT_CLOSE:
			*call_ret = user_close_port((port_id)arg0);
			break;
		case SYSCALL_PORT_DELETE:
			*call_ret = user_delete_port((port_id)arg0);
			break;
		case SYSCALL_PORT_FIND:
			*call_ret = user_find_port((const char *)arg0);
			break;
		case SYSCALL_PORT_GET_INFO:
			*call_ret = user_get_port_info((port_id)arg0, (struct port_info *)arg1);
			break;
		case SYSCALL_PORT_GET_NEXT_PORT_INFO:
			*call_ret = user_get_next_port_info((port_id)arg0, (uint32 *)arg1, (struct port_info *)arg2);
			break;
		case SYSCALL_PORT_BUFFER_SIZE:
			*call_ret = user_port_buffer_size_etc((port_id)arg0, B_CAN_INTERRUPT, 0);
			break;
		case SYSCALL_PORT_BUFFER_SIZE_ETC:
			*call_ret = user_port_buffer_size_etc((port_id)arg0, (uint32)arg1 | B_CAN_INTERRUPT, (bigtime_t)INT32TOINT64(arg2, arg3));
			break;
		case SYSCALL_PORT_COUNT:
			*call_ret = user_port_count((port_id)arg0);
			break;
		case SYSCALL_PORT_READ:
			*call_ret = user_read_port_etc((port_id)arg0, (int32*)arg1, (void*)arg2, (size_t)arg3, B_CAN_INTERRUPT, 0);
			break;
		case SYSCALL_PORT_READ_ETC:
			*call_ret = user_read_port_etc((port_id)arg0, (int32*)arg1, (void*)arg2, (size_t)arg3, (uint32)arg4 | B_CAN_INTERRUPT, (bigtime_t)INT32TOINT64(arg5, arg6));
			break;
		case SYSCALL_PORT_SET_OWNER:
			*call_ret = user_set_port_owner((port_id)arg0, (proc_id)arg1);
			break;
		case SYSCALL_PORT_WRITE:
			*call_ret = user_write_port_etc((port_id)arg0, (int32)arg1, (void *)arg2, (size_t)arg3, B_CAN_INTERRUPT, 0);
			break;
		case SYSCALL_PORT_WRITE_ETC:
			*call_ret = user_write_port_etc((port_id)arg0, (int32)arg1, (void *)arg2, (size_t)arg3, (uint32)arg4 | B_CAN_INTERRUPT, (bigtime_t)INT32TOINT64(arg5, arg6));
			break;
		case SYSCALL_SEM_GET_COUNT:
			*call_ret = user_get_sem_count((sem_id)arg0, (int32*)arg1);
			break;
		case SYSCALL_SEM_GET_SEM_INFO:
			*call_ret = user_get_sem_info((sem_id)arg0, (struct sem_info *)arg1, (size_t)arg2);
			break;
		case SYSCALL_SEM_GET_NEXT_SEM_INFO:
			*call_ret = user_get_next_sem_info((proc_id)arg0, (uint32 *)arg1, (struct sem_info *)arg2,
			                                   (size_t)arg3);
			break;
		case SYSCALL_SEM_SET_SEM_OWNER:
			*call_ret = user_set_sem_owner((sem_id)arg0, (proc_id)arg1);
			break;
		case SYSCALL_FDDUP:
			*call_ret = user_dup(arg0);
			break;
		case SYSCALL_FDDUP2:
			*call_ret = user_dup2(arg0, arg1);
			break;
		case SYSCALL_GET_PROC_TABLE:
			*call_ret = user_proc_get_table((struct proc_info *)arg0, (size_t)arg1);
			break;
		case SYSCALL_GETRLIMIT:
			*call_ret = user_getrlimit((int)arg0, (struct rlimit *)arg1);
			break;
		case SYSCALL_SETRLIMIT:
			*call_ret = user_setrlimit((int)arg0, (const struct rlimit *)arg1);
			break;
		case SYSCALL_ATOMIC_ADD:
			*call_ret = user_atomic_add((int *)arg0, (int)arg1);
			break;
		case SYSCALL_ATOMIC_AND:
			*call_ret = user_atomic_and((int *)arg0, (int)arg1);
			break;
		case SYSCALL_ATOMIC_OR:
			*call_ret = user_atomic_or((int *)arg0, (int)arg1);
			break;
		case SYSCALL_ATOMIC_SET:
			*call_ret = user_atomic_set((int *)arg0, (int)arg1);
			break;
		case SYSCALL_TEST_AND_SET:
			*call_ret = user_test_and_set((int *)arg0, (int)arg1, (int)arg2);
			break;
		case SYSCALL_SYSCTL:
			*call_ret = user_sysctl((int *)arg0, (uint)arg1, (void *)arg2, 
			                        (size_t *)arg3, (void *)arg4, 
			                        (size_t)arg5);
			break;
		case SYSCALL_SOCKET:
			*call_ret = socket((int)arg0, (int)arg1, (int)arg2, false);
			break;
		case SYSCALL_GETDTABLESIZE:
			// ToDo: the correct way would be to lock the io_context
			// or just call vfs_getrlimit()
			*call_ret = (get_current_io_context(false))->table_size;
			break;
		default:
			*call_ret = -1;
	}

//	dprintf("syscall_dispatcher: done with syscall 0x%x\n", call_num);

	return INT_RESCHEDULE;
}
