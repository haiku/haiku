/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#include "ResourceData.h"
#include <stdlib.h>

ResourceData::ResourceData(void)
  :	fType(0),
  	fTypeString("Invalid"),
  	fID(-1),
  	fIDString("Invalid"),
  	fName(""),
  	fData(NULL),
  	fLength(0),
  	fFree(false)
{
}


ResourceData::ResourceData(const type_code &code, const int32 &id,
							const char *name, char *data,
							const size_t &length)
  :	fType(code),
	fID(id),
	fName(name),
	fData(data),
	fLength(length),
  	fFree(false)
{
	fIDString = "";
	fIDString << fID;
	MakeTypeString();
}


ResourceData::ResourceData(const ResourceData &data)
{
	*this = data;
}


ResourceData::~ResourceData(void)
{
	if (fFree)
		free(fData);
}


ResourceData &
ResourceData::operator=(const ResourceData &data)
{
	fType = data.fType;
	fTypeString = data.fTypeString;
	fID = data.fID;
	fIDString = data.fIDString;
	fName = data.fName;
	fLength = data.fLength;
	fFree = data.fFree;
	
	// This is an attribute, so we need to allocate and free the data
	if (fFree) {
		fData = data.fData;
	}
	return *this;
}

bool
ResourceData::SetFromResource(const int32 &index, BResources &res)
{
	char *name;
	if (!res.GetResourceInfo(index, (type_code*)&fType, &fID,
							(const char **)&name, &fLength)) {
		*this = ResourceData();
		return false;
	}
	fName = name;
	MakeTypeString();
	fIDString = "";
	fIDString << fID;
	fFree = false;
	fData = (char *)res.LoadResource(fType,fID,&fLength);
	
	return true;
}


bool
ResourceData::SetFromAttribute(const char *name, BNode &node)
{
	attr_info info;
	if (node.GetAttrInfo(name, &info) != B_OK) {
		*this = ResourceData();
		return false;
	}
	
	fType = info.type;
	fID = -1;
	fIDString = "(attr)";
	fName = name;
	fLength = info.size;
	fFree = true;

	MakeTypeString();
	
	fData = (char *)malloc(fLength);
	if (fData) {
		ssize_t size = node.ReadAttr(name, info.type, 0, (void*)fData, fLength);
		if (size >= 0) {
			fLength = (size_t) size;
			return true;
		}
	}
	
	*this = ResourceData();
	return false;
}


void
ResourceData::SetTo(const type_code &code, const int32 &id,
					const char *name, char *data, const size_t &length)
{
	fType = code;
	fID = id;
	fName = name;
	fData = data;
	fLength = length;
	
	MakeTypeString();
}


void
ResourceData::MakeTypeString(void)
{
	char typestring[7];
	char *typeptr = (char *) &fType;
	typestring[0] = '\'';
	typestring[1] = typeptr[3];
	typestring[2] = typeptr[2];
	typestring[3] = typeptr[1];
	typestring[4] = typeptr[0];
	typestring[5] = '\'';
	typestring[6] = '\0';
	fTypeString = typestring;
}

