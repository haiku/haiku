/*
 * Copyright 2001-2013, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _RESOURCES_H
#define _RESOURCES_H


#include <Entry.h>
#include <File.h>
#include <image.h>


namespace BPrivate {
	namespace Storage {
		class ResourcesContainer;
		class ResourceFile;
	};
};


class BResources {
public:
								BResources();
								BResources(const BFile* file,
									bool clobber = false);
								// Haiku only
								BResources(const char* path,
									bool clobber = false);
								// Haiku only
								BResources(const entry_ref* ref,
									bool clobber = false);

	virtual						~BResources();

			status_t			SetTo(const BFile* file,
									bool clobber = false);
								// Haiku only
			status_t			SetTo(const char* path,
									bool clobber = false);
								// Haiku only
			status_t			SetTo(const entry_ref* ref,
									bool clobber = false);

								// Haiku only
			status_t			SetToImage(image_id image,
									bool clobber = false);
			status_t			SetToImage(const void* codeOrDataPointer,
									bool clobber = false);

			void				Unset();
			status_t			InitCheck() const;

	const	BFile				&File() const;

	const	void*				LoadResource(type_code type, int32 id,
									size_t* _size);
	const	void*				LoadResource(type_code type, const char* name,
									size_t* _size);

			status_t			PreloadResourceType(type_code type = 0);

			status_t			Sync();
			status_t			MergeFrom(BFile* fromFile);
			status_t			WriteTo(BFile* file);

			status_t			AddResource(type_code type, int32 id,
									const void* data, size_t length,
									const char* name = NULL);

			bool				HasResource(type_code type, int32 id);
			bool				HasResource(type_code type, const char* name);

			bool				GetResourceInfo(int32 byIndex,
									type_code* typeFound, int32* idFound,
									const char** nameFound,
									size_t* lengthFound);
			bool				GetResourceInfo(type_code byType,
									int32 andIndex, int32* idFound,
									const char** nameFound,
									size_t* lengthFound);
			bool				GetResourceInfo(type_code byType, int32 andID,
									const char **nameFound,
									size_t* lengthFound);
			bool				GetResourceInfo(type_code byType,
									const char* andName, int32* idFound,
									size_t* lengthFound);
			bool				GetResourceInfo(const void* byPointer,
									type_code* typeFound,
									int32* idFound, size_t* lengthFound,
									const char** nameFound);

			status_t			RemoveResource(const void *resource);
			status_t			RemoveResource(type_code type, int32 id);

								// deprecated
			status_t			WriteResource(type_code type, int32 id,
									const void* data, off_t offset,
									size_t length);

			status_t			ReadResource(type_code type, int32 id,
									void* data, off_t offset,
									size_t length);

			void*				FindResource(type_code type, int32 id,
									size_t* lengthFound);
			void*				FindResource(type_code type, const char* name,
									size_t *lengthFound);

private:
								// FBC
	virtual	void				_ReservedResources1();
	virtual	void				_ReservedResources2();
	virtual	void				_ReservedResources3();
	virtual	void				_ReservedResources4();
	virtual	void				_ReservedResources5();
	virtual	void				_ReservedResources6();
	virtual	void				_ReservedResources7();
	virtual	void				_ReservedResources8();

private:
			BFile				fFile;
			BPrivate::Storage::ResourcesContainer*	fContainer;
			BPrivate::Storage::ResourceFile*		fResourceFile;
			bool				fReadOnly;
			bool				_pad[3];
								// FBC
			uint32				_reserved[3];
};


#endif	// _RESOURCES_H
