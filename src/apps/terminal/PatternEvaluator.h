/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PATTERN_EVALUATOR_H
#define PATTERN_EVALUATOR_H


#include <String.h>


class PatternEvaluator {
public:
	class PlaceholderMapper;

public:
	static	BString				Evaluate(const char* pattern,
									PlaceholderMapper& mapper);

};


class PatternEvaluator::PlaceholderMapper {
public:
	virtual						~PlaceholderMapper();

	virtual	bool				MapPlaceholder(char placeholder,
									int64 number, bool numberGiven,
									BString& _string) = 0;
};


#endif	// PATTERN_EVALUATOR_H
