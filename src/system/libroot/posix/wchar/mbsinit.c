/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


int
__mbsinit(const mbstate_t* ps)
{
	return ps == NULL || ps->count == 0;
}


B_DEFINE_WEAK_ALIAS(__mbsinit, mbsinit);
