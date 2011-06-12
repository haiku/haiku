/*
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! This is main - initializes the kernel and launches the Bootscript */


#include <string.h>

#include <FindDirectory.h>
#include <OS.h>

#include <arch/platform.h>
#include <boot_device.h>
#include <boot_item.h>
#include <boot_splash.h>
#include <commpage.h>
#include <condition_variable.h>
#include <cpu.h>
#include <debug.h>
#include <DPC.h>
#include <elf.h>
#include <fs/devfs.h>
#include <fs/KPath.h>
#include <int.h>
#include <kdevice_manager.h>
#include <kdriver_settings.h>
#include <kernel_daemon.h>
#include <kmodule.h>
#include <kscheduler.h>
#include <ksyscalls.h>
#include <ksystem_info.h>
#include <lock.h>
#include <low_resource_manager.h>
#include <messaging.h>
#include <Notifications.h>
#include <port.h>
#include <posix/realtime_sem.h>
#include <posix/xsi_message_queue.h>
#include <posix/xsi_semaphore.h>
#include <real_time_clock.h>
#include <sem.h>
#include <smp.h>
#include <team.h>
#include <timer.h>
#include <user_debugger.h>
#include <user_mutex.h>
#include <vfs.h>
#include <vm/vm.h>
#include <boot/kernel_args.h>

#include "vm/VMAnonymousCache.h"


//#define TRACE_BOOT
#ifdef TRACE_BOOT
#	define TRACE(x...) dprintf("INIT: " x)
#else
#	define TRACE(x...) ;
#endif

bool gKernelStartup = true;
bool gKernelShutdown = false;

static kernel_args sKernelArgs;
static uint32 sCpuRendezvous;
static uint32 sCpuRendezvous2;
static uint32 sCpuRendezvous3;

static int32 main2(void *);


extern "C" int
_start(kernel_args *bootKernelArgs, int currentCPU)
{
	if (bootKernelArgs->kernel_args_size != sizeof(kernel_args)
		|| bootKernelArgs->version != CURRENT_KERNEL_ARGS_VERSION) {
		// This is something we cannot handle right now - release kernels
		// should always be able to handle the kernel_args of earlier
		// released kernels.
		debug_early_boot_message("Version mismatch between boot loader and "
			"kernel!\n");
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
	if (smp_trap_non_boot_cpus(currentCPU, &sCpuRendezvous3)) {
		// init platform
		arch_platform_init(&sKernelArgs);

		// setup debug output
		debug_init(&sKernelArgs);
		set_dprintf_enabled(true);
		dprintf("Welcome to kernel debugger output!\n");
		dprintf("Haiku revision: %s\n", get_haiku_revision());

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
		boot_item_init();
		debug_init_post_vm(&sKernelArgs);
		low_resource_manager_init();

		// now we can use the heap and create areas
		arch_platform_init_post_vm(&sKernelArgs);
		lock_debug_init();
		TRACE("init driver_settings\n");
		driver_settings_init(&sKernelArgs);
		debug_init_post_settings(&sKernelArgs);
		TRACE("init notification services\n");
		notifications_init();
		TRACE("init teams\n");
		team_init(&sKernelArgs);
		TRACE("init ELF loader\n");
		elf_init(&sKernelArgs);
		TRACE("init modules\n");
		module_init(&sKernelArgs);
		TRACE("init semaphores\n");
		haiku_sem_init(&sKernelArgs);
		TRACE("init interrupts post vm\n");
		int_init_post_vm(&sKernelArgs);
		cpu_init_post_vm(&sKernelArgs);
		commpage_init();
		TRACE("init system info\n");
		system_info_init(&sKernelArgs);

		TRACE("init SMP\n");
		smp_init(&sKernelArgs);
		TRACE("init timer\n");
		timer_init(&sKernelArgs);
		TRACE("init real time clock\n");
		rtc_init(&sKernelArgs);
		timer_init_post_rtc();

		TRACE("init condition variables\n");
		condition_variable_init();

		// now we can create and use semaphores
		TRACE("init VM semaphores\n");
		vm_init_post_sem(&sKernelArgs);
		TRACE("init generic syscall\n");
		generic_syscall_init();
		smp_init_post_generic_syscalls();
		TRACE("init scheduler\n");
		scheduler_init();
		TRACE("init threads\n");
		thread_init(&sKernelArgs);
		TRACE("init kernel daemons\n");
		kernel_daemon_init();
		arch_platform_init_post_thread(&sKernelArgs);

		TRACE("init I/O interrupts\n");
		int_init_io(&sKernelArgs);
		TRACE("init VM threads\n");
		vm_init_post_thread(&sKernelArgs);
		low_resource_manager_init_post_thread();
		TRACE("init DPC\n");
		dpc_init();
		TRACE("init VFS\n");
		vfs_init(&sKernelArgs);
#if ENABLE_SWAP_SUPPORT
		TRACE("init swap support\n");
		swap_init();
#endif
		TRACE("init POSIX semaphores\n");
		realtime_sem_init();
		xsi_sem_init();
		xsi_msg_init();

		// Start a thread to finish initializing the rest of the system. Note,
		// it won't be scheduled before calling scheduler_start() (on any CPU).
		TRACE("spawning main2 thread\n");
		thread_id thread = spawn_kernel_thread(&main2, "main2",
			B_NORMAL_PRIORITY, NULL);
		resume_thread(thread);

		// We're ready to start the scheduler and enable interrupts on all CPUs.
		scheduler_enable_scheduling();

		// bring up the AP cpus in a lock step fashion
		TRACE("waking up AP cpus\n");
		sCpuRendezvous = sCpuRendezvous2 = 0;
		smp_wake_up_non_boot_cpus();
		smp_cpu_rendezvous(&sCpuRendezvous, 0); // wait until they're booted

		// exit the kernel startup phase (mutexes, etc work from now on out)
		TRACE("exiting kernel startup\n");
		gKernelStartup = false;

		smp_cpu_rendezvous(&sCpuRendezvous2, 0);
			// release the AP cpus to go enter the scheduler

		TRACE("starting scheduler on cpu 0 and enabling interrupts\n");
		scheduler_start();
		enable_interrupts();
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
		scheduler_start();
		enable_interrupts();
	}

#ifdef TRACE_BOOT
	// We disable interrupts for this dprintf(), since otherwise dprintf()
	// would acquires a mutex, which is something we must not do in an idle
	// thread, or otherwise the scheduler would be seriously unhappy.
	disable_interrupts();
	TRACE("main: done... begin idle loop on cpu %d\n", currentCPU);
	enable_interrupts();
#endif

	for (;;)
		arch_cpu_idle();

	return 0;
}


static int32
main2(void *unused)
{
	(void)(unused);

	TRACE("start of main2: initializing devices\n");

	boot_splash_init(sKernelArgs.boot_splash);

	commpage_init_post_cpus();

	TRACE("init ports\n");
	port_init(&sKernelArgs);

	TRACE("init user mutex\n");
	user_mutex_init();

	TRACE("init system notifications\n");
	system_notifications_init();

	TRACE("Init modules\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_1_INIT_MODULES);
	module_init_post_threads();

	// init userland debugging
	TRACE("Init Userland debugging\n");
	init_user_debug();

	// init the messaging service
	TRACE("Init Messaging Service\n");
	init_messaging_service();

	/* bootstrap all the filesystems */
	TRACE("Bootstrap file systems\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_2_BOOTSTRAP_FS);
	vfs_bootstrap_file_systems();

	TRACE("Init Device Manager\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_3_INIT_DEVICES);
	device_manager_init(&sKernelArgs);

	TRACE("Add preloaded old-style drivers\n");
	legacy_driver_add_preloaded(&sKernelArgs);

	int_init_post_device_manager(&sKernelArgs);

	TRACE("Mount boot file system\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_4_MOUNT_BOOT_FS);
	vfs_mount_boot_file_system(&sKernelArgs);

#if ENABLE_SWAP_SUPPORT
	TRACE("swap_init_post_modules\n");
	swap_init_post_modules();
#endif

	// CPU specific modules may now be available
	boot_splash_set_stage(BOOT_SPLASH_STAGE_5_INIT_CPU_MODULES);
	cpu_init_post_modules(&sKernelArgs);

	TRACE("vm_init_post_modules\n");
	boot_splash_set_stage(BOOT_SPLASH_STAGE_6_INIT_VM_MODULES);
	vm_init_post_modules(&sKernelArgs);

	TRACE("debug_init_post_modules\n");
	debug_init_post_modules(&sKernelArgs);

	TRACE("device_manager_init_post_modules\n");
	device_manager_init_post_modules(&sKernelArgs);

	boot_splash_set_stage(BOOT_SPLASH_STAGE_7_RUN_BOOT_SCRIPT);
	boot_splash_uninit();
		// NOTE: We could introduce a syscall to draw more icons indicating
		// stages in the boot script itself. Then we should not free the image.
		// In that case we should copy it over to the kernel heap, so that we
		// can still free the kernel args.

	// The boot splash screen is the last user of the kernel args.
	// Note: don't confuse the kernel_args structure (which is never freed)
	// with the kernel args ranges it contains (and which are freed here).
	vm_free_kernel_args(&sKernelArgs);

	// start the init process
	{
		KPath bootScriptPath;
		status_t status = find_directory(B_BEOS_SYSTEM_DIRECTORY, gBootDevice,
			false, bootScriptPath.LockBuffer(), bootScriptPath.BufferSize());
		if (status != B_OK)
			dprintf("main2: find_directory() failed: %s\n", strerror(status));
		bootScriptPath.UnlockBuffer();
		status = bootScriptPath.Append("boot/Bootscript");
		if (status != B_OK) {
			dprintf("main2: constructing path to Bootscript failed: "
				"%s\n", strerror(status));
		}

		const char *args[] = { "/bin/sh", bootScriptPath.Path(), NULL };
		int32 argc = 2;
		thread_id thread;

		thread = load_image(argc, args, NULL);
		if (thread >= B_OK) {
			resume_thread(thread);
			TRACE("Bootscript started\n");
		} else
			dprintf("error starting \"%s\" error = %ld \n", args[0], thread);
	}

	return 0;
}

