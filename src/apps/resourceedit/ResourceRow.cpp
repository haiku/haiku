/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ResourceRow.h"

#include <ColumnListView.h>
#include <ColumnTypes.h>


ResourceRow::ResourceRow()
	:
	BRow()
{
	fRawData = NULL;

	SetField(new BIntegerField(0), 0);
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
	((BIntegerField*)GetField(0))->SetValue(id);
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
ResourceRow::SetResourceTypeCode(type_code code)
{
	fTypeCode = code;
	TypeCodeToString(code, fTypeString);
	((BStringField*)GetField(3))->SetString(fTypeString);
}


void
ResourceRow::SetResourceData(const char* data)
{
	((BStringField*)GetField(4))->SetString(data);
}


void
ResourceRow::SetResourceRawData(const void* data)
{
	if (data == NULL)
		data = kDefaultData;

	fRawData = data;

	int32 ix = FindTypeCodeIndex(ResourceTypeCode());

	if (ix == -1)
		SetResourceData("[Unknown Data]");
	else
		SetResourceData(kDefaultTypes[ix].toString(fRawData));
}


void
ResourceRow::SetResourceSize(off_t size)
{
	((BSizeField*)GetField(5))->SetSize(size);
}


int32
ResourceRow::ResourceID()
{
	return ((BIntegerField*)GetField(0))->Value();
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
ResourceRow::ResourceTypeCode()
{
	return fTypeCode;
}


const char*
ResourceRow::ResourceData()
{
	return ((BStringField*)GetField(4))->String();
}


const void*
ResourceRow::ResourceRawData()
{
	return fRawData;
}


off_t
ResourceRow::ResourceSize()
{
	return ((BSizeField*)GetField(5))->Size();
}
