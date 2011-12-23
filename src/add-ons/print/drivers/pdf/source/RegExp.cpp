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

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <Errors.h>

#include "RegExp.h"

// The first byte of the regexp internal "program" is actually this magic
// number; the start node begins in the second byte.

const uint8 kRegExpMagic = 0234;

// The "internal use only" fields in RegExp.h are present to pass info from
// compile to execute that permits the execute phase to run lots faster on
// simple cases.  They are:
//
// regstart	char that must begin a match; '\0' if none obvious
// reganch	is the match anchored (at beginning-of-line only)?
// regmust	string (pointer into program) that match must include, or NULL
// regmlen	length of regmust string
//
// Regstart and reganch permit very fast decisions on suitable starting points
// for a match, cutting down the work a lot.  Regmust permits fast rejection
// of lines that cannot possibly match.  The regmust tests are costly enough
// that Compile() supplies a regmust only if the r.e. contains something
// potentially expensive (at present, the only such thing detected is * or +
// at the start of the r.e., which can involve a lot of backup).  Regmlen is
// supplied because the test in RunMatcher() needs it and Compile() is computing
// it anyway.
//
//
//
// Structure for regexp "program".  This is essentially a linear encoding
// of a nondeterministic finite-state machine (aka syntax charts or
// "railroad normal form" in parsing technology).  Each node is an opcode
// plus a "next" pointer, possibly plus an operand.  "Next" pointers of
// all nodes except kRegExpBranch implement concatenation; a "next" pointer with
// a kRegExpBranch on both ends of it is connecting two alternatives.  (Here we
// have one of the subtle syntax dependencies:  an individual kRegExpBranch (as
// opposed to a collection of them) is never concatenated with anything
// because of operator precedence.)  The operand of some types of node is
// a literal string; for others, it is a node leading into a sub-FSM.  In
// particular, the operand of a kRegExpBranch node is the first node of the branch.
// (NB this is *not* a tree structure:  the tail of the branch connects
// to the thing following the set of kRegExpBranches.)  The opcodes are:
//

// definition	number	opnd?	meaning 
enum {
	kRegExpEnd = 0,		// no	End of program. 
	kRegExpBol = 1,		// no	Match "" at beginning of line. 
	kRegExpEol = 2,		// no	Match "" at end of line. 
	kRegExpAny = 3,		// no	Match any one character. 
	kRegExpAnyOf = 4,	// str	Match any character in this string. 
	kRegExpAnyBut =	5,	// str	Match any character not in this string. 
	kRegExpBranch =	6,	// node	Match this alternative, or the next... 
	kRegExpBack = 7,	// no	Match "", "next" ptr points backward. 
	kRegExpExactly = 8,	// str	Match this string. 
	kRegExpNothing = 9,	// no	Match empty string. 
	kRegExpStar = 10,	// node	Match this (simple) thing 0 or more times. 
	kRegExpPlus = 11,	// node	Match this (simple) thing 1 or more times. 
	kRegExpOpen	= 20,	// no	Mark this point in input as start of #n. 
							//	kRegExpOpen + 1 is number 1, etc. 
	kRegExpClose = 30	// no	Analogous to kRegExpOpen. 
};

//
// Opcode notes:
//
// kRegExpBranch	The set of branches constituting a single choice are hooked
//		together with their "next" pointers, since precedence prevents
//		anything being concatenated to any individual branch.  The
//		"next" pointer of the last kRegExpBranch in a choice points to the
//		thing following the whole choice.  This is also where the
//		final "next" pointer of each individual branch points; each
//		branch starts with the operand node of a kRegExpBranch node.
//
// kRegExpBack		Normal "next" pointers all implicitly point forward; kRegExpBack
//		exists to make loop structures possible.
//
// kRegExpStar,kRegExpPlus	'?', and complex '*' and '+', are implemented as circular
//		kRegExpBranch structures using kRegExpBack.  Simple cases (one character
//		per match) are implemented with kRegExpStar and kRegExpPlus for speed
//		and to minimize recursive plunges.
//
// kRegExpOpen,kRegExpClose	...are numbered at compile time.
// 
//
//
// A node is one char of opcode followed by two chars of "next" pointer.
// "Next" pointers are stored as two 8-bit pieces, high order first.  The
// value is a positive offset from the opcode of the node containing it.
// An operand, if any, simply follows the node.  (Note that much of the
// code generation knows about this implicit relationship.)
//
// Using two bytes for the "next" pointer is vast overkill for most things,
// but allows patterns to get big without disasters.
//

const char *kMeta = "^$.[()|?+*\\";
const int32 kMaxSize = 32767L;		// Probably could be 65535L.

// Flags to be passed up and down:
enum {
	kHasWidth =	01,	// Known never to match null string. 
	kSimple = 02,	// Simple enough to be kRegExpStar/kRegExpPlus operand. 
	kSPStart = 04,	// Starts with * or +. 
	kWorst = 0	// Worst case. 
};

const char *kRegExpErrorStringArray[] = {
	"Unmatched parenthesis.",
	"Expression too long.",
	"Too many parenthesis.",
	"Junk on end.",
	"*+? operand may be empty.",
	"Nested *?+.",
	"Invalid bracket range.",
	"Unmatched brackets.",
	"Internal error.",
	"?+* follows nothing.",
	"Trailing \\.",
	"Corrupted expression.",
	"Memory corruption.",
	"Corrupted pointers.",
	"Corrupted opcode."
};

#ifdef DEBUG
int32 regnarrate = 0;
#endif

RegExp::RegExp()
	:	fError(B_OK),
		fRegExp(NULL)
{
}

RegExp::RegExp(const char *pattern)
	:	fError(B_OK),
		fRegExp(NULL)
{
	fRegExp = Compile(pattern);
}

RegExp::RegExp(const BString &pattern)
	:	fError(B_OK),
		fRegExp(NULL)
{
	fRegExp = Compile(pattern.String());
}

RegExp::~RegExp()
{
	free(fRegExp);
}



status_t
RegExp::InitCheck() const
{
	return fError;
}

status_t
RegExp::SetTo(const char *pattern)
{
	fError = B_OK;
	free(fRegExp);
	fRegExp = Compile(pattern);
	return fError;
}

status_t
RegExp::SetTo(const BString &pattern)
{
	fError = B_OK;
	free(fRegExp);
	fRegExp = Compile(pattern.String());
	return fError;
}

bool
RegExp::Matches(const char *string) const
{
	if (!fRegExp || !string)
		return false;
		
	return RunMatcher(fRegExp, string) == 1;
}

bool
RegExp::Matches(const BString &string) const
{
	if (!fRegExp)
		return false;

	return RunMatcher(fRegExp, string.String()) == 1;
}


//
// - Compile - compile a regular expression into internal code
//
// We can't allocate space until we know how big the compiled form will be,
// but we can't compile it (and thus know how big it is) until we've got a
// place to put the code.  So we cheat:  we compile it twice, once with code
// generation turned off and size counting turned on, and once "for real".
// This also means that we don't allocate space until we are sure that the
// thing really will compile successfully, and we never have to move the
// code and thus invalidate pointers into it.  (Note that it has to be in
// one piece because free() must be able to free it all.)
//
// Beware that the optimization-preparation code in here knows about some
// of the structure of the compiled regexp.

regexp *
RegExp::Compile(const char *exp)
{
	regexp *r;
	const char *scan;
	const char *longest;
	int32 len;
	int32 flags;

	if (exp == NULL) {
		SetError(B_BAD_VALUE);
		return NULL;
	}

	// First pass: determine size, legality.
	fInputScanPointer = exp;
	fParenthesisCount = 1;
	fCodeSize = 0L;
	fCodeEmitPointer = &fDummy;
	Char(kRegExpMagic);
	if (Reg(0, &flags) == NULL)
		return NULL;

	// Small enough for pointer-storage convention? 
	if (fCodeSize >= kMaxSize) {
		SetError(REGEXP_TOO_BIG);
		return NULL;
	}

	// Allocate space.
	r = (regexp *)malloc(sizeof(regexp) + fCodeSize);

	if (!r) {
		SetError(B_NO_MEMORY);
		return NULL;
	}

	// Second pass: emit code.
	fInputScanPointer = exp;
	fParenthesisCount = 1;
	fCodeEmitPointer = r->program;
	Char(kRegExpMagic);
	if (Reg(0, &flags) == NULL) {
		free(r);
		return NULL;
	}
	
	// Dig out information for optimizations.
	r->regstart = '\0';	// Worst-case defaults.
	r->reganch = 0;
	r->regmust = NULL;
	r->regmlen = 0;
	scan = r->program + 1;			// First kRegExpBranch. 
	if (*Next((char *)scan) == kRegExpEnd) {		// Only one top-level choice.
		scan = Operand(scan);

		// Starting-point info.
		if (*scan == kRegExpExactly)
			r->regstart = *Operand(scan);
		else if (*scan == kRegExpBol)
			r->reganch++;

		//
		// If there's something expensive in the r.e., find the
		// longest literal string that must appear and make it the
		// regmust.  Resolve ties in favor of later strings, since
		// the regstart check works with the beginning of the r.e.
		// and avoiding duplication strengthens checking.  Not a
		// strong reason, but sufficient in the absence of others.
		//
		if (flags&kSPStart) {
			longest = NULL;
			len = 0;
			for (; scan != NULL; scan = Next((char *)scan))
				if (*scan == kRegExpExactly && (int32)strlen(Operand(scan)) >= len) {
					longest = Operand(scan);
					len = (int32)strlen(Operand(scan));
				}
			r->regmust = longest;
			r->regmlen = len;
		}
	}

	return r;
}

regexp *
RegExp::Expression() const
{
	return fRegExp;
}

const char *
RegExp::ErrorString() const
{
	if (fError >= REGEXP_UNMATCHED_PARENTHESIS
		&& fError <= REGEXP_CORRUPTED_OPCODE)
		return kRegExpErrorStringArray[fError - B_ERRORS_END];

	return strerror(fError);
}


void
RegExp::SetError(status_t error) const
{
	fError = error;
}


//
// - Reg - regular expression, i.e. main body or parenthesized thing
//
// Caller must absorb opening parenthesis.
//
// Combining parenthesis handling with the base level of regular expression
// is a trifle forced, but the need to tie the tails of the branches to what
// follows makes it hard to avoid.
//
char *
RegExp::Reg(int32 paren, int32 *flagp)
{
	char *ret;
	char *br;
	char *ender;
	int32 parno = 0;
	int32 flags;

	*flagp = kHasWidth;	// Tentatively.

	// Make an kRegExpOpen node, if parenthesized.
	if (paren) {
		if (fParenthesisCount >= kSubExpressionMax) {
			SetError(REGEXP_TOO_MANY_PARENTHESIS);
			return NULL;
		}
		parno = fParenthesisCount;
		fParenthesisCount++;
		ret = Node((char)(kRegExpOpen + parno));
	} else
		ret = NULL;

	// Pick up the branches, linking them together.
	br = Branch(&flags);
	if (br == NULL)
		return NULL;
	if (ret != NULL)
		Tail(ret, br);	// kRegExpOpen -> first
	else
		ret = br;
	if (!(flags & kHasWidth))
		*flagp &= ~kHasWidth;
	*flagp |= flags&kSPStart;
	while (*fInputScanPointer == '|') {
		fInputScanPointer++;
		br = Branch(&flags);
		if (br == NULL)
			return NULL;
		Tail(ret, br);	// kRegExpBranch -> kRegExpBranch.
		if (!(flags & kHasWidth))
			*flagp &= ~kHasWidth;
		*flagp |= flags&kSPStart;
	}

	// Make a closing node, and hook it on the end.
	ender = Node(paren ? (char)(kRegExpClose + parno) : (char)kRegExpEnd);	
	Tail(ret, ender);

	// Hook the tails of the branches to the closing node.
	for (br = ret; br != NULL; br = Next(br))
		OpTail(br, ender);

	// Check for proper termination.
	if (paren && *fInputScanPointer++ != ')') {
		SetError(REGEXP_UNMATCHED_PARENTHESIS);
		return NULL;
	} else if (!paren && *fInputScanPointer != '\0') {
		if (*fInputScanPointer == ')') {
			SetError(REGEXP_UNMATCHED_PARENTHESIS);
			return NULL;
		} else {
			SetError(REGEXP_JUNK_ON_END);
			return NULL;	//  "Can't happen".
		}
		// NOTREACHED
	}

	return ret;
}

//
// - Branch - one alternative of an | operator
//
// Implements the concatenation operator.
//
char *
RegExp::Branch(int32 *flagp)
{
	char *ret;
	char *chain;
	char *latest;
	int32 flags;

	*flagp = kWorst;		// Tentatively.

	ret = Node(kRegExpBranch);
	chain = NULL;
	while (*fInputScanPointer != '\0'
		&& *fInputScanPointer != '|'
		&& *fInputScanPointer != ')') {
		latest = Piece(&flags);
		if (latest == NULL)
			return NULL;
		*flagp |= flags & kHasWidth;
		if (chain == NULL)	// First piece.
			*flagp |= flags & kSPStart;
		else
			Tail(chain, latest);
		chain = latest;
	}
	if (chain == NULL)	// Loop ran zero times.
		Node(kRegExpNothing);

	return ret;
}

//
// - Piece - something followed by possible [*+?]
//
// Note that the branching code sequences used for ? and the general cases
// of * and + are somewhat optimized:  they use the same kRegExpNothing node as
// both the endmarker for their branch list and the body of the last branch.
// It might seem that this node could be dispensed with entirely, but the
// endmarker role is not redundant.
//
char *
RegExp::Piece(int32 *flagp)
{
	char *ret;
	char op;
	char *next;
	int32 flags;

	ret = Atom(&flags);
	if (ret == NULL)
		return NULL;

	op = *fInputScanPointer;
	if (!IsMult(op)) {
		*flagp = flags;
		return ret;
	}

	if (!(flags & kHasWidth) && op != '?') {
		SetError(REGEXP_STAR_PLUS_OPERAND_EMPTY);
		return NULL;
	}
	*flagp = op != '+' ? kWorst | kSPStart :  kWorst | kHasWidth;

	if (op == '*' && (flags & kSimple))
		Insert(kRegExpStar, ret);
	else if (op == '*') {
		// Emit x* as (x&|), where & means "self".
		Insert(kRegExpBranch, ret);				// Either x 
		OpTail(ret, Node(kRegExpBack));		// and loop
		OpTail(ret, ret);				// back
		Tail(ret, Node(kRegExpBranch));		// or
		Tail(ret, Node(kRegExpNothing));		// null.
	} else if (op == '+' && (flags & kSimple))
		Insert(kRegExpPlus, ret);
	else if (op == '+') {
		// Emit x+ as x(&|), where & means "self".
		next = Node(kRegExpBranch);				// Either
		Tail(ret, next);
		Tail(Node(kRegExpBack), ret);		// loop back
		Tail(next, Node(kRegExpBranch));		// or
		Tail(ret, Node(kRegExpNothing));		// null.
	} else if (op == '?') {
		// Emit x? as (x|)
		Insert(kRegExpBranch, ret);			// Either x
		Tail(ret, Node(kRegExpBranch));	// or 
		next = Node(kRegExpNothing);		// null.
		Tail(ret, next);
		OpTail(ret, next);
	}
	fInputScanPointer++;
	if (IsMult(*fInputScanPointer)) {
		SetError(REGEXP_NESTED_STAR_QUESTION_PLUS);	
		return NULL;
	}
	return ret;
}

//
// - Atom - the lowest level
//
// Optimization:  gobbles an entire sequence of ordinary characters so that
// it can turn them into a single node, which is smaller to store and
// faster to run.  Backslashed characters are exceptions, each becoming a
// separate node; the code is simpler that way and it's not worth fixing.
//
char *
RegExp::Atom(int32 *flagp)
{
	char *ret;
	int32 flags;

	*flagp = kWorst;		// Tentatively.

	switch (*fInputScanPointer++) {
		case '^':
			ret = Node(kRegExpBol);
			break;
		case '$':
			ret = Node(kRegExpEol);
			break;
		case '.':
			ret = Node(kRegExpAny);
			*flagp |= kHasWidth|kSimple;
			break;
		case '[':
			{
				int32 cclass;
				int32 classend;
	
				if (*fInputScanPointer == '^') {	// Complement of range.
					ret = Node(kRegExpAnyBut);
					fInputScanPointer++;
				} else
					ret = Node(kRegExpAnyOf);
				if (*fInputScanPointer == ']' || *fInputScanPointer == '-')
					Char(*fInputScanPointer++);
				while (*fInputScanPointer != '\0' && *fInputScanPointer != ']') {
					if (*fInputScanPointer == '-') {
						fInputScanPointer++;
						if (*fInputScanPointer == ']' || *fInputScanPointer == '\0')
							Char('-');
						else {
							cclass = UCharAt(fInputScanPointer - 2) + 1;
							classend = UCharAt(fInputScanPointer);
							if (cclass > classend + 1) {
								SetError(REGEXP_INVALID_BRACKET_RANGE);
								return NULL;
							}
							for (; cclass <= classend; cclass++)
								Char((char)cclass);
							fInputScanPointer++;
						}
					} else
						Char(*fInputScanPointer++);
				}
				Char('\0');
				if (*fInputScanPointer != ']') {
					SetError(REGEXP_UNMATCHED_BRACKET);
					return NULL;
				}
				fInputScanPointer++;
				*flagp |= kHasWidth | kSimple;
			}
			break;
		case '(':
			ret = Reg(1, &flags);
			if (ret == NULL)
				return NULL;
			*flagp |= flags & (kHasWidth | kSPStart);
			break;
		case '\0':
		case '|':
		case ')':
			SetError(REGEXP_INTERNAL_ERROR);
			return NULL; //  Supposed to be caught earlier.
		case '?':
		case '+':
		case '*':
			SetError(REGEXP_QUESTION_PLUS_STAR_FOLLOWS_NOTHING);
			return NULL;
		case '\\':
			if (*fInputScanPointer == '\0') {
				SetError(REGEXP_TRAILING_BACKSLASH);
				return NULL;
			}
			ret = Node(kRegExpExactly);
			Char(*fInputScanPointer++);
			Char('\0');
			*flagp |= kHasWidth|kSimple;
			break;
		default:
			{
				int32 len;
				char ender;
	
				fInputScanPointer--;
				len = (int32)strcspn(fInputScanPointer, kMeta);
				if (len <= 0) {
					SetError(REGEXP_INTERNAL_ERROR);
					return NULL;
				}
				ender = *(fInputScanPointer + len);
				if (len > 1 && IsMult(ender))
					len--;		// Back off clear of ?+* operand.
				*flagp |= kHasWidth;
				if (len == 1)
					*flagp |= kSimple;
				ret = Node(kRegExpExactly);
				while (len > 0) {
					Char(*fInputScanPointer++);
					len--;
				}
				Char('\0');
			}
			break;
	}

	return ret;
}

//
// - Node - emit a node
//
char *			// Location.
RegExp::Node(char op)
{
	char *ret;
	char *ptr;

	ret = fCodeEmitPointer;
	if (ret == &fDummy) {
		fCodeSize += 3;
		return ret;
	}

	ptr = ret;
	*ptr++ = op;
	*ptr++ = '\0';		// Null "next" pointer.
	*ptr++ = '\0';
	fCodeEmitPointer = ptr;

	return ret;
}

//
// - Char - emit (if appropriate) a byte of code
//
void
RegExp::Char(char b)
{
	if (fCodeEmitPointer != &fDummy)
		*fCodeEmitPointer++ = b;
	else
		fCodeSize++;
}

//
// - Insert - insert an operator in front of already-emitted operand
// 
// Means relocating the operand.
// 
void
RegExp::Insert(char op, char *opnd)
{
	char *src;
	char *dst;
	char *place;

	if (fCodeEmitPointer == &fDummy) {
		fCodeSize += 3;
		return;
	}

	src = fCodeEmitPointer;
	fCodeEmitPointer += 3;
	dst = fCodeEmitPointer;
	while (src > opnd)
		*--dst = *--src;

	place = opnd;		// Op node, where operand used to be.
	*place++ = op;
	*place++ = '\0';
	*place++ = '\0';
}

//
// - Tail - set the next-pointer at the end of a node chain
//
void
RegExp::Tail(char *p, char *val)
{
	char *scan;
	char *temp;
	int32 offset;

	if (p == &fDummy)
		return;

	// Find last node.
	scan = p;
	for (;;) {
		temp = Next(scan);
		if (temp == NULL)
			break;
		scan = temp;
	}

	if (scan[0] == kRegExpBack)
		offset = scan - val;
	else
		offset = val - scan;

	scan[1] = (char)((offset >> 8) & 0377);
	scan[2] = (char)(offset & 0377);
}

//
// - OpTail - Tail on operand of first argument; nop if operandless
//
void
RegExp::OpTail(char *p, char *val)
{
	// "Operandless" and "op != kRegExpBranch" are synonymous in practice.
	if (p == NULL || p == &fDummy || *p != kRegExpBranch)
		return;
	Tail(Operand(p), val);
}

//
// RunMatcher and friends
//

//
// - RunMatcher - match a regexp against a string
//
int32
RegExp::RunMatcher(regexp *prog, const char *string) const
{
	const char *s;

	// Be paranoid... 
	if (prog == NULL || string == NULL) {
		SetError(B_BAD_VALUE); 
		return 0;
	}

	// Check validity of program.
	if (UCharAt(prog->program) != kRegExpMagic) {
		SetError(REGEXP_CORRUPTED_PROGRAM); 
		return 0;
	}

	// If there is a "must appear" string, look for it.
	if (prog->regmust != NULL) {
		s = string;
		while ((s = strchr(s, prog->regmust[0])) != NULL) {
			if (strncmp(s, prog->regmust, (size_t)prog->regmlen) == 0)
				break;	// Found it.
			s++;
		}
		if (s == NULL)	// Not present.
			return 0;
	}

	// Mark beginning of line for ^ .
	fRegBol = string;

	// Simplest case:  anchored match need be tried only once.
	if (prog->reganch)
		return Try(prog, (char*)string);

	// Messy cases:  unanchored match.
	s = string;
	if (prog->regstart != '\0')
		// We know what char it must start with.
		while ((s = strchr(s, prog->regstart)) != NULL) {
			if (Try(prog, (char*)s))
				return 1;
			s++;
		}
	else
		// We don't -- general case.
		do {
			if (Try(prog, (char*)s))
				return 1;
		} while (*s++ != '\0');

	// Failure.
	return 0;
}

//
// - Try - try match at specific point
//
int32			// 0 failure, 1 success 
RegExp::Try(regexp *prog, const char *string) const
{
	int32 i;
	const char **sp;
	const char **ep;

	fStringInputPointer = string;
	fStartPArrayPointer = prog->startp;
	fEndPArrayPointer = prog->endp;

	sp = prog->startp;
	ep = prog->endp;
	for (i = kSubExpressionMax; i > 0; i--) {
		*sp++ = NULL;
		*ep++ = NULL;
	}
	if (Match(prog->program + 1)) {
		prog->startp[0] = string;
		prog->endp[0] = fStringInputPointer;
		return 1;
	} else
		return 0;
}

//
// - Match - main matching routine
//
// Conceptually the strategy is simple:  check to see whether the current
// node matches, call self recursively to see whether the rest matches,
// and then act accordingly.  In practice we make some effort to avoid
// recursion, in particular by going through "ordinary" nodes (that don't
// need to know whether the rest of the match failed) by a loop instead of
// by recursion.
///
int32			// 0 failure, 1 success
RegExp::Match(const char *prog) const
{
	const char *scan;	// Current node.
	const char *next;		// Next node.

	scan = prog;
#ifdef DEBUG
	if (scan != NULL && regnarrate)
		fprintf(stderr, "%s(\n", Prop(scan));
#endif
	while (scan != NULL) {
#ifdef DEBUG
		if (regnarrate)
			fprintf(stderr, "%s...\n", Prop(scan));
#endif
		next = Next(scan);

		switch (*scan) {
			case kRegExpBol:
				if (fStringInputPointer != fRegBol)
					return 0;
				break;
			case kRegExpEol:
				if (*fStringInputPointer != '\0')
					return 0;
				break;
			case kRegExpAny:
				if (*fStringInputPointer == '\0')
					return 0;
				fStringInputPointer++;
				break;
			case kRegExpExactly:
				{
					const char *opnd = Operand(scan);
					// Inline the first character, for speed.
					if (*opnd != *fStringInputPointer)
						return 0;

					uint32 len = strlen(opnd);
					if (len > 1 && strncmp(opnd, fStringInputPointer, len) != 0)
						return 0;

					fStringInputPointer += len;
				}
				break;
			case kRegExpAnyOf:
				if (*fStringInputPointer == '\0'
					|| strchr(Operand(scan), *fStringInputPointer) == NULL)
					return 0;
				fStringInputPointer++;
				break;
			case kRegExpAnyBut:
				if (*fStringInputPointer == '\0'
					|| strchr(Operand(scan), *fStringInputPointer) != NULL)
					return 0;
				fStringInputPointer++;
				break;
			case kRegExpNothing:
				break;
			case kRegExpBack:
				break;
			case kRegExpOpen + 1:
			case kRegExpOpen + 2:
			case kRegExpOpen + 3:
			case kRegExpOpen + 4:
			case kRegExpOpen + 5:
			case kRegExpOpen + 6:
			case kRegExpOpen + 7:
			case kRegExpOpen + 8:
			case kRegExpOpen + 9:
				{
					int32 no;
					const char *save;
	
					no = *scan - kRegExpOpen;
					save = fStringInputPointer;
	
					if (Match(next)) {
						//
						// Don't set startp if some later
						// invocation of the same parentheses
						// already has.
						//
						if (fStartPArrayPointer[no] == NULL)
							fStartPArrayPointer[no] = save;
						return 1;
					} else
						return 0;
				}
				break;
			case kRegExpClose + 1:
			case kRegExpClose + 2:
			case kRegExpClose + 3:
			case kRegExpClose + 4:
			case kRegExpClose + 5:
			case kRegExpClose + 6:
			case kRegExpClose + 7:
			case kRegExpClose + 8:
			case kRegExpClose + 9:
				{
					int32 no;
					const char *save;
	
					no = *scan - kRegExpClose;
					save = fStringInputPointer;
	
					if (Match(next)) {
						//
						// Don't set endp if some later
						// invocation of the same parentheses
						// already has.
						//
						if (fEndPArrayPointer[no] == NULL)
							fEndPArrayPointer[no] = save;
						return 1;
					} else
						return 0;
				}
				break;
			case kRegExpBranch:
				{
					const char *save;
	
					if (*next != kRegExpBranch)		// No choice.
						next = Operand(scan);	// Avoid recursion.
					else {
						do {
							save = fStringInputPointer;
							if (Match(Operand(scan)))
								return 1;
							fStringInputPointer = save;
							scan = Next(scan);
						} while (scan != NULL && *scan == kRegExpBranch);
						return 0;
						// NOTREACHED/
					}
				}
				break;
			case kRegExpStar:
			case kRegExpPlus:
				{
					char nextch;
					int32 no;
					const char *save;
					int32 min;
	
					//
					//Lookahead to avoid useless match attempts
					// when we know what character comes next.
					//
					nextch = '\0';
					if (*next == kRegExpExactly)
						nextch = *Operand(next);
					min = (*scan == kRegExpStar) ? 0 : 1;
					save = fStringInputPointer;
					no = Repeat(Operand(scan));
					while (no >= min) {
						// If it could work, try it.
						if (nextch == '\0' || *fStringInputPointer == nextch)
							if (Match(next))
								return 1;
						// Couldn't or didn't -- back up.
						no--;
						fStringInputPointer = save + no;
					}
					return 0;
				}
				break;
			case kRegExpEnd:
				return 1;	// Success!

			default:
				SetError(REGEXP_MEMORY_CORRUPTION);
				return 0;
			}

		scan = next;
	}

	//
	// We get here only if there's trouble -- normally "case kRegExpEnd" is
	// the terminating point.
	//
	SetError(REGEXP_CORRUPTED_POINTERS);
	return 0;
}

//
// - Repeat - repeatedly match something simple, report how many
//
int32
RegExp::Repeat(const char *p) const
{
	int32 count = 0;
	const char *scan;
	const char *opnd;

	scan = fStringInputPointer;
	opnd = Operand(p);
	switch (*p) {
		case kRegExpAny:
			count = (int32)strlen(scan);
			scan += count;
			break;

		case kRegExpExactly:
			while (*opnd == *scan) {
				count++;
				scan++;
			}
			break;

		case kRegExpAnyOf:
			while (*scan != '\0' && strchr(opnd, *scan) != NULL) {
				count++;
				scan++;
			}
			break;

		case kRegExpAnyBut:
			while (*scan != '\0' && strchr(opnd, *scan) == NULL) {
				count++;
				scan++;
			}
			break;

		default:		// Oh dear.  Called inappropriately.
			SetError(REGEXP_INTERNAL_ERROR);
			count = 0;	// Best compromise.
			break;
	}
	fStringInputPointer = scan;

	return count;
}

//
// - Next - dig the "next" pointer out of a node
//
char *
RegExp::Next(char *p)
{
	int32 offset;

	if (p == &fDummy)
		return NULL;

	offset = ((*(p + 1) & 0377) << 8) + (*(p + 2) & 0377);
	if (offset == 0)
		return NULL;

	if (*p == kRegExpBack)
		return p - offset;
	else
		return p + offset;
}

const char *
RegExp::Next(const char *p) const
{
	int32 offset;

	if (p == &fDummy)
		return NULL;

	offset = ((*(p + 1) & 0377) << 8) + (*(p + 2) & 0377);
	if (offset == 0)
		return NULL;

	if (*p == kRegExpBack)
		return p - offset;
	else
		return p + offset;
}

inline int32
RegExp::UCharAt(const char *p) const
{
	return (int32)*(unsigned char *)p;
}

inline char *
RegExp::Operand(char* p) const
{
	return p + 3;
}

inline const char *
RegExp::Operand(const char* p) const
{
	return p + 3;
}

inline bool
RegExp::IsMult(char c) const
{
	return c == '*' || c == '+' || c == '?';
}


#ifdef DEBUG

//
// - Dump - dump a regexp onto stdout in vaguely comprehensible form
//
void
RegExp::Dump()
{
	const char *s;
	char op = kRegExpExactly;	// Arbitrary non-kRegExpEnd op.
	const char *next;

	s = fRegExp->program + 1;
	while (op != kRegExpEnd) {	// While that wasn't kRegExpEnd last time...
		op = *s;
		printf("%2ld%s", s - fRegExp->program, Prop(s));	// Where, what.
		next = Next(s);
		if (next == NULL)		// Next ptr.
			printf("(0)");
		else 
			printf("(%ld)", (s - fRegExp->program) + (next - s));
		s += 3;
		if (op == kRegExpAnyOf || op == kRegExpAnyBut || op == kRegExpExactly) {
			// Literal string, where present.
			while (*s != '\0') {
				putchar(*s);
				s++;
			}
			s++;
		}
		putchar('\n');
	}

	// Header fields of interest.
	if (fRegExp->regstart != '\0')
		printf("start `%c' ", fRegExp->regstart);
	if (fRegExp->reganch)
		printf("anchored ");
	if (fRegExp->regmust != NULL)
		printf("must have \"%s\"", fRegExp->regmust);
	printf("\n");
}

//
// - Prop - printable representation of opcode
//
char *
RegExp::Prop(const char *op) const
{
	const char *p = NULL;
	static char buf[50];

	(void) strcpy(buf, ":");

	switch (*op) {
		case kRegExpBol:
			p = "kRegExpBol";
			break;
		case kRegExpEol:
			p = "kRegExpEol";
			break;
		case kRegExpAny:
			p = "kRegExpAny";
			break;
		case kRegExpAnyOf:
			p = "kRegExpAnyOf";
			break;
		case kRegExpAnyBut:
			p = "kRegExpAnyBut";
			break;
		case kRegExpBranch:
			p = "kRegExpBranch";
			break;
		case kRegExpExactly:
			p = "kRegExpExactly";
			break;
		case kRegExpNothing:
			p = "kRegExpNothing";
			break;
		case kRegExpBack:
			p = "kRegExpBack";
			break;
		case kRegExpEnd:
			p = "kRegExpEnd";
			break;
		case kRegExpOpen + 1:
		case kRegExpOpen + 2:
		case kRegExpOpen + 3:
		case kRegExpOpen + 4:
		case kRegExpOpen + 5:
		case kRegExpOpen + 6:
		case kRegExpOpen + 7:
		case kRegExpOpen + 8:
		case kRegExpOpen + 9:
			sprintf(buf + strlen(buf), "kRegExpOpen%d", *op - kRegExpOpen);
			p = NULL;
			break;
		case kRegExpClose + 1:
		case kRegExpClose + 2:
		case kRegExpClose + 3:
		case kRegExpClose + 4:
		case kRegExpClose + 5:
		case kRegExpClose + 6:
		case kRegExpClose + 7:
		case kRegExpClose + 8:
		case kRegExpClose + 9:
			sprintf(buf + strlen(buf), "kRegExpClose%d", *op - kRegExpClose);
			p = NULL;
			break;
		case kRegExpStar:
			p = "kRegExpStar";
			break;
		case kRegExpPlus:
			p = "kRegExpPlus";
			break;
		default:
			RegExpError("corrupted opcode");
			break;
	}

	if (p != NULL)
		strcat(buf, p);

	return buf;
}

void
RegExp::RegExpError(const char *) const
{
	// does nothing now, perhaps it should printf?
}

#endif
