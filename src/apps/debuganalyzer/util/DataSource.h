/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <Entry.h>
#include <File.h>
#include <Path.h>


struct BString;


class DataSource {
public:
								DataSource();
	virtual						~DataSource();

	virtual	status_t			CreateDataIO(BDataIO** _io) = 0;

	virtual	status_t			GetName(BString& name);
};


class FileDataSource : public DataSource {
public:
	virtual	status_t			CreateDataIO(BDataIO** _io);

protected:
	virtual	status_t			OpenFile(BFile& file) = 0;
};


class PathDataSource : public FileDataSource {
public:
			status_t			Init(const char* path);

	virtual	status_t			GetName(BString& name);

protected:
	virtual	status_t			OpenFile(BFile& file);

private:
			BPath				fPath;
};


class EntryRefDataSource : public FileDataSource {
public:
			status_t			Init(const entry_ref* ref);

	virtual	status_t			GetName(BString& name);

protected:
	virtual	status_t			OpenFile(BFile& file);

private:
			entry_ref			fRef;
};


#endif	// DATA_SOURCE_H
