/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCHIVABLE_UTILS_H
#define ARCHIVABLE_UTILS_H


#include <Archivable.h>


class ArchivingUtils {
public:
	template<typename ObjectType>
	static	ObjectType*			CastOrDelete(BArchivable* archivable);

	template<typename ObjectType>
	static	ObjectType*			Unarchive(const BMessage& archive);

	static	status_t			ArchiveChild(BArchivable* object,
									BMessage& parentArchive,
									const char* fieldName);
	static	BArchivable*		UnarchiveChild(const BMessage& parentArchive,
									const char* fieldName, int32 index = 0);

	template<typename ObjectType>
	static	ObjectType*			UnarchiveChild(const BMessage& archive,
									const char* fieldName, int32 index = 0);
};


template<typename ObjectType>
/*static*/ ObjectType*
ArchivingUtils::CastOrDelete(BArchivable* archivable)
{
	if (archivable == NULL)
		return NULL;

	ObjectType* object = dynamic_cast<ObjectType*>(archivable);
	if (object == NULL)
		delete archivable;

	return object;
}


template<typename ObjectType>
/*static*/ ObjectType*
ArchivingUtils::Unarchive(const BMessage& archive)
{
	return CastOrDelete<ObjectType>(instantiate_object(
		const_cast<BMessage*>(&archive)));
}


template<typename ObjectType>
/*static*/ ObjectType*
ArchivingUtils::UnarchiveChild(const BMessage& archive, const char* fieldName,
	int32 index)
{
	return CastOrDelete<ObjectType>(UnarchiveChild(archive, fieldName, index));
}



#endif	// ARCHIVABLE_UTILS_H
