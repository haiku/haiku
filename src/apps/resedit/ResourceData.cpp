/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "ResourceData.h"
#include "ResFields.h"
#include <stdlib.h>

ResourceData::ResourceData(void)
  :	fType(0),
  	fTypeString("Invalid"),
  	fID(-1),
  	fIDString("Invalid"),
  	fName(""),
  	fData(NULL),
  	fLength(0),
  	fAttr(false)
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
  	fAttr(false)
{
	fIDString = "";
	fIDString << fID;
	fTypeString = MakeTypeString(code);
}


ResourceData::ResourceData(const ResourceData &data)
{
	*this = data;
}


ResourceData::~ResourceData(void)
{
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
	fAttr = data.fAttr;
	SetData(data.fData, data.fLength);
	
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
	fTypeString = MakeTypeString(fType);
	fIDString = "";
	fIDString << fID;
	fAttr = false;
	char *data = (char *)res.LoadResource(fType, fID, &fLength);
	SetData(data, fLength);
	
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
	fAttr = true;

	fTypeString = MakeTypeString(fType);
	
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
	fTypeString = MakeTypeString(code);
	fID = id;
	fIDString = "";
	fIDString << fID;
	fName = name;
	SetData(data, length);
	
}


void
ResourceData::SetType(const type_code &code)
{
	fType = code;
	fTypeString = MakeTypeString(code);
}


void
ResourceData::SetID(const int32 &id)
{
	fID = id;
	fIDString = "";
	fIDString << fID;
}


void
ResourceData::SetData(const char *data, const size_t &size)
{
	free(fData);
	
	fLength = size;
	
	if (size > 0) {
		fData = (char *)malloc(size);
		memcpy(fData, data, size);
	}
	else
		fData = NULL;
}

