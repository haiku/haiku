/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

// Syscalls not existing under R5

#include <syscalls.h>

// _kern_register_messaging_service
area_id
_kern_register_messaging_service(sem_id lockSem, sem_id counterSem)
{
	return B_ERROR;
}

// _kern_unregister_messaging_service
status_t
_kern_unregister_messaging_service()
{
	return B_ERROR;
}

