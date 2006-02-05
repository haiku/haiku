/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <map>
#include <stack>

#include <Autolock.h>

#include "TokenSpace.h"


namespace BPrivate {

BTokenSpace gDefaultTokens;
	// the default token space - all handlers will go into that one


BTokenSpace::BTokenSpace()
	:
	fTokenCount(1)
{
}


BTokenSpace::~BTokenSpace()
{
}


int32
BTokenSpace::NewToken(int16 type, void* object,
	new_token_callback callback)
{
	BAutolock locker(this);

	token_info tokenInfo = { type, object };
	int32 token = fTokenCount++;

	fTokenMap[token] = tokenInfo;

	if (callback)
		callback(type, object);

	return token;
}


/*!
	Inserts the specified token into the token space. If that token
	already exists, it will be overwritten.
	Don't mix NewToken() and this method unless you know what you're
	doing.
*/
void
BTokenSpace::SetToken(int32 token, int16 type, void* object)
{
	BAutolock locker(this);
	token_info tokenInfo = { type, object };

	fTokenMap[token] = tokenInfo;

	// this makes sure SetToken() plays more or less nice with NewToken()
	if (token >= fTokenCount)
		fTokenCount = token + 1;
}


bool
BTokenSpace::RemoveToken(int32 token, remove_token_callback callback)
{
	BAutolock locker(this);

	TokenMap::iterator iterator = fTokenMap.find(token);
	if (iterator == fTokenMap.end())
		return false;

	if (callback)
		callback(iterator->second.type, iterator->second.object);

	fTokenMap.erase(iterator);
	return true;
}


/*!	Checks wether or not the \a token exists with the specified
	\a type in the token space or not.
*/
bool
BTokenSpace::CheckToken(int32 token, int16 type) const
{
	BAutolock locker(const_cast<BTokenSpace&>(*this));

	TokenMap::const_iterator iterator = fTokenMap.find(token);
	if (iterator != fTokenMap.end() && iterator->second.type == type)
		return true;

	return false;
}


status_t
BTokenSpace::GetToken(int32 token, int16 type, void** _object,
	get_token_callback callback) const
{
	BAutolock locker(const_cast<BTokenSpace&>(*this));

	if (token < 1)
		return B_ENTRY_NOT_FOUND;

	TokenMap::const_iterator iterator = fTokenMap.find(token);

	if (iterator == fTokenMap.end() || iterator->second.type != type)
		return B_ENTRY_NOT_FOUND;

	if (callback && !callback(iterator->second.type, iterator->second.object))
		return B_ENTRY_NOT_FOUND;

	*_object = iterator->second.object;
	return B_OK;
}

}	// namespace BPrivate
