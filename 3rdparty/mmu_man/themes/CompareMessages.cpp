/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * function to compare 2 BMessages
 */
#include <string.h>

#include <Message.h>
#include "Utils.h"

/* returns true if == */
bool CompareMessages(BMessage &a, BMessage &b)
{
	char *name;
	type_code code;
	int32 count, index, i;
	const void *adata, *bdata;
	ssize_t asize, bsize;
	
	if (a.what != b.what)
		return false;
	for (index = 0; a.GetInfo(B_ANY_TYPE, index, GET_INFO_NAME_PTR(&name), &code, &count) == B_OK; i++) {
		for (i = 0; i < count; i++) {
			if (a.FindData(name, code, i, &adata, &asize) != B_OK)
				return false;
			if (b.FindData(name, code, i, &bdata, &bsize) != B_OK)
				return false;
			if (asize != bsize)
				return false;
			if (memcmp(adata, bdata, asize))
				return false;
		}
	}
	/* cross compare */
	for (index = 0; b.GetInfo(B_ANY_TYPE, index, GET_INFO_NAME_PTR(&name), &code, &count) == B_OK; i++) {
		type_code acode;
		int32 acount;
		if (a.GetInfo(name, &acode, &acount) < B_OK)
			return false;
		if (code != acode)
			return false;
		if (count != acount)
			return false;
	}
	return true;
}

