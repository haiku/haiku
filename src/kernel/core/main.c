/* This is main - initializes processors and starts init */

/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <stage2.h>
#include <Errors.h>
#include <kernel.h>
#include <console.h>
#include <debug.h>
#include <faults.h>
#include <arch/int.h>
#include <vm.h>
#include <timer.h>
#include <smp.h>
#include <OS.h>
#include <sem.h>
#include <port.h>
#include <vfs.h>
#include <dev.h>
#include <cbuf.h>
#include <elf.h>
#include <cpu.h>
#include <beos.h>
#include <devs.h>
#include <bus.h>
#include <module.h>
#include <int.h>

#include <string.h>

#include <arch/cpu.h>
#include <arch/faults.h>

static kernel_args ka;

static int main2(void *);

int net_init(kernel_args *ka);

int _start(kernel_args *oldka, int cpu);	/* keep compiler happy */
int _start(kernel_args *oldka, int cpu_num)
{
	memcpy(&ka, oldka, sizeof(kernel_args));

	smp_set_num_cpus(ka.num_cpus);

	// do any pre-booting cpu config
	cpu_preboot_init(&ka);

	// if we're not a boot cpu, spin here until someone wakes us up
	if(smp_trap_non_boot_cpus(&ka, cpu_num) == B_NO_ERROR) {
		// we're the boot processor, so wait for all of the APs to enter the kernel
		smp_wait_for_ap_cpus(&ka);

		// setup debug output
		dbg_init(&ka);
		dbg_set_serial_debug(true);
		dprintf("Welcome to kernel debugger output!\n");

		// init modules
		cpu_init(&ka);
		int_init(&ka);

		vm_init(&ka);
		dprintf("vm up\n");

		// now we can use the heap and create areas
		dbg_init2(&ka);
		int_init2(&ka);

		faults_init(&ka);
		smp_init(&ka);
		timer_init(&ka);

		arch_cpu_init2(&ka);

		sem_init(&ka);

		// now we can create and use semaphores
		vm_init_postsem(&ka);
		cbuf_init();
		vfs_init(&ka);
		thread_init(&ka);
		port_init(&ka);

		vm_init_postthread(&ka);
		elf_init(&ka);

		// start a thread to finish initializing the rest of the system
		{
			thread_id tid;
			tid = thread_create_kernel_thread("main2", &main2, NULL);
			thread_resume_thread(tid);
		}

		smp_wake_up_all_non_boot_cpus();
		smp_enable_ici(); // ici's were previously being ignored
		thread_start_threading();
	} else {
		// this is run per cpu for each AP processor after they've been set loose
		thread_init_percpu(cpu_num);
	}
	int_enable_interrupts();

	dprintf("main: done... begin idle loop on cpu %d\n", cpu_num);
	for(;;);

	return 0;
}

static int main2(void *unused)
{
//	int err;

	(void)(unused);

	dprintf("start of main2: initializing devices\n");

	// bootstrap all the filesystems
	vfs_bootstrap_all_filesystems();

	//net_init(&ka);
	dev_init(&ka);
	module_init(&ka, NULL);
	bus_init(&ka);
	devs_init(&ka);
	con_init(&ka);
	net_init(&ka);
	
	//net_init_postdev(&ka);

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
		proc_id pid;
		pid = proc_create_proc("/boot/bin/init", "init", NULL, 0, 5);
		if(pid < 0)
			kprintf("error starting 'init' error = %d \n",pid);
	}

	return 0;
}

