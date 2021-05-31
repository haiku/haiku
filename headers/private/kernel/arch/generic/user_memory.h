/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_GENERIC_USER_MEMORY_H
#define _KERNEL_ARCH_GENERIC_USER_MEMORY_H


#include <atomic>

#include <setjmp.h>
#include <string.h>

#include <thread.h>


namespace {


struct FaultHandlerGuard {
	FaultHandlerGuard()
	{
		old_handler = thread_get_current_thread()->fault_handler;
		if (old_handler != nullptr) {
			memcpy(old_handler_state,
				thread_get_current_thread()->fault_handler_state,
				sizeof(jmp_buf));
		}
		thread_get_current_thread()->fault_handler = HandleFault;
		std::atomic_signal_fence(std::memory_order_acq_rel);
	}


	~FaultHandlerGuard()
	{
		std::atomic_signal_fence(std::memory_order_acq_rel);
		thread_get_current_thread()->fault_handler = old_handler;
		if (old_handler != nullptr) {
			memcpy(thread_get_current_thread()->fault_handler_state,
				old_handler_state,
				sizeof(jmp_buf));
		}

	}


	[[noreturn]] static void HandleFault()
	{
		longjmp(thread_get_current_thread()->fault_handler_state, 1);
	}

	void 		(*old_handler)(void);
	jmp_buf		old_handler_state;
};


template<typename Function>
bool user_access(Function function)
{
	FaultHandlerGuard guard;
	// TODO: try { } catch (...) { } would be much nicer, wouldn't it?
	// And faster... And world wouldn't end in a terrible disaster if function()
	// or anything it calls created on stack an object with non-trivial
	// destructor.
	auto fail = setjmp(thread_get_current_thread()->fault_handler_state);
	if (fail == 0) {
		set_ac();
		function();
		clear_ac();
		return true;
	}
	clear_ac();
	return false;
}


inline status_t
arch_cpu_user_memcpy(void* src, const void* dst, size_t n)
{
	return user_access([=] { memcpy(src, dst, n); }) ? B_OK : B_ERROR;
}


inline status_t
arch_cpu_user_memset(void* src, char v, size_t n)
{
	return user_access([=] { memset(src, v, n); }) ? B_OK : B_ERROR;
}


inline ssize_t
arch_cpu_user_strlcpy(char* src, const char* dst, size_t n)
{
	ssize_t result;
	return user_access([=, &result] { result = strlcpy(src, dst, n); })
		? result : B_ERROR;
}

}

#endif	// _KERNEL_ARCH_GENERIC_USER_MEMORY_H

