/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "Utilities.h"

#include <fs_attr.h>
#include <Node.h>
#include <String.h>
#include <TypeConstants.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


status_t
WriteAttrString(BNode* node, const char* attr, const char* value)
{
	if (!value)
		value = B_EMPTY_STRING;

	ssize_t size = node->WriteAttr(attr, B_STRING_TYPE, 0, value,
		strlen(value) + 1);

	return size >= 0 ? B_OK : size;
}


//====================================================================
// case-insensitive version of strcmp
//

int32
cistrcmp(const char* str1, const char* str2)
{
	char	c1;
	char	c2;
	int32	len;
	int32	loop;

	len = strlen(str1) + 1;
	for (loop = 0; loop < len; loop++) {
		c1 = str1[loop];
		if (c1 >= 'A' && c1 <= 'Z')
			c1 += 'a' - 'A';
		c2 = str2[loop];
		if (c2 >= 'A' && c2 <= 'Z')
			c2 += 'a' - 'A';
		if (c1 == c2) {
		} else if (c1 < c2) {
			return -1;
		} else if (c1 > c2 || !c2) {
			return 1;
		}
	}
	return 0;
}


//====================================================================
// case-insensitive version of strncmp
//

int32
cistrncmp(const char* str1, const char* str2, int32 max)
{
	char		c1;
	char		c2;
	int32		loop;

	for (loop = 0; loop < max; loop++) {
		c1 = *str1++;
		if (c1 >= 'A' && c1 <= 'Z')
			c1 += 'a' - 'A';
		c2 = *str2++;
		if (c2 >= 'A' && c2 <= 'Z')
			c2 += 'a' - 'A';
		if (c1 == c2) {
		} else if (c1 < c2) {
			return -1;
		} else if (c1 > c2 || !c2) {
			return 1;
		}
	}
	return 0;
}


//--------------------------------------------------------------------
// case-insensitive version of strstr
//

char*
cistrstr(const char* cs, const char* ct)
{
	char		c1;
	char		c2;
	int32		cs_len;
	int32		ct_len;
	int32		loop1;
	int32		loop2;

	cs_len = strlen(cs);
	ct_len = strlen(ct);
	for (loop1 = 0; loop1 < cs_len; loop1++) {
		if (cs_len - loop1 < ct_len)
			return NULL;

		for (loop2 = 0; loop2 < ct_len; loop2++) {
			c1 = cs[loop1 + loop2];
			if ((c1 >= 'A') && (c1 <= 'Z'))
				c1 += ('a' - 'A');
			c2 = ct[loop2];
			if ((c2 >= 'A') && (c2 <= 'Z'))
				c2 += ('a' - 'A');
			if (c1 != c2)
				goto next;
		}
		return const_cast<char*>(&cs[loop1]);
next:
		// label must be followed by a statement
		;
	}
	return NULL;
}


//--------------------------------------------------------------------
// return length of \n terminated line
//

int32
linelen(char* str, int32 len, bool header)
{
	int32		loop;

	for (loop = 0; loop < len; loop++) {
		if (str[loop] == '\n') {
			if (!header || loop < 2
				|| (header && str[loop + 1] != ' ' && str[loop + 1] != '\t'))
				return loop + 1;
		}
	}
	return len;
}

