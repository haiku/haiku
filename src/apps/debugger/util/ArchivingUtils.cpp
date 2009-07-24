/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ArchivingUtils.h"

#include <Message.h>


/*static*/ status_t
ArchivingUtils::ArchiveChild(BArchivable* object, BMessage& parentArchive,
	const char* fieldName)
{
	if (object == NULL)
		return B_BAD_VALUE;

	BMessage archive;
	status_t error = object->Archive(&archive, true);
	if (error != B_OK)
		return error;

	return parentArchive.AddMessage(fieldName, &archive);
}


/*static*/ BArchivable*
ArchivingUtils::UnarchiveChild(const BMessage& parentArchive,
	const char* fieldName, int32 index)
{
	BMessage archive;
	if (parentArchive.FindMessage(fieldName, index, &archive) != B_OK)
		return NULL;

	return instantiate_object(&archive);
}
