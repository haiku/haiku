//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
//	File Name:		TokenHandler.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class to provide tokens with excluded values possible.
//  
//------------------------------------------------------------------------------
#ifndef TOKENHANDLER_H_
#define TOKENHANDLER_H_

#include <OS.h>
#include <List.h>
#include <Locker.h>

// Local Defines ---------------------------------------------------------------
#define B_PREFERRED_TOKEN	-2		/* A little bird told me about this one */
#define B_NULL_TOKEN		-1
#define B_ANY_TOKEN			0
#define B_HANDLER_TOKEN		1

/*!
	\class TokenHandler TokenHandler.h
	\brief Class to provide tokens with excluded values possible.
*/
class TokenHandler
{
public:
	TokenHandler(void);
	~TokenHandler(void);
	int32 GetToken(void);
	void ExcludeValue(int32 value);
	void Reset(void);
	void ResetExcludes(void);
	bool IsExclude(int32 value);
private:
	int32 _index;
	BLocker _lock;
	BList *_excludes;
};

#endif
