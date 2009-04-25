/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DataSource.h"

#include <new>

#include <String.h>


// #pragma mark - DataSource


DataSource::DataSource()
{
}


DataSource::~DataSource()
{
}


status_t
DataSource::GetName(BString& name)
{
	return B_UNSUPPORTED;
}


// #pragma mark - FileDataSource



status_t
FileDataSource::CreateDataIO(BDataIO** _io)
{
	BFile* file = new(std::nothrow) BFile;
	if (file == NULL)
		return B_NO_MEMORY;

	status_t error = OpenFile(*file);
	if (error != B_OK) {
		delete file;
		return error;
	}

	*_io = file;
	return B_OK;
}


// #pragma mark - PathDataSource


status_t
PathDataSource::Init(const char* path)
{
	return fPath.SetTo(path);
}


status_t
PathDataSource::GetName(BString& name)
{
	if (fPath.Path() == NULL)
		return B_NO_INIT;

	name = fPath.Path();
	return B_OK;
}


status_t
PathDataSource::OpenFile(BFile& file)
{
	return file.SetTo(fPath.Path(), B_READ_ONLY);
}


// #pragma mark - EntryRefDataSource


status_t
EntryRefDataSource::Init(const entry_ref* ref)
{
	if (ref->name == NULL)
		return B_BAD_VALUE;

	fRef = *ref;
	if (fRef.name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
EntryRefDataSource::GetName(BString& name)
{
	BEntry entry;
	status_t error = entry.SetTo(&fRef);
	if (error != B_OK)
		return error;

	BPath path;
	error = entry.GetPath(&path);
	if (error != B_OK)
		return error;

	name = path.Path();
	return B_OK;
}


status_t
EntryRefDataSource::OpenFile(BFile& file)
{
	return file.SetTo(&fRef, B_READ_ONLY);
}
