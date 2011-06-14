/*
 * Copyright 2001-2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
/*!
	\file Resources.cpp
	BResources implementation.

	BResources delegates most of the work to ResourcesContainer and
	ResourceFile. The first one manages a collections of ResourceItem's,
	the actual resources, whereas the latter provides the file I/O
	functionality.
	An InitCheck() method is not needed, since a BResources object will
	never be invalid. It always serves as a resources container, even if
	it is not associated with a file. It is always possible to WriteTo()
	the resources BResources contains to a file (a valid one of course).
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

// constructor
/*!	\brief Creates an unitialized BResources object.
*/
BResources::BResources()
	:
	fFile(),
	fContainer(NULL),
	fResourceFile(NULL),
	fReadOnly(false)
{
	fContainer = new(nothrow) ResourcesContainer;
}

// constructor
/*!	\brief Creates a BResources object that represents the resources of the
	supplied file.
	If the \a clobber argument is \c true, the data of the file are erased
	and it is turned into an empty resource file. Otherwise \a file
	must refer either to a resource file or to an executable (ELF or PEF
	binary). If the file has been opened \c B_READ_ONLY, only read access
	to its resources is possible.
	The BResources object makes a copy of \a file, that is the caller remains
	owner of the BFile object.
	\param file the file
	\param clobber if \c true, the file's resources are truncated to size 0
*/
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

// constructor
/*!	\brief Creates a BResources object that represents the resources of the
	supplied file.
	If the \a clobber argument is \c true, the data of the file are erased
	and it is turned into an empty resource file. Otherwise \a path
	must refer either to a resource file or to an executable (ELF or PEF
	binary).
	\param path a path referring to the file
	\param clobber if \c true, the file's resources are truncated to size 0
*/
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

// constructor
/*!	\brief Creates a BResources object that represents the resources of the
	supplied file.
	If the \a clobber argument is \c true, the data of the file are erased
	and it is turned into an empty resource file. Otherwise \a ref
	must refer either to a resource file or to an executable (ELF or PEF
	binary).
	\param ref an entry_ref referring to the file
	\param clobber if \c true, the file's resources are truncated to size 0
*/
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

// destructor
/*!	\brief Frees all resources associated with this object
	Calls Sync() before doing so to make sure that the changes are written
	back to the file.
*/
BResources::~BResources()
{
	Unset();
	delete fContainer;
}

// SetTo
/*!	\brief Re-initialized the BResources object to represent the resources of
	the supplied file.
	What happens, if \a clobber is \c true, depends on the type of the file.
	If the file is capable of containing resources, that is, is a resource
	file or an executable (ELF or PEF), its resources are removed. Otherwise
	the file's data are erased and it is turned into an empty resource file.
	If \a clobber is \c false, \a file must refer to a file that is capable
	of containing resources.
	If the file has been opened \c B_READ_ONLY, only read access
	to its resources is possible.
	The BResources object makes a copy of \a file, that is the caller remains
	owner of the BFile object.
	\param file the file
	\param clobber if \c true, the file's resources are truncated to size 0
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL or uninitialized \a file.
	- \c B_ERROR: Failed to initialize the object (for whatever reason).
*/
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

// SetTo
/*!	\brief Re-initialized the BResources object to represent the resources of
	the supplied file.
	What happens, if \a clobber is \c true, depends on the type of the file.
	If the file is capable of containing resources, that is, is a resource
	file or an executable (ELF or PEF), its resources are removed. Otherwise
	the file's data are erased and it is turned into an empty resource file.
	If \a clobber is \c false, \a path must refer to a file that is capable
	of containing resources.
	\param path a path referring to the file
	\param clobber if \c true, the file's resources are truncated to size 0
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path.
	- \c B_ENTRY_NOT_FOUND: The file couldn't be found.
	- \c B_ERROR: Failed to initialize the object (for whatever reason).
*/
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

// SetTo
/*!	\brief Re-initialized the BResources object to represent the resources of
	the supplied file.
	What happens, if \a clobber is \c true, depends on the type of the file.
	If the file is capable of containing resources, that is, is a resource
	file or an executable (ELF or PEF), its resources are removed. Otherwise
	the file's data are erased and it is turned into an empty resource file.
	If \a clobber is \c false, \a ref must refer to a file that is capable
	of containing resources.
	\param ref an entry_ref referring to the file
	\param clobber if \c true, the file's resources are truncated to size 0
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- \c B_ENTRY_NOT_FOUND: The file couldn't be found.
	- \c B_ERROR: Failed to initialize the object (for whatever reason).
*/
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

// SetToImage
/*!	\brief Re-initialized the BResources object to represent the resources of
	the file from which the specified image has been loaded.
	If \a clobber is \c true, the file's resources are removed.
	\param image ID of a loaded image
	\param clobber if \c true, the file's resources are truncated to size 0
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: The file couldn't be found.
	- \c B_ERROR: Failed to initialize the object (for whatever reason).
*/
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

/*!	\brief Re-initialized the BResources object to represent the resources of
	the file from which the specified image has been loaded.
	The image belongs to the current team and is identified by a pointer into
	it's code (aka text) or data segment, i.e. any pointer to a function or a
	static (or global) variable will do.
	If \a clobber is \c true, the file's resources are removed.
	\param codeOrDataPointer pointer into the text or data segment of the image
	\param clobber if \c true, the file's resources are truncated to size 0
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- \c B_ENTRY_NOT_FOUND: The image or the file couldn't be found.
	- \c B_ERROR: Failed to initialize the object (for whatever reason).
*/
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

// Unset
/*!	\brief Returns the BResources object to an uninitialized state.
	If the object represented resources that had been modified, the data are
	written back to the file.
	\note This method extends the BeOS R5 API.
*/
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

// InitCheck
/*!	Returns the current initialization status.
	Unlike other Storage Kit classes a BResources object is always properly
	initialized, unless it couldn't allocate memory for some important
	internal structures. Thus even after a call to SetTo() that reported an
	error, InitCheck() is likely to return \c B_OK.
	\return
	- \c B_OK, if the objects is properly initialized,
	- \c B_NO_MEMORY otherwise.
	\note This method extends the BeOS R5 API.
*/
status_t
BResources::InitCheck() const
{
	return (fContainer ? B_OK : B_NO_MEMORY);
}

// File
/*!	\brief Returns a reference to the BResources' BFile object.
	\return a reference to the object's BFile.
*/
const BFile&
BResources::File() const
{
	return fFile;
}

// LoadResource
/*!	\brief Loads a resource identified by type and ID into memory.
	A resource is loaded into memory only once. A second call with the same
	parameters will result in the same pointer. The BResources object is the
	owner of the allocated memory and the pointer to it will be valid until
	the object is destroyed or the resource is removed or modified.
	\param type the type of the resource to be loaded
	\param id the ID of the resource to be loaded
	\param outSize a pointer to a variable into which the size of the resource
		   shall be written
	\return A pointer to the resource data, if everything went fine, or
			\c NULL, if the file does not have a resource that matchs the
			parameters or an error occured.
*/
const void*
BResources::LoadResource(type_code type, int32 id, size_t* outSize)
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
		if (outSize)
			*outSize = resource->DataSize();
	}
	return result;
}

// LoadResource
/*!	\brief Loads a resource identified by type and name into memory.
	A resource is loaded into memory only once. A second call with the same
	parameters will result in the same pointer. The BResources object is the
	owner of the allocated memory and the pointer to it will be valid until
	the object is destroyed or the resource is removed or modified.
	\param type the type of the resource to be loaded
	\param name the name of the resource to be loaded
	\param outSize a pointer to a variable into which the size of the resource
		   shall be written
	\return A pointer to the resource data, if everything went fine, or
			\c NULL, if the file does not have a resource that matches the
			parameters or an error occured.
	\note Since a type and name pair may not identify a resource uniquely,
		  this method always returns the first resource that matches the
		  parameters, that is the one with the least index.
*/
const void *
BResources::LoadResource(type_code type, const char* name, size_t* outSize)
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
		if (outSize)
			*outSize = resource->DataSize();
	}
	return result;
}

// PreloadResourceType
/*!	\brief Loads all resources of a certain type into memory.
	For performance reasons it might be useful to do that. If \a type is
	0, all resources are loaded.
	\param type of the resources to be loaded
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_FILE: The resource map is empty???
	- The negative of the number of errors occured.
*/
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

// Sync
/*!	\brief Writes all changes to the resources to the file.
	Since AddResource() and RemoveResource() may change the resources only in
	memory, this method can be used to make sure, that all changes are
	actually written to the file.
	The BResources object's destructor calls Sync() before cleaning up.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_FILE: The resource map is empty???
	- \c B_NOT_ALLOWED: The file is opened read only.
	- \c B_FILE_ERROR: A file error occured.
	- \c B_IO_ERROR: An error occured while writing the resources.
	\note When a resource is written to the file, its data are converted
		  to the endianess of the file, and when reading a resource, the
		  data are converted to the host's endianess. This does of course
		  only work for known types, i.e. those that swap_data() is able to
		  cope with.
*/
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

// MergeFrom
/*!	\brief Adds the resources of the supplied file to this file's resources.
	\param fromFile the file whose resources shall be copied
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a fromFile.
	- \c B_BAD_FILE: The resource map is empty???
	- \c B_FILE_ERROR: A file error occured.
	- \c B_IO_ERROR: An error occured while writing the resources.
*/
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

// WriteTo
/*!	\brief Writes the resources to a new file.
	The resources formerly contained in the target file (if any) are erased.
	When the method returns, the BResources object refers to the new file.
	\param file the file the resources shall be written to.
	\return
	- \c B_OK: Everything went fine.
	- a specific error code.
	\note If the resources have been modified, but not Sync()ed, the old file
		  remains unmodified.
*/
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

// AddResource
/*!	\brief Adds a new resource to the file.
	If a resource with the same type and ID does already exist, it is
	replaced. The caller keeps the ownership of the supplied chunk of memory
	containing the resource data.
	Supplying an empty name (\c "") is equivalent to supplying a \c NULL name.
	\param type the type of the resource
	\param id the ID of the resource
	\param data the resource data
	\param length the size of the data in bytes
	\param name the name of the resource (may be \c NULL)
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a data
	- \c B_NOT_ALLOWED: The file is opened read only.
	- \c B_FILE_ERROR: A file error occured.
	- \c B_NO_MEMORY: Not enough memory for that operation.
*/
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

// HasResource
/*!	\brief Returns whether the file contains a resource with a certain
	type and ID.
	\param type the resource type
	\param id the ID of the resource
	\return \c true, if the file contains a matching resource, \false otherwise
*/
bool
BResources::HasResource(type_code type, int32 id)
{
	return (InitCheck() == B_OK && fContainer->IndexOf(type, id) >= 0);
}

// HasResource
/*!	\brief Returns whether the file contains a resource with a certain
	type and name.
	\param type the resource type
	\param name the name of the resource
	\return \c true, if the file contains a matching resource, \false otherwise
*/
bool
BResources::HasResource(type_code type, const char* name)
{
	return (InitCheck() == B_OK && fContainer->IndexOf(type, name) >= 0);
}

// GetResourceInfo
/*!	\brief Returns information about a resource identified by an index.
	\param byIndex the index of the resource in the file
	\param typeFound a pointer to a variable the type of the found resource
		   shall be written into
	\param idFound a pointer to a variable the ID of the found resource
		   shall be written into
	\param nameFound a pointer to a variable the name pointer of the found
		   resource shall be written into
	\param lengthFound a pointer to a variable the data size of the found
		   resource shall be written into
	\return \c true, if a matching resource could be found, false otherwise
*/
bool
BResources::GetResourceInfo(int32 byIndex, type_code* typeFound,
							int32* idFound, const char** nameFound,
							size_t* lengthFound)
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

// GetResourceInfo
/*!	\brief Returns information about a resource identified by a type and an
	index.
	\param byType the resource type
	\param andIndex the index into a array of resources of type \a byType
	\param idFound a pointer to a variable the ID of the found resource
		   shall be written into
	\param nameFound a pointer to a variable the name pointer of the found
		   resource shall be written into
	\param lengthFound a pointer to a variable the data size of the found
		   resource shall be written into
	\return \c true, if a matching resource could be found, false otherwise
*/
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

// GetResourceInfo
/*!	\brief Returns information about a resource identified by a type and an ID.
	\param byType the resource type
	\param andID the resource ID
	\param nameFound a pointer to a variable the name pointer of the found
		   resource shall be written into
	\param lengthFound a pointer to a variable the data size of the found
		   resource shall be written into
	\return \c true, if a matching resource could be found, false otherwise
*/
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

// GetResourceInfo
/*!	\brief Returns information about a resource identified by a type and a
	name.
	\param byType the resource type
	\param andName the resource name
	\param idFound a pointer to a variable the ID of the found resource
		   shall be written into
	\param lengthFound a pointer to a variable the data size of the found
		   resource shall be written into
	\return \c true, if a matching resource could be found, false otherwise
*/
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

// GetResourceInfo
/*!	\brief Returns information about a resource identified by a data pointer.
	\param byPointer the pointer to the resource data (formely returned by
		   LoadResource())
	\param typeFound a pointer to a variable the type of the found resource
		   shall be written into
	\param idFound a pointer to a variable the ID of the found resource
		   shall be written into
	\param lengthFound a pointer to a variable the data size of the found
		   resource shall be written into
	\param nameFound a pointer to a variable the name pointer of the found
		   resource shall be written into
	\return \c true, if a matching resource could be found, false otherwise
*/
bool
BResources::GetResourceInfo(const void* byPointer, type_code* typeFound,
							int32* idFound, size_t* lengthFound,
							const char** nameFound)
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

// RemoveResource
/*!	\brief Removes a resource identified by its data pointer.
	\param resource the pointer to the resource data (formely returned by
		   LoadResource())
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL or invalid (not pointing to any resource data of
	  this file) \a resource.
	- \c B_NOT_ALLOWED: The file is opened read only.
	- \c B_FILE_ERROR: A file error occured.
	- \c B_ERROR: An error occured while removing the resource.
*/
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

// RemoveResource
/*!	\brief Removes a resource identified by type and ID.
	\param type the type of the resource
	\param id the ID of the resource
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: No such resource.
	- \c B_NOT_ALLOWED: The file is opened read only.
	- \c B_FILE_ERROR: A file error occured.
	- \c B_ERROR: An error occured while removing the resource.
*/
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


// deprecated

// WriteResource
/*!	\brief Writes data into an existing resource.
	If writing the data would exceed the bounds of the resource, it is
	enlarged respectively. If \a offset is past the end of the resource,
	padding with unspecified data is inserted.
	\param type the type of the resource
	\param id the ID of the resource
	\param data the data to be written
	\param offset the byte offset relative to the beginning of the resource at
		   which the data shall be written
	\param length the size of the data to be written
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \a type and \a id do not identify an existing resource or
	  \c NULL \a data.
	- \c B_NO_MEMORY: Not enough memory for this operation.
	- other error codes.
	\deprecated Always use AddResource().
*/
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

// ReadResource
/*!	\brief Reads data from an existing resource.
	If more data than existing are requested, this method does not fail. It
	will then read only the existing data. As a consequence an offset past
	the end of the resource will not cause the method to fail, but no data
	will be read at all.
	\param type the type of the resource
	\param id the ID of the resource
	\param data a pointer to a buffer into which the data shall be read
	\param offset the byte offset relative to the beginning of the resource
		   from which the data shall be read
	\param length the size of the data to be read
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \a type and \a id do not identify an existing resource or
	  \c NULL \a data.
	- \c B_NO_MEMORY: Not enough memory for this operation.
	- other error codes.
	\deprecated Use LoadResource() only.
*/
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

// FindResource
/*!	\brief Finds a resource by type and ID and returns a copy of its data.
	The caller is responsible for free()ing the returned memory.
	\param type the type of the resource
	\param id the ID of the resource
	\param lengthFound a pointer to a variable into which the size of the
		   resource data shall be written
	\return
	- a pointer to the resource data, if everything went fine,
	- \c NULL, if an error occured.
	\deprecated Use LoadResource().
*/
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

// FindResource
/*!	\brief Finds a resource by type and name and returns a copy of its data.
	The caller is responsible for free()ing the returned memory.
	\param type the type of the resource
	\param name the name of the resource
	\param lengthFound a pointer to a variable into which the size of the
		   resource data shall be written
	\return
	- a pointer to the resource data, if everything went fine,
	- \c NULL, if an error occured.
	\deprecated Use LoadResource().
*/
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




