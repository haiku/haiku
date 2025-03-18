/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>

#include <SupportDefs.h>


char*
tmpnam_r(char* nameBuffer)
{
	static int32 counter = 0;

	if (nameBuffer == NULL)
		return NULL;

	snprintf(nameBuffer, L_tmpnam, "%stmp.%d.XXXXXX", P_tmpdir, (int)atomic_add(&counter, 1));
	return mktemp(nameBuffer);
}


char*
tmpnam(char* nameBuffer)
{
	static char buffer[L_tmpnam];
	if (nameBuffer == NULL)
		nameBuffer = buffer;

	return tmpnam_r(nameBuffer);
}
