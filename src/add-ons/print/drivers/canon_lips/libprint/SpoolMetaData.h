/*
 * SpoolMetaData.h
 * Copyright 2003 Michael Pfeiffer. All Rights Reserved.
 */

#ifndef __SPOOLMETADATA_H
#define __SPOOLMETADATA_H

#include <SupportDefs.h>
#include <File.h>
#include <string>

class SpoolMetaData {
private:
	string __description;
	string __mime_type;
	string __creation_time;

public:
	SpoolMetaData(BFile* spool_file);
	~SpoolMetaData();

	const string& getDescription() const { return __description; }
	const string& getMimeType() const { return __mime_type; }
	const string& getCreationTime() const { return __creation_time; }
};

#endif	/* __SpoolMetaData_H */
