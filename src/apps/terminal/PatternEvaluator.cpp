/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PatternEvaluator.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>



// #pragma mark - PatternEvaluator


/*static*/ BString
PatternEvaluator::Evaluate(const char* pattern, PlaceholderMapper& mapper)
{
	BString result;

	while (*pattern != '\0') {
		// find next placeholder
		const char* placeholder = strchr(pattern, '%');
		if (placeholder == NULL)
			return result.Append(pattern);

		// append skipped chars
		if (placeholder != pattern)
			result.Append(pattern, placeholder - pattern);

		pattern = placeholder + 1;

		// check for an escaped '%'
		if (*pattern == '%') {
			result += '%';
			pattern++;
			continue;
		}

		// parse a number, if there is one
		int64 number = 0;
		bool hasNumber = false;
		if (isdigit(*pattern)) {
			char* numberEnd;
			number = strtoll(pattern, &numberEnd, 10);
			pattern = numberEnd;
			hasNumber = true;
		}

		BString mappedValue;
		if (*pattern != '\0' && mapper.MapPlaceholder(*pattern,
				number, hasNumber, mappedValue)) {
			// mapped successfully -- append the replacement string
			result += mappedValue;
			pattern++;
		} else {
			// something went wrong -- just append the literal part of the
			// pattern
			result.Append(placeholder, pattern - placeholder);
		}
	}

	return result;
}



// #pragma mark - PlaceholderMapper


PatternEvaluator::PlaceholderMapper::~PlaceholderMapper()
{
}
