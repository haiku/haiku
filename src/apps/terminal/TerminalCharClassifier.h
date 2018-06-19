/*
 * Copyright 2008-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TERMINAL_CHAR_CLASSIFIER_H
#define TERMINAL_CHAR_CLASSIFIER_H


#include <vector>

#include "UTF8Char.h"


enum {
	CHAR_TYPE_SPACE,
	CHAR_TYPE_WORD_CHAR,
	CHAR_TYPE_WORD_DELIMITER
};


struct UTF8Char;

class TerminalCharClassifier {
public:
	virtual						~TerminalCharClassifier();

	virtual	int					Classify(const UTF8Char& character) = 0;
};


class DefaultCharClassifier: public TerminalCharClassifier {
public:
								DefaultCharClassifier(
									const char* additionalWordChars);

	virtual	int					Classify(const UTF8Char& character);

private:
			std::vector<UTF8Char> fAdditionalWordChars;
};


#endif	// TERMINAL_CHAR_CLASSIFIER_H
