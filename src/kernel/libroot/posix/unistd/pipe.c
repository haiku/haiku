/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>


int
pipe(int streams[2])
{
	// ToDo: if the thread manages to call this function during the
	//	the same microsecond twice, we're doomed :)
	char pipeName[64];
	sprintf(pipeName, "/pipe/%lx-%Ld\n", find_thread(NULL), system_time());

	streams[0] = open(pipeName, O_CREAT | O_RDONLY, 0777);
	if (streams[0] < 0)
		return -1;

	streams[1] = open(pipeName, O_WRONLY);
	if (streams[1] < 0) {
		close(streams[0]);
		return -1;
	}

	return 0;
}

