/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * a helper function to truncate strings
 *
 */


#ifndef TRUNCATE_STRING_H
#define TRUNCATE_STRING_H

#include <SupportDefs.h>

// truncated_string
void
truncate_string(const char* string,
				uint32 mode, float width, char* result,
				const float* escapementArray, float fontSize,
				float ellipsisWidth, int32 length, int32 numChars);

#endif // STRING_TRUNCATION_H
