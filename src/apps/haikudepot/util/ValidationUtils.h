/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef VALIDATION_UTILS_H
#define VALIDATION_UTILS_H


#include <String.h>


class ValidationUtils {

public:
	static bool				IsValidEmail(const BString& value);
	static bool				IsValidNickname(const BString& value);
	static bool				IsValidPasswordClear(const BString& value);
};

#endif // VALIDATION_UTILS_H
