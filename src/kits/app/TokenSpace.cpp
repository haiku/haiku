/*
 * Copyright 2001-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <DirectMessageTarget.h>
#include <TokenSpace.h>

#include <Autolock.h>


namespace BPrivate {

BTokenSpace gDefaultTokens;
	// the default token space - all handlers will go into that one


BTokenSpace::BTokenSpace()
	:
	BLocker("token space"),
	fTokenCount(1)
{
}


BTokenSpace::~BTokenSpace()
{
}


int32
BTokenSpace::NewToken(int16 type, void* object)
{
	BAutolock locker(this);

	token_info tokenInfo = { type, object, NULL };
	int32 token = fTokenCount;

	try {
		fTokenMap[token] = tokenInfo;
	} catch (std::bad_alloc& exception) {
		return -1;
	}

	fTokenCount++;

	return token;
}


/*!
	Inserts the specified token into the token space. If that token
	already exists, it will be overwritten.
	Don't mix NewToken() and this method unless you know what you're
	doing.
*/
bool
BTokenSpace::SetToken(int32 token, int16 type, void* object)
{
	BAutolock locker(this);

	token_info tokenInfo = { type, object, NULL };

	try {
		fTokenMap[token] = tokenInfo;
	} catch (std::bad_alloc& exception) {
		return false;
	}

	// this makes sure SetToken() plays more or less nice with NewToken()
	if (token >= fTokenCount)
		fTokenCount = token + 1;

	return true;
}


bool
BTokenSpace::RemoveToken(int32 token)
{
	BAutolock locker(this);

	TokenMap::iterator iterator = fTokenMap.find(token);
	if (iterator == fTokenMap.end())
		return false;

	fTokenMap.erase(iterator);
	return true;
}


/*!	Checks whether or not the \a token exists with the specified
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
BTokenSpace::GetToken(int32 token, int16 type, void** _object) const
{
	if (token < 1)
		return B_ENTRY_NOT_FOUND;

	BAutolock locker(const_cast<BTokenSpace&>(*this));

	TokenMap::const_iterator iterator = fTokenMap.find(token);
	if (iterator == fTokenMap.end() || iterator->second.type != type)
		return B_ENTRY_NOT_FOUND;

	*_object = iterator->second.object;
	return B_OK;
}


status_t
BTokenSpace::SetHandlerTarget(int32 token, BDirectMessageTarget* target)
{
	if (token < 1)
		return B_ENTRY_NOT_FOUND;

	BAutolock locker(const_cast<BTokenSpace&>(*this));

	TokenMap::iterator iterator = fTokenMap.find(token);
	if (iterator == fTokenMap.end() || iterator->second.type != B_HANDLER_TOKEN)
		return B_ENTRY_NOT_FOUND;

	if (iterator->second.target != NULL)
		iterator->second.target->Release();

	iterator->second.target = target;
	if (target != NULL)
		target->Acquire();

	return B_OK;
}


status_t
BTokenSpace::AcquireHandlerTarget(int32 token, BDirectMessageTarget** _target)
{
	if (token < 1)
		return B_ENTRY_NOT_FOUND;

	BAutolock locker(const_cast<BTokenSpace&>(*this));

	TokenMap::const_iterator iterator = fTokenMap.find(token);
	if (iterator == fTokenMap.end() || iterator->second.type != B_HANDLER_TOKEN)
		return B_ENTRY_NOT_FOUND;

	if (iterator->second.target != NULL)
		iterator->second.target->Acquire();

	*_target = iterator->second.target;
	return B_OK;
}


void
BTokenSpace::InitAfterFork()
{
	// We need to reinitialize the locker to get a new semaphore
	new (this) BTokenSpace();
}


}	// namespace BPrivate
