/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <setjmp.h>


int
main(int argc, char **argv)
{
	jmp_buf state;
	int value;

	if (value = setjmp(state)) {
		printf("failed with: %d!\n", value);
	} else {
		printf("here I am: %d\n", value);
		longjmp(state, 42);
		printf("you won't see me!\n");
	}

	puts("done.");
	return 0;
}
