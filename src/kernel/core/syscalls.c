/* Big case statment for dispatching syscalls */
/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <kernel.h>
#include <ksyscalls.h>
#include <syscalls.h>
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
#include <sys/resource.h>
#include <fd.h>
#include <fs/node_monitor.h>
#include <sysctl.h>
#include <ksocket.h>
#include <kimage.h>
#include <ksignal.h>
#include <real_time_clock.h>
#include <system_info.h>
#include <sys/ioccom.h>
#include <sys/socket.h>
#include <user_atomic.h>

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


int
syscall_dispatcher(unsigned long call_num, void *arg_buffer, uint64 *call_ret)
{
//	dprintf("syscall_dispatcher: thread 0x%x call 0x%x, arg0 0x%x, arg1 0x%x arg2 0x%x arg3 0x%x arg4 0x%x\n",
//		thread_get_current_thread_id(), call_num, arg0, arg1, arg2, arg3, arg4);

	switch (call_num) {
		case SYSCALL_NULL:
			*call_ret = 0;
			break;
		case SYSCALL_MOUNT:
			*call_ret = _user_mount((const char *)arg0, (const char *)arg1, (const char *)arg2, (void *)arg3);
			break;
		case SYSCALL_UNMOUNT:
			*call_ret = _user_unmount((const char *)arg0);
			break;
		case SYSCALL_SYNC:
			*call_ret = _user_sync();
			break;
		case SYSCALL_OPEN_ENTRY_REF:
			*call_ret = _user_open_entry_ref((dev_t)arg0, (ino_t)INT32TOINT64(arg1, arg2), (const char *)arg3, (int)arg4);
			break;
		case SYSCALL_OPEN:
			*call_ret = _user_open((const char *)arg0, (int)arg1);
			break;
		case SYSCALL_CLOSE:
			*call_ret = _user_close((int)arg0);
			break;
		case SYSCALL_FSYNC:
			*call_ret = _user_fsync((int)arg0);
			break;
		case SYSCALL_READ:
			*call_ret = _user_read((int)arg0, (off_t)INT32TOINT64(arg1, arg2), (void *)arg3, (ssize_t)arg4);
			break;
		case SYSCALL_WRITE:
			*call_ret = _user_write((int)arg0, (off_t)INT32TOINT64(arg1, arg2), (const void *)arg3, (ssize_t)arg4);
			break;
		case SYSCALL_SEEK:
			*call_ret = _user_seek((int)arg0, (off_t)INT32TOINT64(arg1, arg2), (int)arg3);
			break;
		case SYSCALL_OPEN_DIR_ENTRY_REF:
			*call_ret = _user_open_dir_entry_ref((dev_t)arg0, (ino_t)INT32TOINT64(arg1,arg2), (const char *)arg3);
			break;
		case SYSCALL_OPEN_DIR_NODE_REF:
			*call_ret = _user_open_dir_node_ref((dev_t)arg0, (ino_t)INT32TOINT64(arg1,arg2));
			break;
		case SYSCALL_OPEN_DIR:
			*call_ret = _user_open_dir((const char *)arg0);
			break;
		case SYSCALL_READ_DIR:
			*call_ret = _user_read_dir((int)arg0, (struct dirent *)arg1, (size_t)arg2, (uint32)arg3);
			break;
		case SYSCALL_REWIND_DIR:
			*call_ret = _user_rewind_dir((int)arg0);
			break;
		case SYSCALL_IOCTL:
			*call_ret = _user_ioctl((int)arg0, (ulong)arg1, (void *)arg2, (size_t)arg3);
			break;
		case SYSCALL_CREATE_ENTRY_REF:
			*call_ret = _user_create_entry_ref((dev_t)arg0, (ino_t)INT32TOINT64(arg1,arg2), (const char *)arg3, (int)arg4, (int)arg5);
			break;
		case SYSCALL_CREATE:
			*call_ret = _user_create((const char *)arg0, (int)arg1, (int)arg2);
			break;
		case SYSCALL_CREATE_DIR_ENTRY_REF:
			*call_ret = _user_create_dir_entry_ref((dev_t)arg0, (ino_t)INT32TOINT64(arg1,arg2), (const char *)arg3, (int)arg4);
			break;
		case SYSCALL_CREATE_DIR:
			*call_ret = _user_create_dir((const char *)arg0, (int)arg1);
			break;
		case SYSCALL_CREATE_SYMLINK:
			*call_ret = _user_create_symlink((const char *)arg0, (const char *)arg1, (int)arg2);
			break;
		case SYSCALL_CREATE_LINK:
			*call_ret = _user_create_link((const char *)arg0, (const char *)arg1);
			break;
		case SYSCALL_READ_LINK:
			*call_ret = _user_read_link((const char *)arg0, (char *)arg1, (size_t)arg2);
			break;
		case SYSCALL_REMOVE_DIR:
			*call_ret = _user_remove_dir((const char *)arg0);
			break;
		case SYSCALL_UNLINK:
			*call_ret = _user_unlink((const char *)arg0);
			break;
		case SYSCALL_RENAME:
			*call_ret = _user_rename((const char *)arg0, (const char *)arg1);
			break;
		case SYSCALL_READ_PATH_STAT:
			*call_ret = _user_read_path_stat((const char *)arg0, (bool)arg1, (struct stat *)arg2, (size_t)arg3);
			break;
		case SYSCALL_WRITE_PATH_STAT:
			*call_ret = _user_write_path_stat((const char *)arg0, (bool)arg1, (const struct stat *)arg2, (size_t)arg3, (int)arg4);
			break;
		case SYSCALL_READ_STAT:
			*call_ret = _user_read_stat((int)arg0, (struct stat *)arg1, (size_t)arg2);
			break;
		case SYSCALL_WRITE_STAT:
			*call_ret = _user_write_stat((int)arg0, (const struct stat *)arg1, (size_t)arg2, (int)arg3);
			break;
		case SYSCALL_ACCESS:
			*call_ret = _user_access((const char *)arg0, (int)arg1);
			break;
		case SYSCALL_SELECT:
			*call_ret = _user_select((int)arg0, (fd_set *)arg1, (fd_set *)arg2, (fd_set *)arg3, (bigtime_t)INT32TOINT64(arg4, arg5), (sigset_t *)arg6);
			break;
		case SYSCALL_POLL:
			*call_ret = _user_poll((struct pollfd *)arg0, (int)arg1, (bigtime_t)INT32TOINT64(arg2, arg3));
			break;
		case SYSCALL_OPEN_ATTR_DIR:
			*call_ret = _user_open_attr_dir((int)arg0, (const char *)arg1);
			break;
		case SYSCALL_CREATE_ATTR:
			*call_ret = _user_create_attr((int)arg0, (const char *)arg1, (uint32)arg2, (int)arg3);
			break;
		case SYSCALL_OPEN_ATTR:
			*call_ret = _user_open_attr((int)arg0, (const char *)arg1, (int)arg2);
			break;
		case SYSCALL_REMOVE_ATTR:
			*call_ret = _user_remove_attr((int)arg0, (const char *)arg1);
			break;
		case SYSCALL_RENAME_ATTR:
			*call_ret = _user_rename_attr((int)arg0, (const char *)arg1, (int)arg2, (const char *)arg3);
			break;
		case SYSCALL_OPEN_INDEX_DIR:
			*call_ret = _user_open_index_dir((dev_t)arg0);
			break;
		case SYSCALL_CREATE_INDEX:
			*call_ret = _user_create_index((dev_t)arg0, (const char *)arg1, (uint32)arg2, (uint32)arg3);
			break;
		case SYSCALL_READ_INDEX_STAT:
			*call_ret = _user_read_index_stat((dev_t)arg0, (const char *)arg1, (struct stat *)arg2);
			break;
		case SYSCALL_REMOVE_INDEX:
			*call_ret = _user_remove_index((dev_t)arg0, (const char *)arg1);
			break;
		case SYSCALL_FDDUP:
			*call_ret = _user_dup(arg0);
			break;
		case SYSCALL_FDDUP2:
			*call_ret = _user_dup2(arg0, arg1);
			break;
		case SYSCALL_GETCWD:
			*call_ret = _user_getcwd((char*)arg0, (size_t)arg1);
			break;
		case SYSCALL_SETCWD:
			*call_ret = _user_setcwd((int)arg0, (const char *)arg1);
			break;

		case SYSCALL_SYSTEM_TIME:
			*call_ret = system_time();
			break;
		case SYSCALL_SNOOZE_ETC:
			*call_ret = _user_snooze_etc((bigtime_t)INT32TOINT64(arg0, arg1), (int)arg2, (int32)arg3);
			break;

		/* semaphore syscalls */

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

		/* VM/area syscalls */

		case SYSCALL_CREATE_AREA:
			*call_ret = _user_create_area((char *)arg0, (void **)arg1, (uint32)arg2,
				(size_t)arg3, (uint32)arg4, (uint32)arg5);
			break;
		case SYSCALL_CLONE_AREA:
			*call_ret = _user_clone_area((char *)arg0, (void **)arg1, (uint32)arg2,
				(uint32)arg3, (area_id)arg4);
			break;
		case SYSCALL_VM_MAP_FILE:
			*call_ret = user_vm_map_file(
				(char *)arg0, (void **)arg1, (int)arg2,
				(addr)arg3, (int)arg4, (int)arg5, (const char *)arg6,
				(off_t)INT32TOINT64(arg7, arg8));
			break;
		case SYSCALL_FIND_AREA:
			*call_ret = _user_find_area((const char *)arg0);
			break;
		case SYSCALL_DELETE_AREA:
			*call_ret = _user_delete_area((area_id)arg0);
			break;
		case SYSCALL_GET_AREA_INFO:
			*call_ret = _user_get_area_info((area_id)arg0, (area_info *)arg1);
			break;
		case SYSCALL_GET_NEXT_AREA_INFO:
			*call_ret = _user_get_next_area_info((team_id)arg0, (int32 *)arg1, (area_info *)arg2);
			break;
		case SYSCALL_SET_AREA_PROTECTION:
			*call_ret = _user_set_area_protection((area_id)arg0, (uint32)arg1);
			break;
		case SYSCALL_AREA_FOR:
			*call_ret = _user_area_for((void *)arg0);
			break;

		/* Thread/team syscalls */

		case SYSCALL_FIND_THREAD:
			*call_ret = _user_find_thread((const char *)arg0);
			break;
		case SYSCALL_EXIT_THREAD:
			_user_exit_thread((status_t)arg0);
			*call_ret = 0;
			break;
		case SYSCALL_CREATE_TEAM:
			*call_ret = _user_create_team((const char *)arg0, (const char *)arg1, (char **)arg2, (int)arg3, (char **)arg4, (int)arg5, (int)arg6);
			break;
		case SYSCALL_WAIT_FOR_THREAD:
			*call_ret = _user_wait_for_thread((thread_id)arg0, (status_t *)arg1);
			break;
		case SYSCALL_WAIT_FOR_TEAM:
			*call_ret = _user_wait_for_team((team_id)arg0, (status_t *)arg1);
			break;
		case SYSCALL_SPAWN_THREAD:
			*call_ret = _user_spawn_thread((thread_func)arg0, (const char *)arg1, (int)arg2, (void *)arg3, (void *)arg4);
			break;
		case SYSCALL_SET_THREAD_PRIORITY:
			*call_ret = _user_set_thread_priority((thread_id)arg0, (int32)arg1);
			break;
		case SYSCALL_KILL_THREAD:
			*call_ret = _user_kill_thread((thread_id)arg0);
			break;
		case SYSCALL_GET_THREAD_INFO:
			*call_ret = _user_get_thread_info((thread_id)arg0, (thread_info *)arg1);
			break;
		case SYSCALL_GET_NEXT_THREAD_INFO:
			*call_ret = _user_get_next_thread_info((team_id)arg0, (int32 *)arg1, (thread_info *)arg2);
			break;
		case SYSCALL_GET_TEAM_INFO:
			*call_ret = _user_get_team_info((team_id)arg0, (team_info *)arg1);
			break;
		case SYSCALL_GET_NEXT_TEAM_INFO:
			*call_ret = _user_get_next_team_info((int32 *)arg0, (team_info *)arg1);
			break;
		case SYSCALL_SUSPEND_THREAD:
			*call_ret = _user_suspend_thread((thread_id)arg0);
			break;
		case SYSCALL_RESUME_THREAD:
			*call_ret = _user_resume_thread((thread_id)arg0);
			break;
		case SYSCALL_RENAME_THREAD:
			*call_ret = _user_rename_thread((thread_id)arg0, (const char *)arg1);
			break;
		case SYSCALL_SEND_DATA:
			*call_ret = _user_send_data((thread_id)arg0, (int32)arg1, (const void *)arg2, (size_t)arg3);
			break;
		case SYSCALL_RECEIVE_DATA:
			*call_ret = _user_receive_data((thread_id *)arg0, (void *)arg1, (size_t)arg2);
			break;
		case SYSCALL_HAS_DATA:
			*call_ret = _user_has_data((thread_id)arg0);
			break;
		case SYSCALL_KILL_TEAM:
			*call_ret = _user_kill_team((team_id)arg0);
			break;
		case SYSCALL_GET_CURRENT_TEAM_ID:
			*call_ret = _user_get_current_team();
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
			*call_ret = user_set_port_owner((port_id)arg0, (team_id)arg1);
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
			*call_ret = user_get_next_sem_info((team_id)arg0, (uint32 *)arg1, (struct sem_info *)arg2,
			                                   (size_t)arg3);
			break;
		case SYSCALL_SEM_SET_SEM_OWNER:
			*call_ret = user_set_sem_owner((sem_id)arg0, (team_id)arg1);
			break;
/* obsolete; replaced by get_next_team_info
		case SYSCALL_GET_PROC_TABLE:
			*call_ret = user_team_get_table((struct team_info *)arg0, (size_t)arg1);
			break;
*/
		case SYSCALL_GETRLIMIT:
			*call_ret = _user_getrlimit((int)arg0, (struct rlimit *)arg1);
			break;
		case SYSCALL_SETRLIMIT:
			*call_ret = _user_setrlimit((int)arg0, (const struct rlimit *)arg1);
			break;

		// image calls
		case SYSCALL_REGISTER_IMAGE:
			*call_ret = _user_register_image((image_info *)arg0, (size_t)arg1);
			break;
		case SYSCALL_UNREGISTER_IMAGE:
			*call_ret = _user_unregister_image((image_id)arg0);
			break;
		case SYSCALL_GET_IMAGE_INFO:
			*call_ret = _user_get_image_info((image_id)arg0, (image_info *)arg1, (size_t)arg2);
			break;
		case SYSCALL_GET_NEXT_IMAGE_INFO:
			*call_ret = _user_get_next_image_info((team_id)arg0, (int32 *)arg1, (image_info *)arg2, (size_t)arg3);
			break;

		// node monitor calls
		case SYSCALL_START_WATCHING:
			*call_ret = user_start_watching((dev_t)arg0, (ino_t)INT32TOINT64(arg1, arg2), (uint32)arg3, (port_id)arg4, (uint32)arg5);
			break;
		case SYSCALL_STOP_WATCHING:
			*call_ret = user_stop_watching((dev_t)arg0, (ino_t)INT32TOINT64(arg1, arg2), (uint32)arg3, (port_id)arg4, (uint32)arg5);
			break;
		case SYSCALL_STOP_NOTIFYING:
			*call_ret = user_stop_notifying((port_id)arg0, (uint32)arg1);
			break;

		case SYSCALL_SYSCTL:
			*call_ret = user_sysctl((int *)arg0, (uint)arg1, (void *)arg2, 
			                        (size_t *)arg3, (void *)arg4, 
			                        (size_t)arg5);
			break;

		// time calls
		case SYSCALL_SET_REAL_TIME_CLOCK:
			_user_set_real_time_clock((uint32)arg0);
			break;

/* removed until net code has goneback into build
		case SYSCALL_SOCKET:
			*call_ret = socket((int)arg0, (int)arg1, (int)arg2, false);
			break;
 */
		case SYSCALL_SETENV:
			*call_ret = user_setenv((const char *)arg0, (const char *)arg1, (int)arg2);
			break;
		case SYSCALL_GETENV:
			*call_ret = user_getenv((const char *)arg0, (char **)arg1);
			break;
		case SYSCALL_DEBUG_OUTPUT:
			_user_debug_output((const char *)arg0);
			break;

		case SYSCALL_RETURN_FROM_SIGNAL:
			*call_ret = arch_restore_signal_frame();
			break;
		case SYSCALL_SIGACTION:
			*call_ret = user_sigaction((int)arg0, (const struct sigaction *)arg1, (struct sigaction *)arg2);
			break;
		case SYSCALL_SEND_SIGNAL:
			*call_ret = user_send_signal((pid_t)arg0, (uint)arg1);
			break;
		case SYSCALL_SET_ALARM:
			*call_ret = user_set_alarm((bigtime_t)INT32TOINT64(arg0, arg1), (uint32)arg2);
			break;
		case SYSCALL_GET_SYSTEM_INFO:
			*call_ret = _user_get_system_info((system_info *)arg0, (size_t)arg1);
			break;
		// 32 bit atomic functions
#ifdef ATOMIC_FUNCS_ARE_SYSCALLS
		case SYSCALL_ATOMIC_SET:
			*call_ret = _user_atomic_set((vint32 *)arg0, (int32)arg1);
			break;
		case SYSCALL_ATOMIC_TEST_AND_SET:
			*call_ret = _user_atomic_test_and_set((vint32 *)arg0, (int32)arg1, (int32)arg2);
			break;
		case SYSCALL_ATOMIC_ADD:
			*call_ret = _user_atomic_add((vint32 *)arg0, (int32)arg1);
			break;
		case SYSCALL_ATOMIC_AND:
			*call_ret = _user_atomic_and((vint32 *)arg0, (int32)arg1);
			break;
		case SYSCALL_ATOMIC_OR:
			*call_ret = _user_atomic_or((vint32 *)arg0, (int32)arg1);
			break;
		case SYSCALL_ATOMIC_GET:
			*call_ret = _user_atomic_get((vint32 *)arg0);
			break;
#endif

		// 64 bit atomic functions
#ifdef ATOMIC64_FUNCS_ARE_SYSCALLS
		case SYSCALL_ATOMIC_SET64:
			*call_ret = _user_atomic_set64((vint64 *)arg0, INT32TOINT64(arg1, arg2));
			break;
		case SYSCALL_ATOMIC_TEST_AND_SET64:
			*call_ret = _user_atomic_test_and_set64((vint64 *)arg0, INT32TOINT64(arg1, arg2), INT32TOINT64(arg3, arg4));
			break;
		case SYSCALL_ATOMIC_ADD64:
			*call_ret = _user_atomic_add64((vint64 *)arg0, INT32TOINT64(arg1, arg2));
			break;
		case SYSCALL_ATOMIC_AND64:
			*call_ret = _user_atomic_and64((vint64 *)arg0, INT32TOINT64(arg1, arg2));
			break;
		case SYSCALL_ATOMIC_OR64:
			*call_ret = _user_atomic_or64((vint64 *)arg0, INT32TOINT64(arg1, arg2));
			break;
		case SYSCALL_ATOMIC_GET64:
			*call_ret = _user_atomic_get64((vint64 *)arg0);
			break;
#endif

		default:
			*call_ret = -1;
	}

//	dprintf("syscall_dispatcher: done with syscall 0x%x\n", call_num);

	return B_INVOKE_SCHEDULER;
}
