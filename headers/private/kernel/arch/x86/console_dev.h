/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _CONSOLE_DEV_H
#define _CONSOLE_DEV_H

struct kernel_args;

int console_dev_init(struct kernel_args *ka);

enum {
	CONSOLE_OP_WRITEXY = 2376
};

#endif
