/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <syscalls.h>



ssize_t
wait_for_objects(object_wait_info* infos, int numInfos)
{
	return _kern_wait_for_objects(infos, numInfos, 0, 0);
}


ssize_t
wait_for_objects_etc(object_wait_info* infos, int numInfos, uint32 flags,
	bigtime_t timeout)
{
	return _kern_wait_for_objects(infos, numInfos, flags, timeout);
}
