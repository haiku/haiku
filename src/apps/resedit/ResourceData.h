/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#ifndef RESOURCE_DATA_H
#define RESOURCE_DATA_H

#include <Resources.h>
#include <String.h>
#include <Node.h>
#include <fs_attr.h>

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
	int32			GetID(void) const { return fID; }
	const char *	GetIDString(void) const { return fIDString.String(); }
	const char *	GetName(void) const { return fName.String(); }
	char *			GetData(void) { return fData; }
	size_t			GetLength(void) const  { return fLength; }
	
	bool			IsAttribute(void) const { return fFree; }
	
private:
	void			MakeTypeString(void);
	
	int32			fType;
	BString			fTypeString;
	int32			fID;
	BString			fIDString;
	BString			fName;
	char			*fData;
	size_t			fLength;
	bool			fFree;
};

#endif
