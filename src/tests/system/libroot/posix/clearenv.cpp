/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>


int gTestNr;


void
set_env()
{
	gTestNr++;
	printf("Test %d...\n", gTestNr);

	if (setenv("TEST_VARIABLE", "42", true) != 0)
		fprintf(stderr, "Test %d: setting variable failed!\n", gTestNr);
}


void
test_env()
{
	if (getenv("TEST_VARIABLE") != NULL)
		fprintf(stderr, "Test %d: not cleared!\n", gTestNr);
	if (setenv("OTHER_VARIABLE", "test", true) != 0)
		fprintf(stderr, "Test %d: setting other failed!\n", gTestNr);
}


int
main(int argc, char** argv)
{
	set_env();
	environ = NULL;
	test_env();

	set_env();
	environ[0] = NULL;
	test_env();

	static char* emptyEnv[1] = {NULL};
	set_env();
	environ = emptyEnv;
	test_env();

	set_env();
	environ = (char**)calloc(1, sizeof(*environ));
	test_env();

	// clearenv() is not part of the POSIX specs
#if 1
	set_env();
	clearenv();
	test_env();
#endif

	return 0;
}

