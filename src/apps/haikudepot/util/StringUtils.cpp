/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "StringUtils.h"


/*static*/ void
StringUtils::InSituTrimSpaceAndControl(BString& value)
{
	int i = 0;
	int len = value.Length();

	while (i < len && _IsSpaceOrControl(value.ByteAt(i)))
		i ++;

	if (i != 0)
		value.Remove(0, i);

	len = value.Length();
	i = len - 1;

	while (i > 0 && _IsSpaceOrControl(value.ByteAt(i)))
		i --;

	if (i != (len - 1))
		value.Remove(i + 1, (len - (i + 1)));
}


/*static*/ void
StringUtils::InSituStripSpaceAndControl(BString& value)
{
	for (int i = value.Length() - 1; i >= 0; i--) {
		if (_IsSpaceOrControl(value.ByteAt(i)))
			value.Remove(i, 1);
	}
}


/*static*/ bool
StringUtils::_IsSpaceOrControl(char ch)
{
	return ch <= 32;
}
