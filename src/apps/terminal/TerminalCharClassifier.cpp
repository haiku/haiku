/*
 * Copyright 2008-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TerminalCharClassifier.h"

#include <ctype.h>

#include <algorithm>


// #pragma mark - TerminalCharClassifier


TerminalCharClassifier::~TerminalCharClassifier()
{
}


// #pragma mark - DefaultCharClassifier


DefaultCharClassifier::DefaultCharClassifier(const char* additionalWordChars)
{
	const char* p = additionalWordChars;
	while (p != NULL && *p != '\0') {
		int count = UTF8Char::ByteCount(*p);
		if (count <= 0 || count > 4)
			break;
		fAdditionalWordChars.push_back(UTF8Char(p, count));
		p += count;
	}
}


int
DefaultCharClassifier::Classify(const UTF8Char& character)
{
	if (character.IsSpace())
		return CHAR_TYPE_SPACE;

	if (character.IsAlNum())
		return CHAR_TYPE_WORD_CHAR;

	if (std::find(fAdditionalWordChars.begin(), fAdditionalWordChars.end(),
			character) != fAdditionalWordChars.end()) {
		return CHAR_TYPE_WORD_CHAR;
	}

	return CHAR_TYPE_WORD_DELIMITER;
}
