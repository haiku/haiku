/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <kernel.h>
#include <thread.h>
#include <boot/stage2.h>

#include <string.h>


int
arch_team_init_team_struct(struct team *team, bool kernel)
{
	return 0;
}


int
arch_thread_init_thread_struct(struct thread *t)
{
	// set up an initial state (stack & fpu)
	memset(&t->arch_info, 0, sizeof(t->arch_info));

	return 0;
}


int
arch_thread_init_kthread_stack(struct thread *t, int (*start_func)(void), void (*entry_func)(void), void (*exit_func)(void))
{
	addr_t *kstack = (addr_t *)t->kernel_stack_base;
	size_t kstack_size = KSTACK_SIZE;
	addr_t *kstack_top = kstack + kstack_size / sizeof(addr_t) - 2 * 4;

	// r13-r31, cr, r2
	kstack_top -= (31 - 13 + 1) + 1 + 1;

	// set the saved lr address to be the start_func
	kstack_top--;
	*kstack_top = (addr_t)start_func;

	// save this stack position
	t->arch_info.sp = (void *)kstack_top;

	return 0;
}


// ToDo: we are single proc for now
static struct thread *current_thread = NULL;

struct thread *
arch_thread_get_current_thread(void)
{
	return current_thread;
}


void 
arch_thread_set_current_thread(struct thread *t)
{
	current_thread = t;
}


void
arch_thread_init_tls(struct thread *thread)
{
}


void
arch_thread_context_switch(struct thread *t_from, struct thread *t_to)
{
    // set the new kernel stack in the EAR register.
	// this is used in the exception handler code to decide what kernel stack to 
	// switch to if the exception had happened when the processor was in user mode  
	asm("mtear  %0" :: "g"(t_to->kernel_stack_base + KSTACK_SIZE - 8));

    // switch the asids if we need to
	if (t_to->team->_aspace_id >= 0) {
		// the target thread has is user space
		if (t_from->team->_aspace_id != t_to->team->_aspace_id) {
			// switching to a new address space
			ppc_translation_map_change_asid(&t_to->team->aspace->translation_map);
		}
	}

	ppc_context_switch(&t_from->arch_info.sp, t_to->arch_info.sp);
}


void
arch_thread_dump_info(void *info)
{
	struct arch_thread *at = (struct arch_thread *)info;

	dprintf("\tsp: %p\n", at->sp);
}


void
arch_thread_enter_uspace(struct thread *thread, addr entry, void *arg1, void *arg2)
{
	panic("arch_thread_enter_uspace(): not yet implemented\n");
}


void
arch_setup_signal_frame(struct thread *thread, struct sigaction *sa, int sig, int sigMask)
{
}


int64
arch_restore_signal_frame(void)
{
	return 0;
}


void
arch_check_syscall_restart(struct thread *thread)
{
}


