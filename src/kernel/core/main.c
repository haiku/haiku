/* This is main - initializes processors and starts init */

/*
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <OS.h>

#include <boot/kernel_args.h>
#include <console.h>
#include <debug.h>
#include <arch/faults.h>
#include <arch/int.h>
#include <arch/cpu.h>
#include <vm.h>
#include <timer.h>
#include <smp.h>
#include <sem.h>
#include <port.h>
#include <vfs.h>
#include <dev.h>
#include <cbuf.h>
#include <elf.h>
#include <cpu.h>
#include <devs.h>
#include <bus.h>
#include <kmodule.h>
#include <int.h>
#include <team.h>
#include <kdevice_manager.h>
#include <real_time_clock.h>
#include <kernel_daemon.h>

#include <string.h>


#define TRACE_BOOT
#ifdef TRACE_BOOT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

bool kernel_startup;

static kernel_args ka;

static int32 main2(void *);
int _start(kernel_args *oldka, int cpu);	/* keep compiler happy */


int
_start(kernel_args *oldka, int cpu_num)
{
	kernel_startup = true;

	memcpy(&ka, oldka, sizeof(kernel_args));
		// the passed in kernel args are in a non-allocated range of memory

	smp_set_num_cpus(ka.num_cpus);

	// do any pre-booting cpu config
	cpu_preboot_init(&ka);

	// if we're not a boot cpu, spin here until someone wakes us up
	if (smp_trap_non_boot_cpus(&ka, cpu_num) == B_NO_ERROR) {
		// we're the boot processor, so wait for all of the APs to enter the kernel
		smp_wait_for_ap_cpus(&ka);

		// setup debug output
		dbg_init(&ka);
		set_dprintf_enabled(true);
		dprintf("Welcome to kernel debugger output!\n");

		// init modules
		cpu_init(&ka);
		int_init(&ka);

		vm_init(&ka);
		TRACE(("vm up\n"));

		// now we can use the heap and create areas
		dbg_init2(&ka);
		int_init2(&ka);

		faults_init(&ka);
		smp_init(&ka);
		rtc_init(&ka);
		timer_init(&ka);

		arch_cpu_init2(&ka);

		sem_init(&ka);

		TRACE(("##################################################################\n"));
		TRACE(("semaphores now available\n"));
		TRACE(("##################################################################\n"));

		// now we can create and use semaphores
		vm_init_postsem(&ka);
		cbuf_init();
		vfs_init(&ka);
		team_init(&ka);
		thread_init(&ka);
		port_init(&ka);
		kernel_daemon_init();

		vm_init_postthread(&ka);
		elf_init(&ka);

		// start a thread to finish initializing the rest of the system
		{
			thread_id thread = spawn_kernel_thread(&main2, "main2", B_NORMAL_PRIORITY, NULL);
			resume_thread(thread);
		}

		smp_wake_up_all_non_boot_cpus();
		smp_enable_ici(); // ici's were previously being ignored
		start_scheduler();
	} else {
		// this is run per cpu for each AP processor after they've been set loose
		smp_per_cpu_init(&ka, cpu_num);
		thread_per_cpu_init(cpu_num);
	}
	TRACE(("##################################################################\n"));
	TRACE(("interrupts now enabled\n"));
	TRACE(("##################################################################\n"));

	kernel_startup = false;
	enable_interrupts();

	TRACE(("main: done... begin idle loop on cpu %d\n", cpu_num));
	for (;;)
		arch_cpu_idle();

	return 0;
}


static int32
main2(void *unused)
{
	(void)(unused);

	TRACE(("start of main2: initializing devices\n"));

	TRACE(("Init modules\n"));
	module_init(&ka);

	/* bootstrap all the filesystems */
	TRACE(("Bootstrap file systems\n"));
	vfs_bootstrap_file_systems();

	TRACE(("Init Device Manager\n"));
	device_manager_init(&ka);

	// ToDo: device manager starts here, bus_init()/dev_init() won't be necessary anymore,
	//	but instead, the hardware and drivers are rescanned then.

	TRACE(("Mount boot file system\n"));
	vfs_mount_boot_file_system();

	TRACE(("Init busses\n"));
	bus_init(&ka);

	TRACE(("Init devices\n"));
	dev_init(&ka);

	TRACE(("Init console\n"));
	con_init(&ka);

	//net_init_postdev(&ka);

	//module_test();
#if 0
		// XXX remove
		vfs_test();
#endif
#if 0
		// XXX remove
		thread_test();
#endif
#if 0
		vm_test();
#endif
#if 0
	panic("debugger_test\n");
#endif
#if 0
	cbuf_test();
#endif
#if 0
	port_test();
#endif
	// start the init process
	{
		team_id pid;
		pid = team_create_team("/bin/init", "init", NULL, 0, NULL, 0, B_NORMAL_PRIORITY);

		TRACE(("Init started\n"));
		if (pid < 0)
			kprintf("error starting 'init' error = %ld \n", pid);
	}

	return 0;
}

