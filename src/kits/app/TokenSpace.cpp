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
//	File Name:		TokenSpace.cpp
//	Author(s):		Erik Jaesler <erik@cgsoftware.com>
//	Description:	Class for creating tokens
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <map>
#include <stack>

// System Includes -------------------------------------------------------------
#include <Autolock.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "TokenSpace.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

namespace BPrivate {

BTokenSpace gDefaultTokens;


//------------------------------------------------------------------------------
BTokenSpace::BTokenSpace()
{
}
//------------------------------------------------------------------------------
BTokenSpace::~BTokenSpace()
{
}
//------------------------------------------------------------------------------
int32 BTokenSpace::NewToken(int16 type, void* object,
							new_token_callback callback)
{
	BAutolock Lock(fLocker);
	TTokenInfo ti = { type, object };
	int32 token;
	if (fTokenBin.empty())
	{
		token = fTokenCount;
		++fTokenCount;
	}
	else
	{
		token = fTokenBin.top();
		fTokenBin.pop();
	}

	fTokenMap[token] = ti;

	if (callback)
	{
		callback(type, object);
	}

	return token;
}
//------------------------------------------------------------------------------
bool BTokenSpace::RemoveToken(int32 token, remove_token_callback callback)
{
	BAutolock Lock(fLocker);
	TTokenMap::iterator iter = fTokenMap.find(token);
	if (iter == fTokenMap.end())
	{
		return false;
	}

	if (callback)
	{
		callback(iter->second.type, iter->second.object);
	}

	fTokenMap.erase(iter);
	fTokenBin.push(token);

	return true;
}
//------------------------------------------------------------------------------
bool BTokenSpace::CheckToken(int32 token, int16 type) const
{
	BAutolock Locker(const_cast<BLocker&>(fLocker));
	TTokenMap::const_iterator iter = fTokenMap.find(token);
	if (iter != fTokenMap.end() && iter->second.type == type)
	{
		return true;
	}

	return false;
}
//------------------------------------------------------------------------------
status_t BTokenSpace::GetToken(int32 token, int16 type, void** object,
							   get_token_callback callback) const
{
	BAutolock Locker(const_cast<BLocker&>(fLocker));
	TTokenMap::const_iterator iter = fTokenMap.find(token);
	if (iter == fTokenMap.end())
	{
		*object = NULL;
		return B_ERROR;
	}

	if (callback && !callback(iter->second.type, iter->second.object))
	{
		*object = NULL;
		return B_ERROR;
	}

	*object = iter->second.object;

	return B_OK;
}
//------------------------------------------------------------------------------

}	// namespace BPrivate

/*
 * $Log $
 *
 * $Id  $
 *
 */

