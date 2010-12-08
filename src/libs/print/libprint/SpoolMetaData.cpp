/*
 * SpoolMetaData.cpp
 * Copyright 2003 Michael Pfeiffer. All Rights Reserved.
 */

#include "SpoolMetaData.h"
#include <String.h>

const char *kSDDescription             = "_spool/Description";
const char* kSDMimeType                = "_spool/MimeType";


SpoolMetaData::SpoolMetaData(BFile* spool_file) 
{
	BString string;
	time_t time;
	if (spool_file->ReadAttrString(kSDDescription, &string) == B_OK)
		fDescription = string.String();

	if (spool_file->ReadAttrString(kSDMimeType, &string) == B_OK)
		fMimeType = string.String();

	if (spool_file->GetCreationTime(&time) == B_OK)
		fCreationTime = ctime(&time);
}


SpoolMetaData::~SpoolMetaData()
{
}


const string&
SpoolMetaData::GetDescription() const
{
	return fDescription;
}


const string&
SpoolMetaData::GetMimeType() const
{
	return fMimeType;
}


const string&
SpoolMetaData::GetCreationTime() const
{
	return fCreationTime;
}
