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

#include <PthreadMutexLocker.h>


namespace BPrivate {

BTokenSpace gDefaultTokens;
	// the default token space - all handlers will go into that one


static int32
get_next_token(int32 token)
{
	if (token == INT32_MAX)
		return 1;
	else
		return token + 1;
}


BTokenSpace::BTokenSpace()
	:
	fNextToken(1)
{
	fLock = (pthread_mutex_t)PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
}


BTokenSpace::~BTokenSpace()
{
	pthread_mutex_destroy(&fLock);
}


int32
BTokenSpace::NewToken(int16 type, void* object)
{
	PthreadMutexLocker locker(&fLock);

	token_info tokenInfo = { type, object, NULL };

	int32 wraparoundToken = fNextToken;

	try {
		for (;;) {
			int32 token = fNextToken;
			bool done = fTokenMap.insert(std::make_pair(token, tokenInfo)).second;
			fNextToken = get_next_token(token);

			if (done)
				return token;

			if (fNextToken == wraparoundToken)
				return -1;
		}
	} catch (std::bad_alloc& exception) {
		return -1;
	}
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
	PthreadMutexLocker locker(&fLock);

	token_info tokenInfo = { type, object, NULL };

	try {
		fTokenMap[token] = tokenInfo;
	} catch (std::bad_alloc& exception) {
		return false;
	}

	// this makes sure SetToken() plays more or less nice with NewToken()
	if (token >= fNextToken)
		fNextToken = get_next_token(token);

	return true;
}


bool
BTokenSpace::RemoveToken(int32 token)
{
	PthreadMutexLocker locker(&fLock);

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
	PthreadMutexLocker locker(&fLock);

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

	PthreadMutexLocker locker(&fLock);

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

	PthreadMutexLocker locker(&fLock);

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

	PthreadMutexLocker locker(&fLock);

	TokenMap::const_iterator iterator = fTokenMap.find(token);
	if (iterator == fTokenMap.end() || iterator->second.type != B_HANDLER_TOKEN)
		return B_ENTRY_NOT_FOUND;

	if (iterator->second.target != NULL)
		iterator->second.target->Acquire();

	*_target = iterator->second.target;
	return B_OK;
}


}	// namespace BPrivate
