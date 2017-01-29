/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
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


BHttpResult::BHttpResult(BMessage* archive)
	:
	BUrlResult(archive)
{
	fUrl = archive->FindString("http:url");
	fStatusCode = archive->FindInt32("http:statusCode");
	fStatusString = archive->FindString("http:statusString");

	BMessage headers;
	archive->FindMessage("http:headers", &headers);
	fHeaders.PopulateFromArchive(&headers);
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


status_t
BHttpResult::Archive(BMessage* target, bool deep) const
{
	status_t result = BUrlResult::Archive(target, deep);
	if (result != B_OK)
		return result;

	target->AddString("http:url", fUrl);
	target->AddInt32("http:statusCode", fStatusCode);
	target->AddString("http:statusString", fStatusString);

	BMessage headers;
	fHeaders.Archive(&headers);
	target->AddMessage("http:headers", &headers);

	return B_OK;
}


/*static*/ BArchivable*
BHttpResult::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "BHttpResult"))
	{
		return NULL;
	}

	return new BHttpResult(archive);
}
