/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* This is main - initializes processors and starts init */


#include <OS.h>

#include <boot/kernel_args.h>
#include <console.h>
#include <debug.h>
#include <arch/faults.h>
#include <arch/dbg_console.h>
#include <ksyscalls.h>
#include <vm.h>
#include <timer.h>
#include <smp.h>
#include <sem.h>
#include <port.h>
#include <vfs.h>
#include <cbuf.h>
#include <elf.h>
#include <cpu.h>
#include <kdriver_settings.h>
#include <kmodule.h>
#include <int.h>
#include <team.h>
#include <system_info.h>
#include <kdevice_manager.h>
#include <real_time_clock.h>
#include <kernel_daemon.h>
#include <messaging.h>

#include <string.h>


//#define TRACE_BOOT
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
	thread_id thread = -1;

	kernel_startup = true;

	if (oldka->kernel_args_size != sizeof(kernel_args)
		|| oldka->version != CURRENT_KERNEL_ARGS_VERSION) {
		// This is something we cannot handle right now - release kernels
		// should always be able to handle the kernel_args of earlier
		// released kernels.
		arch_dbg_con_early_boot_message("Version mismatch between boot loader and kernel!\n");
		return -1;
	}

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
		debug_init(&ka);
		set_dprintf_enabled(true);
		dprintf("Welcome to kernel debugger output!\n");

		// init modules
		TRACE(("init CPU\n"));
		cpu_init(&ka);
		TRACE(("init interrupts\n"));
		int_init(&ka);

		TRACE(("init VM\n"));
		vm_init(&ka);
			// Before vm_init_post_sem() is called, we have to make sure that
			// the boot loader allocated region is not used anymore

		// now we can use the heap and create areas
		TRACE(("init driver_settings\n"));
		driver_settings_init(&ka);
		debug_init_post_vm(&ka);
		int_init_post_vm(&ka);
		cpu_init_post_vm(&ka);
		TRACE(("init system info\n"));
		system_info_init(&ka);

		TRACE(("init faults\n"));
		faults_init(&ka);
		TRACE(("init SMP\n"));
		smp_init(&ka);
		TRACE(("init timer\n"));
		timer_init(&ka);
		TRACE(("init real time clock\n"));
		rtc_init(&ka);

		TRACE(("init semaphores\n"));
		sem_init(&ka);

		// now we can create and use semaphores
		TRACE(("init VM semaphores\n"));
		vm_init_post_sem(&ka);
		TRACE(("init driver_settings\n"));
		driver_settings_init_post_sem(&ka);
		TRACE(("init generic syscall\n"));
		generic_syscall_init();
		TRACE(("init cbuf\n"));
		cbuf_init();
		TRACE(("init VFS\n"));
		vfs_init(&ka);
		TRACE(("init teams\n"));
		team_init(&ka);
		TRACE(("init threads\n"));
		thread_init(&ka);
		TRACE(("init ports\n"));
		port_init(&ka);
		TRACE(("init kernel daemons\n"));
		kernel_daemon_init();

		TRACE(("init VM threads\n"));
		vm_init_post_thread(&ka);
		TRACE(("init ELF loader\n"));
		elf_init(&ka);

		// start a thread to finish initializing the rest of the system
		thread = spawn_kernel_thread(&main2, "main2", B_NORMAL_PRIORITY, NULL);

		smp_wake_up_all_non_boot_cpus();
		smp_enable_ici(); // ici's were previously being ignored
		start_scheduler();
	} else {
		// this is run per cpu for each AP processor after they've been set loose
		smp_per_cpu_init(&ka, cpu_num);
		thread_per_cpu_init(cpu_num);
	}

	TRACE(("enable interrupts, exit kernel startup\n"));
	kernel_startup = false;
	enable_interrupts();

	if (thread >= B_OK)
		resume_thread(thread);

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

	// ToDo: the preloaded image debug data is placed in the kernel args, and
	//	thus, if they are enabled, the kernel args shouldn't be freed, so
	//	that we don't have to copy them.
	//	What is yet missing is a mechanism that controls this (via driver settings).
	if (0) {
		// module_init() is supposed to be the last user of the kernel args
		// Note: don't confuse the kernel_args structure (which is never freed)
		// with the kernel args ranges it contains (and which are freed here).
		vm_free_kernel_args(&ka);
	}

	// init the messaging service
	TRACE(("Init Messaging Service\n"));
	init_messaging_service();

	/* bootstrap all the filesystems */
	TRACE(("Bootstrap file systems\n"));
	vfs_bootstrap_file_systems();

	TRACE(("Init Device Manager\n"));
	device_manager_init(&ka);

	// ToDo: device manager starts here, bus_init()/dev_init() won't be necessary anymore,
	//	but instead, the hardware and drivers are rescanned then.

	TRACE(("Mount boot file system\n"));
	vfs_mount_boot_file_system(&ka);

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
		const char *shellArgs[] = {"/bin/sh", "/boot/beos/system/boot/Bootscript", NULL};
		const char *initArgs[] = {"/bin/init", NULL};
		const char **args;
		int32 argc;
		thread_id thread;

		struct stat st;
		if (stat(shellArgs[1], &st) == 0) {
			// start Bootscript
			args = shellArgs;
			argc = 2;
		} else {
			// ToDo: this is only necessary as long as we have the bootdir mechanism
			// start init
			args = initArgs;
			argc = 1;
		}

		thread = load_image(argc, args, NULL);
		if (thread >= B_OK) {
			resume_thread(thread);
			TRACE(("Bootscript started\n"));
		} else
			dprintf("error starting \"%s\" error = %ld \n", args[0], thread);
	}

	return 0;
}

