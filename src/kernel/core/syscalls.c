/* Big case statement for dispatching syscalls */
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
#include <arch_config.h>
#include <disk_device_manager/ddm_userland_interface.h>
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


static inline
int
_user_null()
{
	return 0;
}

// 
static inline
bigtime_t
_user_system_time()
{
	return system_time();
}

// map to the arch specific call
static inline
int64
_user_restore_signal_frame()
{
	return arch_restore_signal_frame();
}

// XXX: _kern_exit() was formerly mapped to _user_exit_thread(). That's
// probably not correct.
static inline
void
_user_exit(int returnCode)
{
	_user_exit_thread(returnCode);
}

// TODO: Replace when networking code is added to the build. 
static inline
int
_user_socket(int family, int type, int proto)
{
	return 0;
}


int
syscall_dispatcher(unsigned long call_num, void *args, uint64 *call_ret)
{
//	dprintf("syscall_dispatcher: thread 0x%x call 0x%x, arg0 0x%x, arg1 0x%x arg2 0x%x arg3 0x%x arg4 0x%x\n",
//		thread_get_current_thread_id(), call_num, arg0, arg1, arg2, arg3, arg4);

	switch (call_num) {
		// the cases are auto-generated
		#include "syscall_dispatcher.h"

		default:
			*call_ret = -1;
	}

//	dprintf("syscall_dispatcher: done with syscall 0x%x\n", call_num);

	return B_INVOKE_SCHEDULER;
}
