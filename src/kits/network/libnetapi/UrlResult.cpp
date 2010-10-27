/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <UrlResult.h>
#include <Debug.h>


using std::ostream;


BUrlResult::BUrlResult(const BUrl& url)
	:
	fUrl(url),
	fRawData(),
	fHeaders()
{
}


BUrlResult::BUrlResult(const BUrlResult& other)
	:
	fUrl(),
	fRawData(),
	fHeaders()
{
	*this = other;
}


// #pragma mark Result parameters modifications


void
BUrlResult::SetUrl(const BUrl& url)
{
	fUrl = url;
}


// #pragma mark Result parameters access


const BUrl&
BUrlResult::Url() const
{
	return fUrl;
}


const BMallocIO&
BUrlResult::RawData() const
{
	return fRawData;
}


const BHttpHeaders&
BUrlResult::Headers() const
{
	return fHeaders;
}


int32
BUrlResult::StatusCode() const
{
	return fStatusCode;
}


const BString&
BUrlResult::StatusText() const
{
	return fStatusString;
}


// #pragma mark Result tests


bool
BUrlResult::HasHeaders() const
{
	return (fHeaders.CountHeaders() > 0);
}


// #pragma mark Overloaded operators


BUrlResult&
BUrlResult::operator=(const BUrlResult& other)
{
	fUrl = other.fUrl;
	fHeaders = other.fHeaders;
	
	fRawData.SetSize(other.fRawData.BufferLength());
	fRawData.WriteAt(0, fRawData.Buffer(), fRawData.BufferLength());
	
	return *this;
}


ostream& 	
operator<<(ostream& out, const BUrlResult& result)
{
	out.write(reinterpret_cast<const char*>(result.fRawData.Buffer()),
		result.fRawData.BufferLength());
	
	return out;
}
