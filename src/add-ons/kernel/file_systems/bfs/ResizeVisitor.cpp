/*
 * Copyright (C) 2020 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#include "ResizeVisitor.h"


ResizeVisitor::ResizeVisitor(Volume* volume)
	:
	FileSystemVisitor(volume)
{
}


status_t
ResizeVisitor::Resize(off_t size, disk_job_id job)
{
	return B_NOT_SUPPORTED;
}
