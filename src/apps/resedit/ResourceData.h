/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef RESOURCE_DATA_H
#define RESOURCE_DATA_H

#include <fs_attr.h>
#include <Resources.h>
#include <Node.h>
#include <String.h>

class ResourceData
{
public:
					ResourceData(void);
					ResourceData(const type_code &code, const int32 &id,
								 const char *name, char *data,
								 const size_t &length);
					ResourceData(const ResourceData &data);
					~ResourceData(void);
	void			SetTo(const type_code &code, const int32 &id,
						 const char *name, char *data, const size_t &length);
	ResourceData &	operator=(const ResourceData &data);
	
	bool			SetFromResource(const int32 &index, BResources &res);
	bool			SetFromAttribute(const char *name, BNode &node);
	
	type_code		GetType(void) const { return fType; }
	const char *	GetTypeString(void) const { return fTypeString.String(); }
	void			SetType(const type_code &code);
	
	int32			GetID(void) const { return fID; }
	const char *	GetIDString(void) const { return fIDString.String(); }
	void			SetID(const int32 &id);
	
	const char *	GetName(void) const { return fName.String(); }
	void			SetName(const char *name) { fName = name; }
	
	char *			GetData(void) { return fData; }
	size_t			GetLength(void) const  { return fLength; }
	void			SetData(const char *data, const size_t &size);
	
	bool			IsAttribute(void) const { return fAttr; }
	void			SetAttribute(const bool &value) { fAttr = value; }
	
private:
	int32			fType;
	BString			fTypeString;
	int32			fID;
	BString			fIDString;
	BString			fName;
	char			*fData;
	size_t			fLength;
	bool			fAttr;
};

#endif
