/* 
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <string.h>


char*
strtok(char* str, char const* sep)
{
	static char *p = NULL;
	return strtok_r(str, sep, &p);
}
