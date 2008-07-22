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
	typedef std::string string;
private:
	string fDescription;
	string fMimeType;
	string fCreationTime;

public:
	SpoolMetaData(BFile* spool_file);
	~SpoolMetaData();

	const string& getDescription() const { return fDescription; }
	const string& getMimeType() const { return fMimeType; }
	const string& getCreationTime() const { return fCreationTime; }
};

#endif	/* __SpoolMetaData_H */
