/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <kernel/int.h>
#include <kernel/vm_priv.h>
#include <kernel/arch/cpu.h>
#include <nulibc/string.h>

static struct thread *curr_thread = NULL;

int arch_proc_init_proc_struct(struct proc *p, bool kernel)
{
#if 0
	if(!kernel) {
		unsigned int page;
		if(vm_get_free_page(&page) < 0) {
			panic("arch_proc_init_proc_struct: could not find free frame for page dir\n");
		}
		p->arch_info.pgdir = (unsigned int *)PHYS_ADDR_TO_P1(page * PAGE_SIZE);
		memset(p->arch_info.pgdir, 0, PAGE_SIZE);
	} else {
		p->arch_info.pgdir = NULL;
	}
#endif
	return 0;
}

int arch_thread_init_thread_struct(struct thread *t)
{
	t->arch_info.sp = NULL;

	return 0;
}

int arch_thread_initialize_kthread_stack(struct thread *t, int (*start_func)(void), void (*entry_func)(void), void (*exit_func)(void))
{
	unsigned int *kstack = (unsigned int *)t->kernel_stack_base;
	unsigned int kstack_size = KSTACK_SIZE;
	unsigned int *kstack_top = kstack + kstack_size / sizeof(unsigned int);
	int i;

	// clear the kernel stack
	memset(kstack, 0, kstack_size);

	// set the final return address to be thread_kthread_exit
	kstack_top--;
	*kstack_top = (unsigned int)exit_func;

	// set the return address to be the start of the first function
	kstack_top--;
	*kstack_top = (unsigned int)start_func;

	// set the return address to be the start of the entry (thread setup) function
	kstack_top--;
	*kstack_top = (unsigned int)entry_func;

	// simulate the important registers being pushed
	for(i=0; i<7+2+5; i++) {
		kstack_top--;
		*kstack_top = 0;
	}

	// set the address of the function caller function
	// that will in turn call all of the above functions
	// pushed onto the stack
	kstack_top--;
	*kstack_top = (unsigned int)sh4_function_caller;

	// save the sp
	t->arch_info.sp = kstack_top;

	return 0;
}

void arch_thread_context_switch(struct thread *t_from, struct thread *t_to)
{
#if 0
	int i;
	dprintf("arch_thread_context_switch: 0x%x->0x%x to sp 0x%x\n",
		t_from->id, t_to->id, t_to->arch_info.sp);
#endif
#if 0
	for(i=0; i<8; i++) {
		dprintf("sp[%d] = 0x%x\n", i, t_to->arch_info.sp[i]);
	}
#endif
	sh4_set_kstack(t_to->kernel_stack_base + KSTACK_SIZE);

	if(t_to->proc->aspace != NULL) {
		sh4_set_user_pgdir(vm_translation_map_get_pgdir(&t_to->proc->aspace->translation_map));
	} else {
		sh4_set_user_pgdir(NULL);
	}
	sh4_context_switch(&t_from->arch_info.sp, t_to->arch_info.sp);
}

void arch_thread_dump_info(void *info)
{
	struct arch_thread *at = (struct arch_thread *)info;
	dprintf("\tsp: 0x%x\n", at->sp);
}

void arch_thread_enter_uspace(addr entry, void *args, addr ustack_top)
{
    dprintf("arch_thread_entry_uspace: entry 0x%x, ustack_top 0x%x\n",
	        entry, ustack_top);

    int_disable_interrupts();

    sh4_set_kstack(thread_get_current_thread()->kernel_stack_base + KSTACK_SIZE);

    sh4_enter_uspace(entry, args, ustack_top - 4);
	// never get to here
}

void arch_thread_switch_kstack_and_call(struct thread *t, addr new_kstack, void (*func)(void *), void *arg)
{
	sh4_switch_stack_and_call(new_kstack, func, arg);
}

struct thread *arch_thread_get_current_thread(void)
{
	return curr_thread;
}

void arch_thread_set_current_thread(struct thread *t)
{
	curr_thread = t;
}

