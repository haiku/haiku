/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <stdint.h>

//#include <x86intrin.h>
#define _X86INTRIN_H_INCLUDED
#include <ia32intrin.h>



static uint64_t cv_factor;
static uint64_t cv_factor_nsec;


extern "C" void
__x86_setup_system_time(uint64_t cv, uint64_t cv_nsec)
{
	cv_factor = cv;
	cv_factor_nsec = cv_nsec;
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
system_time()
{
	__uint128_t time = static_cast<__uint128_t>(__rdtsc()) * cv_factor;
	return time >> 64;
}


extern "C" [[gnu::optimize("omit-frame-pointer")]] int64_t
system_time_nsecs()
{
	__uint128_t t = static_cast<__uint128_t>(__rdtsc()) * cv_factor_nsec;
	return t >> 32;
}

