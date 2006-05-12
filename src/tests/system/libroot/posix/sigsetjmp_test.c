/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <setjmp.h>

void
jump_to_top_level(jmp_buf *state, int value)
{
	siglongjmp(*state, value);
}


int
main(int argc, char **argv)
{
	jmp_buf state;
	int value;

	if (value = sigsetjmp(state, 1)) {
		printf("failed with: %d!\n", value);
	} else {
		printf("here I am: %d\n", value);
		jump_to_top_level(&state, 42);
		printf("you won't see me!\n");
	}

	puts("done.");
	return 0;
}
