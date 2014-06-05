/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <HttpResult.h>
#include <Debug.h>


using std::ostream;


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


size_t
BHttpResult::Length() const
{
	const char* length = Headers()["Content-Length"];
	if (length == NULL)
		return 0;
	return atoi(length);
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
