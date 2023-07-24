/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <sys/cdefs.h>

#include <SupportDefs.h>

#include <util/Random.h>


extern "C" {

long __stack_chk_guard;


void
__stack_chk_fail()
{
	panic("stack smashing detected\n");
}


}


void
stack_protector_init()
{
	__stack_chk_guard = secure_get_random<long>();
}

