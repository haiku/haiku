/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <stage2.h>
#include <debug.h>
#include <vm.h>
#include <memheap.h>
#include <thread.h>
#include <arch/thread.h>
#include <int.h>
#include <string.h>

int arch_team_init_team_struct(struct team *p, bool kernel)
{
	return 0;
}

// from arch_interrupts.S
extern void	i386_stack_init( struct farcall *interrupt_stack_offset );

int arch_thread_init_thread_struct(struct thread *t)
{
	// set up an initial state (stack & fpu)
	memset(&t->arch_info, 0, sizeof(t->arch_info));

	// let the asm function know the offset to the interrupt stack within struct thread
	// I know no better ( = static) way to tell the asm function the offset
	i386_stack_init( &((struct thread*)0)->arch_info.interrupt_stack );

	return 0;
}

int arch_thread_initialize_kthread_stack(struct thread *t, int (*start_func)(void), void (*entry_func)(void), void (*exit_func)(void))
{
	unsigned int *kstack = (unsigned int *)t->kernel_stack_base;
	unsigned int kstack_size = KSTACK_SIZE;
	unsigned int *kstack_top = kstack + kstack_size / sizeof(unsigned int);
	int i;

//	dprintf("arch_thread_initialize_kthread_stack: kstack 0x%p, start_func 0x%p, entry_func 0x%p\n",
//		kstack, start_func, entry_func);

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

	// simulate pushfl
//	kstack_top--;
//	*kstack_top = 0x00; // interrupts still disabled after the switch

	// simulate initial popad
	for(i=0; i<8; i++) {
		kstack_top--;
		*kstack_top = 0;
	}

	// save the stack position
	t->arch_info.current_stack.esp = kstack_top;
	t->arch_info.current_stack.ss = (int *)KERNEL_DATA_SEG;

	return 0;
}

void arch_thread_switch_kstack_and_call(struct thread *t, addr new_kstack, void (*func)(void *), void *arg)
{
	i386_switch_stack_and_call(new_kstack, func, arg);
}

void arch_thread_context_switch(struct thread *t_from, struct thread *t_to)
{
	addr new_pgdir;
#if 0
	int i;

	dprintf("arch_thread_context_switch: cpu %d 0x%x -> 0x%x, aspace 0x%x -> 0x%x, old stack = 0x%x:0x%x, stack = 0x%x:0x%x\n",
		smp_get_current_cpu(), t_from->id, t_to->id,
		t_from->team->aspace, t_to->team->aspace,
		t_from->arch_info.current_stack.ss, t_from->arch_info.current_stack.esp,
		t_to->arch_info.current_stack.ss, t_to->arch_info.current_stack.esp);
#endif
#if 0
	for(i=0; i<11; i++)
		dprintf("*esp[%d] (0x%x) = 0x%x\n", i, ((unsigned int *)new_at->esp + i), *((unsigned int *)new_at->esp + i));
#endif
	i386_set_kstack(t_to->kernel_stack_base + KSTACK_SIZE);

#if 0
{
	int a = *(int *)(t_to->kernel_stack_base + KSTACK_SIZE - 4);
}
#endif

	if(t_from->team->_aspace_id >= 0 && t_to->team->_aspace_id >= 0) {
		// they are both uspace threads
		if(t_from->team->_aspace_id == t_to->team->_aspace_id) {
			// dont change the pgdir, same address space
			new_pgdir = NULL;
		} else {
			// switching to a new address space
			new_pgdir = vm_translation_map_get_pgdir(&t_to->team->aspace->translation_map);
		}
	} else if(t_from->team->_aspace_id < 0 && t_to->team->_aspace_id < 0) {
		// they must both be kspace threads
		new_pgdir = NULL;
	} else if(t_to->team->_aspace_id < 0) {
		// the one we're switching to is kspace
		new_pgdir = vm_translation_map_get_pgdir(&t_to->team->kaspace->translation_map);
	} else {
		new_pgdir = vm_translation_map_get_pgdir(&t_to->team->aspace->translation_map);
	}
#if 0
	dprintf("new_pgdir is 0x%x\n", new_pgdir);
#endif

#if 0
{
	int a = *(int *)(t_to->arch_info.current_stack.esp - 4);
}
#endif

	if((new_pgdir % PAGE_SIZE) != 0)
		panic("arch_thread_context_switch: bad pgdir 0x%lx\n", new_pgdir);

	i386_fsave_swap(t_from->arch_info.fpu_state, t_to->arch_info.fpu_state);
	i386_context_switch(&t_from->arch_info, &t_to->arch_info, new_pgdir);
}

void arch_thread_dump_info(void *info)
{
	struct arch_thread *at = (struct arch_thread *)info;

	dprintf("\tesp: %p\n", at->current_stack.esp);
	dprintf("\tss: %p\n", at->current_stack.ss);
	dprintf("\tfpu_state at %p\n", at->fpu_state);
}

void arch_thread_enter_uspace(addr entry, void *args, addr ustack_top)
{
	dprintf("arch_thread_entry_uspace: entry 0x%lx, args %p, ustack_top 0x%lx\n",
		entry, args, ustack_top);

	// make sure the fpu is in a good state
	asm("fninit");

	disable_interrupts();

	i386_set_kstack(thread_get_current_thread()->kernel_stack_base + KSTACK_SIZE);

	i386_enter_uspace(entry, args, ustack_top - 4);
}

