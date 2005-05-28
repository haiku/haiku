/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

// This file includes some known R5 syscalls
// ToDo: maybe they should indeed do something...

#include <SupportDefs.h>


int _kset_mon_limit_(int num);
int _kset_fd_limit_(int num);
int _kget_cpu_state_(int cpuNum);
int _kset_cpu_state_(int cpuNum, int state);


int
_kset_mon_limit_(int num)
{
	return B_ERROR;
}


int
_kset_fd_limit_(int num)
{
	return B_ERROR;
}


int
_kget_cpu_state_(int cpuNum)
{
	return 1;
}


int
_kset_cpu_state_(int cpuNum, int state)
{
	return state ? B_OK : B_ERROR;
}

