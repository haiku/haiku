/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */

#include <HttpResult.h>

#include <errno.h>
#include <Debug.h>

using namespace BPrivate::Network;


BHttpResult::BHttpResult(const BUrl& url)
	:
	fUrl(url),
	fHeaders(),
	fStatusCode(0)
{
}


BHttpResult::BHttpResult(const BHttpResult& other)
	:
	fUrl(other.fUrl),
	fHeaders(other.fHeaders),
	fStatusCode(other.fStatusCode),
	fStatusString(other.fStatusString)
{
}


BHttpResult::~BHttpResult()
{
}


// #pragma mark Result parameters modifications


void
BHttpResult::SetUrl(const BUrl& url)
{
	fUrl = url;
}


// #pragma mark Result parameters access


const BUrl&
BHttpResult::Url() const
{
	return fUrl;
}


BString
BHttpResult::ContentType() const
{
	return Headers()["Content-Type"];
}


off_t
BHttpResult::Length() const
{
	const char* length = Headers()["Content-Length"];
	if (length == NULL)
		return 0;

	/* NOTE: Not RFC7230 compliant:
	 * - If Content-Length is a list, all values must be checked and verified
	 *   to be duplicates of each other, but this is currently not supported.
	 */
	off_t result = 0;
	/* strtoull() will ignore a prefixed sign, so we verify that there aren't
	 * any before continuing (RFC7230 only permits digits).
	 *
	 * We can check length[0] directly because header values are trimmed by
	 * HttpHeader beforehand. */
	if (length[0] != '-' && length[0] != '+') {
		errno = 0;
		char *endptr = NULL;
		result = strtoull(length, &endptr, 10);
		/* ERANGE will be signalled if the result is too large (which can
		 * happen), in that case, return 0. */
		if (errno != 0 || *endptr != '\0')
			result = 0;
	}
	return result;
}


const BHttpHeaders&
BHttpResult::Headers() const
{
	return fHeaders;
}


int32
BHttpResult::StatusCode() const
{
	return fStatusCode;
}


const BString&
BHttpResult::StatusText() const
{
	return fStatusString;
}


// #pragma mark Result tests


bool
BHttpResult::HasHeaders() const
{
	return fHeaders.CountHeaders() > 0;
}


// #pragma mark Overloaded members


BHttpResult&
BHttpResult::operator=(const BHttpResult& other)
{
	if (this == &other)
		return *this;

	fUrl = other.fUrl;
	fHeaders = other.fHeaders;
	fStatusCode = other.fStatusCode;
	fStatusString = other.fStatusString;

	return *this;
}
