/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* This is main - initializes processors and starts init */


#include <OS.h>

#include <arch/platform.h>
#include <boot_item.h>
#include <cbuf.h>
#include <cpu.h>
#include <debug.h>
#include <elf.h>
#include <int.h>
#include <kdevice_manager.h>
#include <kdriver_settings.h>
#include <kernel_daemon.h>
#include <kmodule.h>
#include <kscheduler.h>
#include <ksyscalls.h>
#include <messaging.h>
#include <port.h>
#include <real_time_clock.h>
#include <sem.h>
#include <smp.h>
#include <system_info.h>
#include <team.h>
#include <timer.h>
#include <user_debugger.h>
#include <vfs.h>
#include <vm.h>
#include <boot/kernel_args.h>

#include <string.h>


#define TRACE_BOOT
#ifdef TRACE_BOOT
#	define TRACE(x...) dprintf("INIT : " x)
#else
#	define TRACE(x...) ;
#endif

bool kernel_startup = true;

static kernel_args sKernelArgs;
static uint32 sCpuRendezvous;
static uint32 sCpuRendezvous2;

static int32 main2(void *);
int _start(kernel_args *bootKernelArgs, int cpu);	/* keep compiler happy */

int
_start(kernel_args *bootKernelArgs, int currentCPU)
{
	if (bootKernelArgs->kernel_args_size != sizeof(kernel_args)
		|| bootKernelArgs->version != CURRENT_KERNEL_ARGS_VERSION) {
		// This is something we cannot handle right now - release kernels
		// should always be able to handle the kernel_args of earlier
		// released kernels.
		debug_early_boot_message("Version mismatch between boot loader and kernel!\n");
		return -1;
	}

	smp_set_num_cpus(bootKernelArgs->num_cpus);

	// wait for all the cpus to get here
	smp_cpu_rendezvous(&sCpuRendezvous, currentCPU);

	// the passed in kernel args are in a non-allocated range of memory
	if (currentCPU == 0)
		memcpy(&sKernelArgs, bootKernelArgs, sizeof(kernel_args));

	smp_cpu_rendezvous(&sCpuRendezvous2, currentCPU);

	// do any pre-booting cpu config
	cpu_preboot_init_percpu(&sKernelArgs, currentCPU);
	thread_preboot_init_percpu(&sKernelArgs, currentCPU);

	// if we're not a boot cpu, spin here until someone wakes us up
	if (smp_trap_non_boot_cpus(currentCPU)) {
		thread_id thread;

		// init platform
		arch_platform_init(&sKernelArgs);

		// setup debug output
		debug_init(&sKernelArgs);
		set_dprintf_enabled(true);
		dprintf("Welcome to kernel debugger output!\n");

		// init modules
		TRACE("init CPU\n");
		cpu_init(&sKernelArgs);
		cpu_init_percpu(&sKernelArgs, currentCPU);
		TRACE("init interrupts\n");
		int_init(&sKernelArgs);

		TRACE("init VM\n");
		vm_init(&sKernelArgs);
			// Before vm_init_post_sem() is called, we have to make sure that
			// the boot loader allocated region is not used anymore

		// now we can use the heap and create areas
		arch_platform_init_post_vm(&sKernelArgs);
		TRACE("init driver_settings\n");
		boot_item_init();
		driver_settings_init(&sKernelArgs);
		debug_init_post_vm(&sKernelArgs);
		int_init_post_vm(&sKernelArgs);
		cpu_init_post_vm(&sKernelArgs);
		TRACE("init system info\n");
		system_info_init(&sKernelArgs);

		TRACE("init SMP\n");
		smp_init(&sKernelArgs);
		TRACE("init timer\n");
		timer_init(&sKernelArgs);
		TRACE("init real time clock\n");
		rtc_init(&sKernelArgs);

		TRACE("init semaphores\n");
		sem_init(&sKernelArgs);

		// now we can create and use semaphores
		TRACE("init VM semaphores\n");
		vm_init_post_sem(&sKernelArgs);
		TRACE("init driver_settings\n");
		driver_settings_init_post_sem(&sKernelArgs);
		TRACE("init generic syscall\n");
		generic_syscall_init();
		TRACE("init cbuf\n");
		cbuf_init();
		TRACE("init teams\n");
		team_init(&sKernelArgs);
		TRACE("init threads\n");
		thread_init(&sKernelArgs);
		TRACE("init ports\n");
		port_init(&sKernelArgs);
		TRACE("init kernel daemons\n");
		kernel_daemon_init();
		arch_platform_init_post_thread(&sKernelArgs);

		TRACE("init VM threads\n");
		vm_init_post_thread(&sKernelArgs);
		TRACE("init ELF loader\n");
		elf_init(&sKernelArgs);
		TRACE("init scheduler\n");
		scheduler_init();
		TRACE("init VFS\n");
		vfs_init(&sKernelArgs);

		// bring up the AP cpus in a lock step fashion
		TRACE("waking up AP cpus\n");
		sCpuRendezvous = sCpuRendezvous2 = 0;
		smp_wake_up_non_boot_cpus();
		smp_cpu_rendezvous(&sCpuRendezvous, 0); // wait until they're booted

		// exit the kernel startup phase (mutexes, etc work from now on out)
		TRACE("exiting kernel startup\n");
		kernel_startup = false;

		smp_cpu_rendezvous(&sCpuRendezvous2, 0); // release the AP cpus to go enter the scheduler

		TRACE("enabling interrupts and starting scheduler on cpu 0\n");
		enable_interrupts();
		scheduler_start();

		// start a thread to finish initializing the rest of the system
		TRACE("starting main2 thread\n");
		thread = spawn_kernel_thread(&main2, "main2", B_NORMAL_PRIORITY, NULL);
		TRACE("resuming main2 thread...\n");
		resume_thread(thread);
	} else {
		// lets make sure we're in sync with the main cpu
		// the boot processor has probably been sending us 
		// tlb sync messages all along the way, but we've 
		// been ignoring them
		arch_cpu_global_TLB_invalidate();
	
		// this is run for each non boot processor after they've been set loose
		cpu_init_percpu(&sKernelArgs, currentCPU);
		smp_per_cpu_init(&sKernelArgs, currentCPU);

		// wait for all other AP cpus to get to this point
		smp_cpu_rendezvous(&sCpuRendezvous, currentCPU);
		smp_cpu_rendezvous(&sCpuRendezvous2, currentCPU);

		// welcome to the machine
		enable_interrupts();
		scheduler_start();
	}

	TRACE("main: done... begin idle loop on cpu %d\n", currentCPU);
	for (;;)
		arch_cpu_idle();

	return 0;
}


static int32
main2(void *unused)
{
	(void)(unused);

	TRACE("start of main2: initializing devices\n");

	TRACE("Init modules\n");
	module_init(&sKernelArgs);

	// ToDo: the preloaded image debug data is placed in the kernel args, and
	//	thus, if they are enabled, the kernel args shouldn't be freed, so
	//	that we don't have to copy them.
	//	What is yet missing is a mechanism that controls this (via driver settings).
	if (0) {
		// module_init() is supposed to be the last user of the kernel args
		// Note: don't confuse the kernel_args structure (which is never freed)
		// with the kernel args ranges it contains (and which are freed here).
		vm_free_kernel_args(&sKernelArgs);
	}

	// init userland debugging
	TRACE("Init Userland debugging\n");
	init_user_debug();

	// init the messaging service
	TRACE("Init Messaging Service\n");
	init_messaging_service();

	/* bootstrap all the filesystems */
	TRACE("Bootstrap file systems\n");
	vfs_bootstrap_file_systems();

	TRACE("Init Device Manager\n");
	device_manager_init(&sKernelArgs);

	// ToDo: device manager starts here, bus_init()/dev_init() won't be necessary anymore,
	//	but instead, the hardware and drivers are rescanned then.

	int_init_post_device_manager(&sKernelArgs);

	TRACE("Mount boot file system\n");
	vfs_mount_boot_file_system(&sKernelArgs);

	// CPU specific modules may now be available
	cpu_init_post_modules(&sKernelArgs);
	vm_init_post_modules(&sKernelArgs);
	debug_init_post_modules(&sKernelArgs);
	device_manager_init_post_modules(&sKernelArgs);

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
			TRACE("Bootscript started\n");
		} else
			dprintf("error starting \"%s\" error = %ld \n", args[0], thread);
	}

	return 0;
}

