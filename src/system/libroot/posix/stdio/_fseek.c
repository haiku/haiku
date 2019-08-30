/*
** Copyright 2005, Jérôme Duval. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <stdio.h>

int
_fseek(FILE *stream, fpos_t offset, int seekType)
{
	return fseeko(stream, offset, seekType);
}
