//------------------------------------------------------------------------------
//	TokenSpace.cpp
//
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

