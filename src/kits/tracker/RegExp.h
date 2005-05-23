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


// This code is based on regexp.c, v.1.3 by Henry Spencer:

// @(#)regexp.c	1.3 of 18 April 87
//
//	Copyright (c) 1986 by University of Toronto.
//	Written by Henry Spencer.  Not derived from licensed software.
//
//	Permission is granted to anyone to use this software for any
//	purpose on any computer system, and to redistribute it freely,
//	subject to the following restrictions:
//
//	1. The author is not responsible for the consequences of use of
//		this software, no matter how awful, even if they arise
//		from defects in it.
//
//	2. The origin of this software must not be misrepresented, either
//		by explicit claim or by omission.
//
//	3. Altered versions must be plainly marked as such, and must not
//		be misrepresented as being the original software.
//
// Beware that some of this code is subtly aware of the way operator
// precedence is structured in regular expressions.  Serious changes in
// regular-expression syntax might require a total rethink.
//

// ALTERED VERSION: Adapted to ANSI C and C++ for the OpenTracker
// project (www.opentracker.org), Jul 11, 2000.

#ifndef _REG_EXP_H
#define _REG_EXP_H

#include <String.h>

namespace BPrivate {

enum {
	REGEXP_UNMATCHED_PARENTHESIS = B_ERRORS_END,
	REGEXP_TOO_BIG,
	REGEXP_TOO_MANY_PARENTHESIS,
	REGEXP_JUNK_ON_END,
	REGEXP_STAR_PLUS_OPERAND_EMPTY,
	REGEXP_NESTED_STAR_QUESTION_PLUS,
	REGEXP_INVALID_BRACKET_RANGE,
	REGEXP_UNMATCHED_BRACKET,
	REGEXP_INTERNAL_ERROR,
	REGEXP_QUESTION_PLUS_STAR_FOLLOWS_NOTHING,
	REGEXP_TRAILING_BACKSLASH,
	REGEXP_CORRUPTED_PROGRAM,
	REGEXP_MEMORY_CORRUPTION,
	REGEXP_CORRUPTED_POINTERS,
	REGEXP_CORRUPTED_OPCODE
};

const int32 kSubExpressionMax = 10;

struct regexp {
	const char *startp[kSubExpressionMax];
	const char *endp[kSubExpressionMax];
	char regstart;		/* Internal use only. See RegExp.cpp for details. */
	char reganch;		/* Internal use only. */
	const char *regmust;/* Internal use only. */
	int regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chumminess with compiler. */
};

class RegExp {

public:
	RegExp();
	RegExp(const char *);
	RegExp(const BString &);
	~RegExp();
	
	status_t InitCheck() const;
	
	status_t SetTo(const char*);
	status_t SetTo(const BString &);
	
	bool Matches(const char *string) const;
	bool Matches(const BString &) const;

	int32 RunMatcher(regexp *, const char *) const;
	regexp *Compile(const char *);
	regexp *Expression() const;
	const char *ErrorString() const;

#ifdef DEBUG
	void Dump();
#endif

private:

	void SetError(status_t error) const;

	// Working functions for Compile():
	char *Reg(int32, int32 *);
	char *Branch(int32 *);
	char *Piece(int32 *);
	char *Atom(int32 *);
	char *Node(char);
	char *Next(char *);
	const char *Next(const char *) const;
	void Char(char);
	void Insert(char, char *);
	void Tail(char *, char *);
	void OpTail(char *, char *);

	// Working functions for RunMatcher():
	int32 Try(regexp *, const char *) const;
	int32 Match(const char *) const;
	int32 Repeat(const char *) const;

	// Utility functions:
#ifdef DEBUG
	char *Prop(const char *) const;
	void RegExpError(const char *) const;
#endif
	inline int32 UCharAt(const char *p) const;
	inline char *Operand(char* p) const;
	inline const char *Operand(const char* p) const;
	inline bool	IsMult(char c) const;

// --------- Variables -------------

	mutable status_t fError;
	regexp *fRegExp;

	// Work variables for Compile().

	const char *fInputScanPointer;
	int32 fParenthesisCount;		
	char fDummy;
	char *fCodeEmitPointer;		// &fDummy = don't.
	long fCodeSize;		

	// Work variables for RunMatcher().

	mutable const char *fStringInputPointer;
	mutable const char *fRegBol;	// Beginning of input, for ^ check.
	mutable const char **fStartPArrayPointer;
	mutable const char **fEndPArrayPointer;
};

} // namespace BPrivate

using namespace BPrivate;

#endif
