// Author: Ryan Fleet
// Created: October 11th 2002
// Modified: October 12th 2002


#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "usage: unmount path\n");
		return 1;
	}
	
	if(unmount(argv[1]) < 0)
	{
		fprintf(stderr, "unmount: %s\n", strerror(errno));
		return 1;
	}
		
	return 0;
}
