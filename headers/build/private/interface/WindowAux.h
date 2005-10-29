//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		WindowAux.h
//	Author:			Adrian Oanca (oancaadrian@yahoo.com)
//	Description:	BWindowAux.h contains helper definitions for BWindow class
//
//------------------------------------------------------------------------------

#ifndef	_WINDOWAUX_H
#define	_WINDOWAUX_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Message.h>
#include <Handler.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


struct  _BCmdKey{
	uint32		key;
	uint32		modifiers;
	BMessage*	message;
	BHandler*	target;
	int32		targetToken;

	_BCmdKey(uint32 k, uint32 mod, BMessage* m = NULL)
		: key(k),
		  modifiers(mod),
		  message(m),
		  target(NULL),
		  targetToken(B_ANY_TOKEN)
	{
	}
	~_BCmdKey()
	{
		delete message;
	}

};

#endif // _WINDOWAUX_H
