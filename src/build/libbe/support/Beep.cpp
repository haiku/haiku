/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <Beep.h>
#include <stdio.h>


status_t
system_beep(const char *eventName)
{
	printf("beep event \"%s\" requested.\n", eventName);
	// ToDo: ask media server to beep around
	return B_ERROR;
}


status_t
beep()
{
	return system_beep(NULL);
}


status_t
add_system_beep_event(const char *eventName, uint32 flags _BEEP_FLAGS)
{
	// ToDo: ask media server to add beep event
	return B_ERROR;
}

