/*
 * Copyright 2001-2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2013 Haiku, Inc.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */


#include <Resources.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>

#include "ResourceFile.h"
#include "ResourceItem.h"
#include "ResourcesContainer.h"


using namespace BPrivate::Storage;
using namespace std;


// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf


// Creates an unitialized BResources object.
BResources::BResources()
	:
	fFile(),
	fContainer(NULL),
	fResourceFile(NULL),
	fReadOnly(false)
{
	fContainer = new(nothrow) ResourcesContainer;
}


// Creates a BResources object that represents the resources of the
// supplied file.
BResources::BResources(const BFile* file, bool clobber)
	:
	fFile(),
	fContainer(NULL),
	fResourceFile(NULL),
	fReadOnly(false)
{
	fContainer = new(nothrow) ResourcesContainer;
	SetTo(file, clobber);
}


// Creates a BResources object that represents the resources of the
// file referenced by the supplied path.
BResources::BResources(const char* path, bool clobber)
	:
	fFile(),
	fContainer(NULL),
	fResourceFile(NULL),
	fReadOnly(false)
{
	fContainer = new(nothrow) ResourcesContainer;
	SetTo(path, clobber);
}


// Creates a BResources object that represents the resources of the
// file referenced by the supplied ref.
BResources::BResources(const entry_ref* ref, bool clobber)
	:
	fFile(),
	fContainer(NULL),
	fResourceFile(NULL),
	fReadOnly(false)
{
	fContainer = new(nothrow) ResourcesContainer;
	SetTo(ref, clobber);
}


// Frees all resources associated with this object
BResources::~BResources()
{
	Unset();
	delete fContainer;
}


// Initialized the BResources object to represent the resources of
// the supplied file.
status_t
BResources::SetTo(const BFile* file, bool clobber)
{
	Unset();
	status_t error = B_OK;
	if (file) {
		error = file->InitCheck();
		if (error == B_OK) {
			fFile = *file;
			error = fFile.InitCheck();
		}
		if (error == B_OK) {
			fReadOnly = !fFile.IsWritable();
			fResourceFile = new(nothrow) ResourceFile;
			if (fResourceFile)
				error = fResourceFile->SetTo(&fFile, clobber);
			else
				error = B_NO_MEMORY;
		}
		if (error == B_OK) {
			if (fContainer)
				error = fResourceFile->InitContainer(*fContainer);
			else
				error = B_NO_MEMORY;
		}
	}
	if (error != B_OK) {
		delete fResourceFile;
		fResourceFile = NULL;
		if (fContainer)
			fContainer->MakeEmpty();
	}
	return error;
}


// Initialized the BResources object to represent the resources of
// the file referred to by the supplied path.
status_t
BResources::SetTo(const char* path, bool clobber)
{
	if (!path)
		return B_BAD_VALUE;

	// open file
	BFile file;
	status_t error = file.SetTo(path, B_READ_WRITE);
	if (error != B_OK) {
		Unset();
		return error;
	}

	// delegate the actual work
	return SetTo(&file, clobber);
}


// Initialized the BResources object to represent the resources of the
// file referenced by the supplied ref.
status_t
BResources::SetTo(const entry_ref* ref, bool clobber)
{
	if (!ref)
		return B_BAD_VALUE;

	// open file
	BFile file;
	status_t error = file.SetTo(ref, B_READ_WRITE);
	if (error != B_OK) {
		Unset();
		return error;
	}

	// delegate the actual work
	return SetTo(&file, clobber);
}


// Initialized the BResources object to represent the resources of
// the file from which the specified image has been loaded.
status_t
BResources::SetToImage(image_id image, bool clobber)
{
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	// get an image info
	image_info info;
	status_t error = get_image_info(image, &info);
	if (error != B_OK) {
		Unset();
		return error;
	}

	// delegate the actual work
	return SetTo(info.name, clobber);
#else	// HAIKU_TARGET_PLATFORM_HAIKU
	return B_NOT_SUPPORTED;
#endif
}


// Initialized the BResources object to represent the resources of
// the file from which the specified pointer has been loaded.
status_t
BResources::SetToImage(const void* codeOrDataPointer, bool clobber)
{
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (!codeOrDataPointer)
		return B_BAD_VALUE;

	// iterate through the images and find the one in question
	addr_t address = (addr_t)codeOrDataPointer;
	image_info info;
	int32 cookie = 0;

	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (((addr_t)info.text <= address
				&& address - (addr_t)info.text < (addr_t)info.text_size)
			|| ((addr_t)info.data <= address
				&& address - (addr_t)info.data < (addr_t)info.data_size)) {
			return SetTo(info.name, clobber);
		}
	}

	return B_ENTRY_NOT_FOUND;
#else	// HAIKU_TARGET_PLATFORM_HAIKU
	return B_NOT_SUPPORTED;
#endif
}


// Returns the BResources object to an uninitialized state.
void
BResources::Unset()
{
	if (fContainer && fContainer->IsModified())
		Sync();
	delete fResourceFile;
	fResourceFile = NULL;
	fFile.Unset();
	if (fContainer)
		fContainer->MakeEmpty();
	else
		fContainer = new(nothrow) ResourcesContainer;
	fReadOnly = false;
}


// Gets the initialization status of the object.
status_t
BResources::InitCheck() const
{
	return (fContainer ? B_OK : B_NO_MEMORY);
}


// Gets a reference to the internal BFile object.
const BFile&
BResources::File() const
{
	return fFile;
}


// Loads a resource identified by type and id into memory.
const void*
BResources::LoadResource(type_code type, int32 id, size_t* _size)
{
	// find the resource
	status_t error = InitCheck();
	ResourceItem* resource = NULL;
	if (error == B_OK) {
		resource = fContainer->ResourceAt(fContainer->IndexOf(type, id));
		if (!resource)
			error = B_ENTRY_NOT_FOUND;
	}
	// load it, if necessary
	if (error == B_OK && !resource->IsLoaded() && fResourceFile)
		error = fResourceFile->ReadResource(*resource);
	// return the result
	const void *result = NULL;
	if (error == B_OK) {
		result = resource->Data();
		if (_size)
			*_size = resource->DataSize();
	}
	return result;
}


// Loads a resource identified by type and name into memory.
const void*
BResources::LoadResource(type_code type, const char* name, size_t* _size)
{
	// find the resource
	status_t error = InitCheck();
	ResourceItem* resource = NULL;
	if (error == B_OK) {
		resource = fContainer->ResourceAt(fContainer->IndexOf(type, name));
		if (!resource)
			error = B_ENTRY_NOT_FOUND;
	}
	// load it, if necessary
	if (error == B_OK && !resource->IsLoaded() && fResourceFile)
		error = fResourceFile->ReadResource(*resource);
	// return the result
	const void* result = NULL;
	if (error == B_OK) {
		result = resource->Data();
		if (_size)
			*_size = resource->DataSize();
	}
	return result;
}


// Loads all resources of the specified type into memory.
status_t
BResources::PreloadResourceType(type_code type)
{
	status_t error = InitCheck();
	if (error == B_OK && fResourceFile) {
		if (type == 0)
			error = fResourceFile->ReadResources(*fContainer);
		else {
			int32 count = fContainer->CountResources();
			int32 errorCount = 0;
			for (int32 i = 0; i < count; i++) {
				ResourceItem *resource = fContainer->ResourceAt(i);
				if (resource->Type() == type) {
					if (fResourceFile->ReadResource(*resource) != B_OK)
						errorCount++;
				}
			}
			error = -errorCount;
		}
	}
	return error;
}


// Writes all changes to the resources to the file.
status_t
BResources::Sync()
{
	status_t error = InitCheck();
	if (error == B_OK)
		error = fFile.InitCheck();
	if (error == B_OK) {
		if (fReadOnly)
			error = B_NOT_ALLOWED;
		else if (!fResourceFile)
			error = B_FILE_ERROR;
	}
	if (error == B_OK)
		error = fResourceFile->ReadResources(*fContainer);
	if (error == B_OK)
		error = fResourceFile->WriteResources(*fContainer);
	return error;
}


// Adds the resources of fromFile to the internal file of the
// BResources object.
status_t
BResources::MergeFrom(BFile* fromFile)
{
	status_t error = (fromFile ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = InitCheck();
	if (error == B_OK) {
		ResourceFile resourceFile;
		error = resourceFile.SetTo(fromFile);
		ResourcesContainer container;
		if (error == B_OK)
			error = resourceFile.InitContainer(container);
		if (error == B_OK)
			error = resourceFile.ReadResources(container);
		if (error == B_OK)
			fContainer->AssimilateResources(container);
	}
	return error;
}


// Writes the resources to a new file.
status_t
BResources::WriteTo(BFile* file)
{
	status_t error = (file ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = InitCheck();
	// make sure, that all resources are loaded
	if (error == B_OK && fResourceFile) {
		error = fResourceFile->ReadResources(*fContainer);
		fResourceFile->Unset();
	}
	// set the new file, but keep the old container
	if (error == B_OK) {
		ResourcesContainer *container = fContainer;
		fContainer = new(nothrow) ResourcesContainer;
		if (fContainer) {
			error = SetTo(file, false);
			delete fContainer;
		} else
			error = B_NO_MEMORY;
		fContainer = container;
	}
	// write the resources
	if (error == B_OK && fResourceFile)
		error = fResourceFile->WriteResources(*fContainer);
	return error;
}


// Adds a new resource to the file.
status_t
BResources::AddResource(type_code type, int32 id, const void* data,
						size_t length, const char* name)
{
	status_t error = (data ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = InitCheck();
	if (error == B_OK)
		error = (fReadOnly ? B_NOT_ALLOWED : B_OK);
	if (error == B_OK) {
		ResourceItem* item = new(nothrow) ResourceItem;
		if (!item)
			error = B_NO_MEMORY;
		if (error == B_OK) {
			item->SetIdentity(type, id, name);
			ssize_t written = item->WriteAt(0, data, length);
			if (written < 0)
				error = written;
			else if (written != (ssize_t)length)
				error = B_ERROR;
		}
		if (error == B_OK) {
			if (!fContainer->AddResource(item))
				error = B_NO_MEMORY;
		}
		if (error != B_OK)
			delete item;
	}
	return error;
}


// Returns whether the file contains a resource with the specified
// type and id.
bool
BResources::HasResource(type_code type, int32 id)
{
	return (InitCheck() == B_OK && fContainer->IndexOf(type, id) >= 0);
}


// Returns whether the file contains a resource with the specified
// type and name.
bool
BResources::HasResource(type_code type, const char* name)
{
	return (InitCheck() == B_OK && fContainer->IndexOf(type, name) >= 0);
}


// Gets information about a resource identified by byindex.
bool
BResources::GetResourceInfo(int32 byIndex, type_code* typeFound,
	int32* idFound, const char** nameFound, size_t* lengthFound)
{
	ResourceItem* item = NULL;
	if (InitCheck() == B_OK)
		item = fContainer->ResourceAt(byIndex);
	if (item) {
		if (typeFound)
			*typeFound = item->Type();
		if (idFound)
			*idFound = item->ID();
		if (nameFound)
			*nameFound = item->Name();
		if (lengthFound)
			*lengthFound = item->DataSize();
	}
	return item;
}


// Gets information about a resource identified by byType and andIndex.
bool
BResources::GetResourceInfo(type_code byType, int32 andIndex, int32* idFound,
	const char** nameFound, size_t* lengthFound)
{
	ResourceItem* item = NULL;
	if (InitCheck() == B_OK) {
		item = fContainer->ResourceAt(fContainer->IndexOfType(byType,
															  andIndex));
	}
	if (item) {
		if (idFound)
			*idFound = item->ID();
		if (nameFound)
			*nameFound = item->Name();
		if (lengthFound)
			*lengthFound = item->DataSize();
	}
	return item;
}


// Gets information about a resource identified by byType and andID.
bool
BResources::GetResourceInfo(type_code byType, int32 andID,
	const char** nameFound, size_t* lengthFound)
{
	ResourceItem* item = NULL;
	if (InitCheck() == B_OK)
		item = fContainer->ResourceAt(fContainer->IndexOf(byType, andID));
	if (item) {
		if (nameFound)
			*nameFound = item->Name();
		if (lengthFound)
			*lengthFound = item->DataSize();
	}
	return item;
}


// Gets information about a resource identified by byType and andName.
bool
BResources::GetResourceInfo(type_code byType, const char* andName,
	int32* idFound, size_t* lengthFound)
{
	ResourceItem* item = NULL;
	if (InitCheck() == B_OK)
		item = fContainer->ResourceAt(fContainer->IndexOf(byType, andName));
	if (item) {
		if (idFound)
			*idFound = item->ID();
		if (lengthFound)
			*lengthFound = item->DataSize();
	}
	return item;
}


// Gets information about a resource identified by byPointer.
bool
BResources::GetResourceInfo(const void* byPointer, type_code* typeFound,
	int32* idFound, size_t* lengthFound, const char** nameFound)
{
	ResourceItem* item = NULL;
	if (InitCheck() == B_OK)
		item = fContainer->ResourceAt(fContainer->IndexOf(byPointer));
	if (item) {
		if (typeFound)
			*typeFound = item->Type();
		if (idFound)
			*idFound = item->ID();
		if (nameFound)
			*nameFound = item->Name();
		if (lengthFound)
			*lengthFound = item->DataSize();
	}
	return item;
}


// Removes a resource identified by its data pointer.
status_t
BResources::RemoveResource(const void* resource)
{
	status_t error = (resource ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = InitCheck();
	if (error == B_OK)
		error = (fReadOnly ? B_NOT_ALLOWED : B_OK);
	if (error == B_OK) {
		ResourceItem* item
			= fContainer->RemoveResource(fContainer->IndexOf(resource));
		if (item)
			delete item;
		else
			error = B_BAD_VALUE;
	}
	return error;
}


// Removes a resource identified by type and id.
status_t
BResources::RemoveResource(type_code type, int32 id)
{
	status_t error = InitCheck();
	if (error == B_OK)
		error = (fReadOnly ? B_NOT_ALLOWED : B_OK);
	if (error == B_OK) {
		ResourceItem* item
			= fContainer->RemoveResource(fContainer->IndexOf(type, id));
		if (item)
			delete item;
		else
			error = B_BAD_VALUE;
	}
	return error;
}


// #pragma mark - deprecated methods


// Writes data into an existing resource
// (deprecated, use AddResource() instead).
status_t
BResources::WriteResource(type_code type, int32 id, const void* data,
	off_t offset, size_t length)
{
	status_t error = (data && offset >= 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = InitCheck();
	if (error == B_OK)
		error = (fReadOnly ? B_NOT_ALLOWED : B_OK);
	ResourceItem *item = NULL;
	if (error == B_OK) {
		item = fContainer->ResourceAt(fContainer->IndexOf(type, id));
		if (!item)
			error = B_BAD_VALUE;
	}
	if (error == B_OK && fResourceFile)
		error = fResourceFile->ReadResource(*item);
	if (error == B_OK) {
		if (item) {
			ssize_t written = item->WriteAt(offset, data, length);
			if (written < 0)
				error = written;
			else if (written != (ssize_t)length)
				error = B_ERROR;
		}
	}
	return error;
}


// Reads data from an existing resource
// (deprecated, use LoadResource() instead).
status_t
BResources::ReadResource(type_code type, int32 id, void* data, off_t offset,
	size_t length)
{
	status_t error = (data && offset >= 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = InitCheck();
	ResourceItem* item = NULL;
	if (error == B_OK) {
		item = fContainer->ResourceAt(fContainer->IndexOf(type, id));
		if (!item)
			error = B_BAD_VALUE;
	}
	if (error == B_OK && fResourceFile)
		error = fResourceFile->ReadResource(*item);
	if (error == B_OK) {
		if (item) {
			ssize_t read = item->ReadAt(offset, data, length);
			if (read < 0)
				error = read;
		} else
			error = B_BAD_VALUE;
	}
	return error;
}


// Finds a resource by type and id and returns a pointer to a copy of
// its data (deprecated, use LoadResource() instead).
void*
BResources::FindResource(type_code type, int32 id, size_t* lengthFound)
{
	void* result = NULL;
	size_t size = 0;
	const void* data = LoadResource(type, id, &size);
	if (data != NULL) {
		if ((result = malloc(size)))
			memcpy(result, data, size);
	}
	if (lengthFound)
		*lengthFound = size;
	return result;
}


// Finds a resource by type and name and returns a pointer to a copy of
// its data (deprecated, use LoadResource() instead).
void*
BResources::FindResource(type_code type, const char* name, size_t* lengthFound)
{
	void* result = NULL;
	size_t size = 0;
	const void *data = LoadResource(type, name, &size);
	if (data != NULL) {
		if ((result = malloc(size)))
			memcpy(result, data, size);
	}
	if (lengthFound)
		*lengthFound = size;
	return result;
}


// FBC
void BResources::_ReservedResources1() {}
void BResources::_ReservedResources2() {}
void BResources::_ReservedResources3() {}
void BResources::_ReservedResources4() {}
void BResources::_ReservedResources5() {}
void BResources::_ReservedResources6() {}
void BResources::_ReservedResources7() {}
void BResources::_ReservedResources8() {}
