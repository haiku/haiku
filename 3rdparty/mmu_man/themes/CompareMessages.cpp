/*
 * function to compare 2 BMessages
 */
#include <string.h>

#include <Message.h>

/* returns true if == */
bool CompareMessages(BMessage &a, BMessage &b)
{
	void *cookie = NULL;
	const char *name;
	type_code code;
	int32 count, i;
	const void *adata, *bdata;
	ssize_t asize, bsize;
	
	if (a.what != b.what)
		return false;
	while (a.GetNextName(&cookie, &name, &code, &count) == B_OK) {
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
	cookie = NULL;
	/* cross compare */
	while (b.GetNextName(&cookie, &name, &code, &count) == B_OK) {
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

