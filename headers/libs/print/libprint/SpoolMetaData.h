/*
 * SpoolMetaData.h
 * Copyright 2003 Michael Pfeiffer. All Rights Reserved.
 */

#ifndef __SPOOLMETADATA_H
#define __SPOOLMETADATA_H

#include <SupportDefs.h>
#include <File.h>
#include <string>

using namespace std;

class SpoolMetaData {
public:
						SpoolMetaData(BFile* spool_file);
						~SpoolMetaData();

	const string&		GetDescription() const;
	const string&		GetMimeType() const;
	const string&		GetCreationTime() const;

private:
	string fDescription;
	string fMimeType;
	string fCreationTime;
};


#endif	/* __SpoolMetaData_H */
