/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <string.h>
#include <errno.h>


int 
sethostname(const char *hostName, size_t nameSize)
{
	return EPERM;
}


int 
gethostname(char *hostName, size_t nameSize)
{
	strlcpy(hostName, "beast", nameSize);
	return 0;
}

