/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <setjmp.h>
#include <stdio.h>


int
main(int argc, char **argv)
{
	jmp_buf state;
	int value;

	if ((value = setjmp(state)) != 0) {
		printf("failed with: %d!\n", value);
	} else {
		printf("here I am: %d\n", value);
		longjmp(state, 42);
		printf("you won't see me!\n");
	}

	puts("done.");
	return 0;
}
