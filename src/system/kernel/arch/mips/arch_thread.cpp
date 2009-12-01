/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <kernel/thread.h>

int arch_proc_init_proc_struct(struct proc *p, bool kernel)
{
	return 0;
}

int arch_thread_init_thread_struct(struct thread *t)
{
	return 0;
}

int arch_thread_initialize_kthread_stack(struct thread *t, int (*start_func)(void), void (*entry_func)(void))
{
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
#if 0
	sh4_set_kstack(t_to->kernel_stack_area->base + KSTACK_SIZE);
	
	if(t_from->proc->arch_info.pgdir != t_to->proc->arch_info.pgdir)
		sh4_set_user_pgdir((addr)t_to->proc->arch_info.pgdir);
	sh4_context_switch(&t_from->arch_info.sp, t_to->arch_info.sp);
#endif
}

void arch_thread_dump_info(void *info)
{
}

void arch_thread_enter_uspace(addr entry, addr ustack_top)
{
}

