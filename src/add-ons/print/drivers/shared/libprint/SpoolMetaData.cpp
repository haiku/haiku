/*
 * SpoolMetaData.cpp
 * Copyright 2003 Michael Pfeiffer. All Rights Reserved.
 */

#include "SpoolMetaData.h"
#include <String.h>

const char *SD_DESCRIPTION             = "_spool/Description";
const char* SD_MIME_TYPE               = "_spool/MimeType";

SpoolMetaData::SpoolMetaData(BFile* spool_file) 
{
	BString string;
	time_t time;
	if (spool_file->ReadAttrString(SD_DESCRIPTION, &string) == B_OK) {
		__description = string.String();
	}
	if (spool_file->ReadAttrString(SD_MIME_TYPE, &string) == B_OK) {
		__mime_type = string.String();
	}
	if (spool_file->GetCreationTime(&time) == B_OK) {
		__creation_time = ctime(&time);
	}
}

SpoolMetaData::~SpoolMetaData()
{
}

