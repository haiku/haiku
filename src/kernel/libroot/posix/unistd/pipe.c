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
	static int32 counter = 0;
		// ToDo: a way without this variable would be even cooler

	char pipeName[64];
	sprintf(pipeName, "/pipe/%03lx-%03ld", find_thread(NULL), atomic_add(&counter, 1));

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

