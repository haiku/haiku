/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef REG_EXP_H
#define REG_EXP_H


#include <stddef.h>


class RegExp {
public:
			enum PatternType {
				PATTERN_TYPE_REGULAR_EXPRESSION,
				PATTERN_TYPE_WILDCARD
			};

			class MatchResult;

public:
								RegExp();
								RegExp(const char* pattern,
									PatternType patternType
										= PATTERN_TYPE_REGULAR_EXPRESSION,
									bool caseSensitive = true);
								RegExp(const RegExp& other);
								~RegExp();

			bool				IsValid() const
									{ return fData != NULL; }

			bool				SetPattern(const char* pattern,
									PatternType patternType
										= PATTERN_TYPE_REGULAR_EXPRESSION,
									bool caseSensitive = true);

			MatchResult			Match(const char* string) const;

			RegExp&				operator=(const RegExp& other);

private:
			struct Data;
			struct MatchResultData;

private:
			Data*				fData;
};


class RegExp::MatchResult {
public:
								MatchResult();
								MatchResult(const MatchResult& other);
								~MatchResult();

			bool				HasMatched() const;

			size_t				StartOffset() const;
			size_t				EndOffset() const;

			size_t				GroupCount() const;
			size_t				GroupStartOffsetAt(size_t index) const;
			size_t				GroupEndOffsetAt(size_t index) const;

			MatchResult&		operator=(const MatchResult& other);

private:
			friend class RegExp;

private:
								MatchResult(MatchResultData* data);
									// takes over the data reference

private:
			MatchResultData*	fData;
};


#endif	// REG_EXP_H
