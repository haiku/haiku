/*
 * Copyright 2012, Oliver Tappe, zooey@hirschkaefer.de
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <wchar.h>


int
main(int argc, char** argv)
{
	int result = 0;

	fprintf(stdout, "stdout should now be set to non-wide mode ...\n");
	result = fwide(stdout, 0);
	if (result != -1)
	{
		printf("PROBLEM: fwide(stdout, 0) = %d (expected -1)\n", result);
	}

	fwprintf(stderr, L"stderr should now be set to non-wide mode ...\n");
	result = fwide(stderr, 0);
	if (result != 1)
	{
		printf("PROBLEM: fwide(stderr, 0) = %d (expected -1)\n", result);
	}

	fprintf(stderr, "%s", "this should *not* be visible!\n");
	fwprintf(stdout, L"%ls", L"this should *not* be visible!\n");

	fprintf(stderr, "%ls", L"this should *not* be visible!\n");
	fwprintf(stdout, L"%s", "this should *not* be visible!\n");

	fprintf(stdout, "%ls", L"this *should* be visible!\n");
	fwprintf(stderr, L"%s", "this *should* be visible!\n");

	fprintf(stdout, "%s", "this *should* be visible!\n");
	fwprintf(stderr, L"%ls", L"this *should* be visible!\n");

	return 0;
}
