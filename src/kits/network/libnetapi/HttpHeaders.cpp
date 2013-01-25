/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <ctype.h>
#include <string.h>
#include <new>

#include <String.h>
#include <HttpHeaders.h>


// #pragma mark -- BHttpHeader


BHttpHeader::BHttpHeader()
	:
	fName(),
	fValue(),
	fRawHeader(),
	fRawHeaderValid(true)
{
}


BHttpHeader::BHttpHeader(const char* string)
	:
	fRawHeaderValid(true)
{
	SetHeader(string);
}


BHttpHeader::BHttpHeader(const char* name, const char* value)
	:
	fRawHeaderValid(false)
{
	SetName(name);
	SetValue(value);
}


BHttpHeader::BHttpHeader(const BHttpHeader& copy)
	:
	fName(copy.fName),
	fValue(copy.fValue),
	fRawHeaderValid(false)
{
}


void
BHttpHeader::SetName(const char* name)
{
	fRawHeaderValid = false;
	fName = name;
	fName.Trim().CapitalizeEachWord();
}


void
BHttpHeader::SetValue(const char* value)
{
	fRawHeaderValid = false;
	fValue = value;
	fValue.Trim();
}


bool
BHttpHeader::SetHeader(const char* string)
{
	fRawHeaderValid = false;
	fName.Truncate(0);
	fValue.Truncate(0);

	const char* separator = strchr(string, ':');

	if (separator == NULL)
		return false;

	fName.SetTo(string, separator - string);
	fName.Trim().CapitalizeEachWord();
	SetValue(separator + 1);
	return true;
}


const char*
BHttpHeader::Name() const
{
	return fName.String();
}


const char*
BHttpHeader::Value() const
{
	return fValue.String();
}


const char*
BHttpHeader::Header() const
{
	if (!fRawHeaderValid) {
		fRawHeaderValid = true;

		fRawHeader.Truncate(0);
		fRawHeader << fName << ": " << fValue;
	}

	return fRawHeader.String();
}


bool
BHttpHeader::NameIs(const char* name) const
{
	return fName == BString(name).Trim().CapitalizeEachWord();
}


BHttpHeader&
BHttpHeader::operator=(const BHttpHeader& other)
{
	fName = other.fName;
	fValue = other.fValue;
	fRawHeaderValid = false;

	return *this;
}


// #pragma mark -- BHttpHeaders


BHttpHeaders::BHttpHeaders()
	:
	fHeaderList()
{
}


BHttpHeaders::BHttpHeaders(const BHttpHeaders& copy)
	:
	fHeaderList()
{
	for (int32 i = 0; i < copy.CountHeaders(); i++)
		AddHeader(copy.HeaderAt(i).Name(), copy.HeaderAt(i).Value());
}


BHttpHeaders::~BHttpHeaders()
{
	_EraseData();
}


// #pragma mark Header access


const char*
BHttpHeaders::HeaderValue(const char* name) const
{
	for (int32 i = 0; i < fHeaderList.CountItems(); i++) {
		BHttpHeader* header
			= reinterpret_cast<BHttpHeader*>(fHeaderList.ItemAtFast(i));

		if (header->NameIs(name))
			return header->Value();
	}

	return NULL;
}


BHttpHeader&
BHttpHeaders::HeaderAt(int32 index) const
{
	//! Note: index _must_ be in-bounds
	BHttpHeader* header
		= reinterpret_cast<BHttpHeader*>(fHeaderList.ItemAtFast(index));

	return *header;
}


// #pragma mark Header count


int32
BHttpHeaders::CountHeaders() const
{
	return fHeaderList.CountItems();
}


// #pragma Header tests


int32
BHttpHeaders::HasHeader(const char* name) const
{
	for (int32 i = 0; i < fHeaderList.CountItems(); i++) {
		BHttpHeader* header
			= reinterpret_cast<BHttpHeader*>(fHeaderList.ItemAt(i));

		if (header->NameIs(name))
			return i;
	}

	return B_ERROR;
}


// #pragma mark Header add/replace


bool
BHttpHeaders::AddHeader(const char* line)
{
	BHttpHeader* heapHeader = new(std::nothrow) BHttpHeader(line);

	if (heapHeader != NULL) {
		fHeaderList.AddItem(heapHeader);
		return true;
	}

	return false;
}


bool
BHttpHeaders::AddHeader(const char* name, const char* value)
{
	BHttpHeader* heapHeader = new(std::nothrow) BHttpHeader(name, value);

	if (heapHeader != NULL) {
		fHeaderList.AddItem(heapHeader);
		return true;
	}

	return false;
}


bool
BHttpHeaders::AddHeader(const char* name, int32 value)
{
	BString strValue;
	strValue << value;

	return AddHeader(name, strValue);
}


// #pragma mark Header deletion


void
BHttpHeaders::Clear()
{
	_EraseData();
	fHeaderList.MakeEmpty();
}


// #pragma mark Overloaded operators


BHttpHeaders&
BHttpHeaders::operator=(const BHttpHeaders& other)
{
	for (int32 i = 0; i < other.CountHeaders(); i++)
		AddHeader(other.HeaderAt(i).Name(), other.HeaderAt(i).Value());

	return *this;
}


BHttpHeader&
BHttpHeaders::operator[](int32 index) const
{
	//! Note: Index _must_ be in-bounds
	BHttpHeader* header
		= reinterpret_cast<BHttpHeader*>(fHeaderList.ItemAtFast(index));

	return *header;
}


const char*
BHttpHeaders::operator[](const char* name) const
{
	return HeaderValue(name);
}


void
BHttpHeaders::_EraseData()
{
	// Free allocated data;
	for (int32 i = 0; i < fHeaderList.CountItems(); i++) {
		BHttpHeader* header
			= reinterpret_cast<BHttpHeader*>(fHeaderList.ItemAtFast(i));

		delete header;
	}
}
