/*
 * ResizeVisitor.h
 * Copyright (C) 2020 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef RESIZE_VISITOR_H
#define RESIZE_VISITOR_H


#include "FileSystemVisitor.h"


class ResizeVisitor: public FileSystemVisitor {
	public:
					ResizeVisitor(Volume* volume);

		status_t	Resize(off_t size, disk_job_id job);
};


#endif /* !RESIZE_VISITOR_H */
