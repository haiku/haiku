/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TERMINAL_CHAR_CLASSIFIER_H
#define TERMINAL_CHAR_CLASSIFIER_H


enum {
	CHAR_TYPE_SPACE,
	CHAR_TYPE_WORD_CHAR,
	CHAR_TYPE_WORD_DELIMITER
};


class TerminalCharClassifier {
public:
	virtual						~TerminalCharClassifier();

	virtual	int					Classify(const char* character) = 0;
};


#endif	// TERMINAL_CHAR_CLASSIFIER_H
