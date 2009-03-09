/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <kscheduler.h>


struct scheduler_ops kScheduler = {
	NULL,
	NULL,
	NULL,
	NULL,
};
struct scheduler_ops* gScheduler = &kScheduler;
