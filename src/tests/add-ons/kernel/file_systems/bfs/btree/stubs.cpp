/*
 * Copyright 2014-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <fs_interface.h>
#include <kscheduler.h>


status_t
acquire_vnode(fs_volume* volume, ino_t vnodeID)
{
	return B_OK;
}


status_t
put_vnode(fs_volume* volume, ino_t vnodeID)
{
	return B_OK;
}


// #pragma mark -


void
scheduler_enqueue_in_run_queue(Thread* thread)
{
}


void
scheduler_reschedule(int32 next_state)
{
}
