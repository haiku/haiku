/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef STRING_UTILS_H
#define STRING_UTILS_H


#include <String.h>


class StringUtils {

public:
	static	void			InSituTrimSpaceAndControl(BString& value);
	static	void			InSituStripSpaceAndControl(BString& value);

	static	int				NullSafeCompare(const char* s1, const char* s2);

private:
	static	bool			_IsSpaceOrControl(char ch);
};

#endif // STRING_UTILS_H
