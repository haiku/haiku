/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <mime/DatabaseDirectory.h>

#include <fs_attr.h>
#include <Node.h>
#include <StringList.h>

#include <mime/database_support.h>
#include <mime/DatabaseLocation.h>


namespace BPrivate {
namespace Storage {
namespace Mime {


DatabaseDirectory::DatabaseDirectory()
	:
	BMergedDirectory(B_COMPARE)
{
}


DatabaseDirectory::~DatabaseDirectory()
{
}


status_t
DatabaseDirectory::Init(DatabaseLocation* databaseLocation,
	const char* superType)
{
	status_t error = BMergedDirectory::Init();
	if (error != B_OK)
		return error;

	const BStringList& directories = databaseLocation->Directories();
	int32 count = directories.CountStrings();
	for (int32 i = 0; i < count; i++) {
		BString directory = directories.StringAt(i);
		if (superType != NULL)
			directory << '/' << superType;

		AddDirectory(directory);
	}

	return B_OK;
}


bool
DatabaseDirectory::ShallPreferFirstEntry(const entry_ref& entry1, int32 index1,
	const entry_ref& entry2, int32 index2)
{
	return _IsValidMimeTypeEntry(entry1) || !_IsValidMimeTypeEntry(entry2);
}


bool
DatabaseDirectory::_IsValidMimeTypeEntry(const entry_ref& entry)
{
	// check whether the MIME:TYPE attribute exists
	BNode node;
	attr_info info;
	return node.SetTo(&entry) == B_OK
		&& node.GetAttrInfo(BPrivate::Storage::Mime::kTypeAttr, &info) == B_OK;
}


} // namespace Mime
} // namespace Storage
} // namespace BPrivate
