/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

// This file includes some known R5 syscalls
// ToDo: maybe they should indeed do something...

#include <SupportDefs.h>

#include <syscalls.h>


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
	return _kern_cpu_enabled(cpuNum);
}


int
_kset_cpu_state_(int cpuNum, int state)
{
	return _kern_set_cpu_enabled(cpuNum, state != 0);
}

