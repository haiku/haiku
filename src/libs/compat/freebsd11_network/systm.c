/*
 * Copyright 2012, Jérôme Duval, korli@users.berlios.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <compat/sys/systm.h>
#include <compat/sys/kernel.h>


int
_pause(const char* waitMessage, int timeout)
{
	KASSERT(timeout != 0, ("pause: timeout required"));
	return snooze(ticks_to_usecs(timeout));
}


void
freeenv(char *env)
{
}


char *
getenv(const char *name)
{
	return NULL;
}


char *
kern_getenv(const char *name)
{
	return NULL;
}

