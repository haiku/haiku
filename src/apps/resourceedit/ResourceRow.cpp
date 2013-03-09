/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ResourceRow.h"

#include <ColumnListView.h>
#include <ColumnTypes.h>

#include <stdlib.h>


ResourceRow::ResourceRow()
	:
	BRow()
{
	Parent = NULL;
	ActionIndex = -1;

	SetField(new BStringField(""), 0);
	SetField(new BStringField(""), 1);
	SetField(new BStringField(""), 2);
	SetField(new BStringField(""), 3);
	SetField(new BStringField(""), 4);
	SetField(new BSizeField(0), 5);
}


ResourceRow::~ResourceRow()
{
	// ...
}


void
ResourceRow::SetResourceID(int32 id)
{
	((BStringField*)GetField(0))->SetString((BString() << id).String());
}


void
ResourceRow::SetResourceStringID(const char* id)
{
	((BStringField*)GetField(0))->SetString(id);
}


void
ResourceRow::SetResourceName(const char* name)
{
	((BStringField*)GetField(1))->SetString(name);
}


void
ResourceRow::SetResourceType(const char* type)
{
	((BStringField*)GetField(2))->SetString(type);
}


void
ResourceRow::SetResourceCode(type_code code)
{
	fCode = code;
	ResourceType::CodeToString(code, fTypeString);
	((BStringField*)GetField(3))->SetString(fTypeString);
}


void
ResourceRow::SetResourceStringCode(const char* code)
{
	fCode = ResourceType::StringToCode(code);
	((BStringField*)GetField(3))->SetString(code);
}


void
ResourceRow::SetResourceData(const char* data)
{
	((BStringField*)GetField(4))->SetString(data);
}


void
ResourceRow::SetResourceSize(off_t size)
{
	((BSizeField*)GetField(5))->SetSize(size);
}


int32
ResourceRow::ResourceID()
{
	const char* strID = ResourceStringID();

	// TODO: Check whether is numeric and resolve if not.

	return atoi(strID);
}


const char*
ResourceRow::ResourceStringID()
{
	return ((BStringField*)GetField(0))->String();
}


const char*
ResourceRow::ResourceName()
{
	return ((BStringField*)GetField(1))->String();
}


const char*
ResourceRow::ResourceType()
{
	return ((BStringField*)GetField(2))->String();
}


type_code
ResourceRow::ResourceCode()
{
	return fCode;
}


const char*
ResourceRow::ResourceStringCode()
{
	return 	((BStringField*)GetField(3))->String();
}


const char*
ResourceRow::ResourceData()
{
	return ((BStringField*)GetField(4))->String();
}


off_t
ResourceRow::ResourceSize()
{
	return ((BSizeField*)GetField(5))->Size();
}
