/*
 * Copyright 2013-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <UrlResult.h>


BUrlResult::BUrlResult()
	:
	BArchivable(),
	fContentType(),
	fLength(0)
{
}


BUrlResult::BUrlResult(BMessage* archive)
	:
	BArchivable(archive)
{
	fContentType = archive->FindString("ContentType");
	fLength = archive->FindInt32("Length");
}


BUrlResult::~BUrlResult()
{
}


status_t
BUrlResult::Archive(BMessage* archive, bool deep) const
{
	status_t result = BArchivable::Archive(archive, deep);

	if (result != B_OK)
		return result;

	archive->AddString("ContentType", fContentType);
	archive->AddInt32("Length", fLength);

	return B_OK;
}


void
BUrlResult::SetContentType(BString contentType)
{
	fContentType = contentType;
}


void
BUrlResult::SetLength(size_t length)
{
	fLength = length;
}


BString
BUrlResult::ContentType() const
{
	return fContentType;
}


size_t
BUrlResult::Length() const
{
	return fLength;
}


/*static*/ BArchivable*
BUrlResult::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "BUrlResult"))
		return NULL;
	return new BUrlResult(archive);
}
