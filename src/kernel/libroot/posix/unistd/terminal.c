/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>


/*
 * isatty - is the given file descriptor bound to a terminal device?
 * 
 * a simple call to fetch the terminal control attributes suffices
 * (only a valid tty device will succeed)
 *
 */
int
isatty(int fd)
{
	struct termios term;
	
	return (tcgetattr(fd, &term) == 0);
}


/*
 * ctermid - return the name of the controlling terminal
 *
 * this is a totally useless function!
 * (but kept for historical Posix compatibility)
 * yes, it *always* returns "/dev/tty"
 *
 */
char *
ctermid(char *s)
{
	static char defaultBuffer[L_ctermid];
	
	if (s == NULL)
		s = defaultBuffer;
	
	return strcpy(s, "/dev/tty");
}

