/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>

#include <signal.h>
#include <stdio.h>


int
sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	// ToDo: implement me!
	fprintf(stderr, "sigprocmask(): NOT IMPLEMENTED\n");
	return -1;
}

