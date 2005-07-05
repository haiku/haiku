/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 */


#include <map>
#include <stack>

#include <Autolock.h>

#include "TokenSpace.h"


namespace BPrivate {

BTokenSpace gDefaultTokens;
	// the one and only token space object per team


BTokenSpace::BTokenSpace()
{
}


BTokenSpace::~BTokenSpace()
{
}


int32
BTokenSpace::NewToken(int16 type, void* object,
	new_token_callback callback)
{
	BAutolock Lock(fLocker);

	TTokenInfo tokenInfo = { type, object };
	int32 token;

	if (fTokenBin.empty()) {
		token = fTokenCount;
		++fTokenCount;
	} else {
		token = fTokenBin.top();
		fTokenBin.pop();
	}

	fTokenMap[token] = tokenInfo;

	if (callback)
		callback(type, object);

	return token;
}


bool
BTokenSpace::RemoveToken(int32 token, remove_token_callback callback)
{
	BAutolock Lock(fLocker);

	TTokenMap::iterator iter = fTokenMap.find(token);
	if (iter == fTokenMap.end())
		return false;

	if (callback)
		callback(iter->second.type, iter->second.object);

	fTokenMap.erase(iter);
	fTokenBin.push(token);

	return true;
}


/**	Checks wether or not the \a token exists with the specified
 *	\a type in the token space or not.
 */

bool
BTokenSpace::CheckToken(int32 token, int16 type) const
{
	BAutolock Locker(const_cast<BLocker&>(fLocker));

	TTokenMap::const_iterator iter = fTokenMap.find(token);
	if (iter != fTokenMap.end() && iter->second.type == type)
		return true;

	return false;
}


status_t
BTokenSpace::GetToken(int32 token, int16 type, void** object,
	get_token_callback callback) const
{
	BAutolock Locker(const_cast<BLocker&>(fLocker));

	TTokenMap::const_iterator iter = fTokenMap.find(token);
	if (iter == fTokenMap.end()) {
		*object = NULL;
		return B_ERROR;
	}

	if (callback && !callback(iter->second.type, iter->second.object)) {
		*object = NULL;
		return B_ERROR;
	}

	*object = iter->second.object;

	return B_OK;
}

}	// namespace BPrivate

