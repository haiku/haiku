/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.2
// Copyright (C) 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//		  mcseemagg@yahoo.com
//		  http://www.antigrain.com

#include "PathTokenizer.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


namespace agg {	
namespace svg {

// globals
const char PathTokenizer::sCommands[]   = "+-MmZzLlHhVvCcSsQqTtAaFfPp";
const char PathTokenizer::sNumeric[]	= ".Ee0123456789";
const char PathTokenizer::sSeparators[] = " ,\t\n\r";

// constructor
PathTokenizer::PathTokenizer()
	: fPath(0),
	  fLastNumber(0.0),
	  fLastCommand(0)
{
	init_char_mask(fCommandsMask,   sCommands);
	init_char_mask(fNumericMask,	sNumeric);
	init_char_mask(fSeparatorsMask, sSeparators);
}

// set_path_str
void
PathTokenizer::set_path_str(const char* str)
{
	fPath = str;
	fLastCommand = 0;
	fLastNumber = 0.0;
}

// next
bool
PathTokenizer::next()
{
	if(fPath == 0) return false;

	// Skip all white spaces and other garbage
	while (*fPath && !is_command(*fPath) && !isNumeric(*fPath))  {
		if (!is_separator(*fPath)) {
			char buf[100];
			sprintf(buf, "PathTokenizer::next : Invalid Character %c", *fPath);
			throw exception(buf);
		}
		fPath++;
	}

	if (*fPath == 0) return false;

	if (is_command(*fPath)) {
		// Check if the command is a numeric sign character
		if(*fPath == '-' || *fPath == '+')
		{
			return parse_number();
		}
		fLastCommand = *fPath++;
		while(*fPath && is_separator(*fPath)) fPath++;
		if(*fPath == 0) return true;
	}
	return parse_number();
}

// next
double
PathTokenizer::next(char cmd)
{
	if (!next()) throw exception("parse_path: Unexpected end of path");
	if (last_command() != cmd) {
		char buf[100];
		sprintf(buf, "parse_path: Command %c: bad or missing parameters", cmd);
		throw exception(buf);
	}
	return last_number();
}

// init_char_mask
void
PathTokenizer::init_char_mask(char* mask, const char* char_set)
{
	memset(mask, 0, 256/8);
	while (*char_set) {
		unsigned c = unsigned(*char_set++) & 0xFF;
		mask[c >> 3] |= 1 << (c & 7);
	}
}

// contains
inline bool
PathTokenizer::contains(const char* mask, unsigned c) const
{
	return (mask[(c >> 3) & (256 / 8 - 1)] & (1 << (c & 7))) != 0;
}

// is_command
inline bool
PathTokenizer::is_command(unsigned c) const
{
	return contains(fCommandsMask, c);
}

// isNumeric
inline bool
PathTokenizer::isNumeric(unsigned c) const
{
	return contains(fNumericMask, c);
}

// is_separator
inline bool
PathTokenizer::is_separator(unsigned c) const
{
	return contains(fSeparatorsMask, c);
}

// parse_number
bool
PathTokenizer::parse_number()
{
	char* end;
	fLastNumber = strtod(fPath, &end);
	fPath = end;
	return true;
}


} //namespace svg
} //namespace agg




