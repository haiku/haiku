/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
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
	BString before;
	bool isBefore = false;
	bool isAfter = false;
	bool hadResult = false;
	bool began = false;

	while (*pattern != '\0') {
		// find next placeholder
		const char* placeholder = strchr(pattern, '%');
		size_t length = 0;
		if (placeholder != NULL)
			length = placeholder - pattern;
		else
			length = INT_MAX;

		// append skipped chars
		if (placeholder != pattern) {
			if (isBefore) {
				before.SetTo(pattern, length);
				isBefore = false;
			} else if (!isAfter || hadResult) {
				result.Append(pattern, length);
				isBefore = false;
				before.SetTo(NULL);
				isAfter = false;
			}
		}
		if (placeholder == NULL)
			return result;

		pattern = placeholder + 1;

		// check for special placeholders
		switch (pattern[0]) {
			case '%':
				// An escaped '%'
				result += '%';
				pattern++;
				continue;
			case '<':
				// An optional before string
				isBefore = began = true;
				hadResult = false;
				pattern++;
				continue;
			case '>':
				// An optional after string
				isAfter = true;
				began = false;
				before.SetTo(NULL);
				pattern++;
				continue;
			case '-':
				// End of any other section; ignore
				pattern++;
				isBefore = false;
				isAfter = false;
				continue;
		}
		// Count non alpha numeric characters to the before section
		while (pattern[0] != '\0' && !isalnum(pattern[0])) {
			before.Append(pattern[0], 1);
			pattern++;
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
			if (began && !mappedValue.IsEmpty())
				hadResult = true;
			if (!before.IsEmpty() && !mappedValue.IsEmpty()) {
				result += before;
				before.SetTo(NULL);
			}

			result += mappedValue;
			pattern++;
		} else {
			// something went wrong -- just append the literal part of the
			// pattern
			result.Append(placeholder, length);
		}
	}

	return result;
}


// #pragma mark - PlaceholderMapper


PatternEvaluator::PlaceholderMapper::~PlaceholderMapper()
{
}
