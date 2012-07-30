/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef _TRACKER_STRING_H
#define _TRACKER_STRING_H


#include <ctype.h>

#include <OS.h>
#include <String.h>

#include "RegExp.h"


namespace BPrivate {

enum TrackerStringExpressionType {
	kNone = B_ERROR,
	kStartsWith = 0,
	kEndsWith,
	kContains,
	kGlobMatch,
	kRegexpMatch
};


class TrackerString : public BString
{
public:
	TrackerString();
	TrackerString(const char*);
	TrackerString(const TrackerString&);
	TrackerString(const char*, int32 maxLength);
	~TrackerString();

	bool Matches(const char*, bool caseSensitivity = false,
		TrackerStringExpressionType expressionType = kGlobMatch) const;

	bool MatchesRegExp(const char*, bool caseSensitivity = true) const;
	bool MatchesRegExp(const RegExp&) const;
	bool MatchesRegExp(const RegExp*) const;
	
	bool MatchesGlob(const char*, bool caseSensitivity = false) const;
	bool EndsWith(const char*, bool caseSensitivity = false) const;
	bool StartsWith(const char*, bool caseSensitivity = false) const;
	bool Contains(const char*, bool caseSensitivity = false) const;

	int32 FindFirst(const BString&) const;
	int32 FindFirst(const char*) const;
	int32 FindFirst(const BString&, int32 fromOffset) const;
	int32 FindFirst(const char*, int32 fromOffset) const;
	int32 FindFirst(char) const;
	int32 FindFirst(char, int32 fromOffset) const;

	int32 FindLast(const BString&) const;
	int32 FindLast(const char*) const;
	int32 FindLast(const BString&, int32 beforeOffset) const;
	int32 FindLast(const char*, int32 beforeOffset) const;
	int32 FindLast(char) const;
	int32 FindLast(char, int32 beforeOffset) const;

	int32 IFindFirst(const BString&) const;
	int32 IFindFirst(const char*) const;
	int32 IFindFirst(const BString&, int32 fromOffset) const;
	int32 IFindFirst(const char*, int32 fromOffset) const;

	int32 IFindLast(const BString&) const;
	int32 IFindLast(const char*) const;
	int32 IFindLast(const BString&, int32 beforeOffset) const;
	int32 IFindLast(const char*, int32 beforeOffset) const;

private:
	bool IsGlyph(char) const;
	bool IsInsideGlyph(char) const;
		// Not counting start!
	bool IsStartOfGlyph(char) const;
	const char* MoveToEndOfGlyph(const char*) const;
	
	// Functions for Glob matching:
	bool MatchesBracketExpression(const char* string, const char* pattern,
		bool caseSensitivity) const;
	bool StringMatchesPattern(const char* string, const char* pattern,
		bool caseSensitivity) const;
		
	char ConditionalToLower(char c, bool toLower) const;
	bool CharsAreEqual(char char1, char char2, bool toLower) const;
	bool UTF8CharsAreEqual(const char* string1, const char* string2) const;
};


inline bool
TrackerString::MatchesRegExp(const RegExp* expression) const
{
	if (expression == NULL || expression->InitCheck() != B_OK)
		return false;
				
	return expression->Matches(*this);
}


inline bool
TrackerString::MatchesRegExp(const RegExp &expression) const
{
	if (expression.InitCheck() != B_OK)
		return false;
				
	return expression.Matches(*this);
}


inline char
TrackerString::ConditionalToLower(char c, bool caseSensitivity) const
{
	return caseSensitivity ? c : (char)tolower(c);
}


inline bool
TrackerString::CharsAreEqual(char char1, char char2,
	bool caseSensitivity) const
{
	return ConditionalToLower(char1, caseSensitivity)
		== ConditionalToLower(char2, caseSensitivity);
}

} // namespace BPrivate

using namespace BPrivate;

#endif	// _TRACKER_STRING_H
