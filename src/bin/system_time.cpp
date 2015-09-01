/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */

#include <OS.h>

#include <stdio.h>


int
main(int argc, char* argv[])
{
	printf("%" B_PRIdBIGTIME "\n", system_time());
	return 0;
}
