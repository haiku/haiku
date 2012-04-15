/*
** Distributed under the terms of the OpenBeOS License.
** Copyright 2003-2004,2012. All rights reserved.
**
** Authors:	Axel Dörfler, axeld@pinc-software.de
**			Oliver Tappe, zooey@hirschkaefer.de
*/

#include <CatalogData.h>


// Provides an empty implementation of BCatalogData for the build host.


BCatalogData::BCatalogData(const char *signature, const char *language,
	uint32 fingerprint)
	:
	fInitCheck(B_NO_INIT),
	fSignature(signature),
	fLanguageName(language),
	fFingerprint(fingerprint),
	fNext(NULL)
{
	fLanguageName.ToLower();
		// canonicalize language-name to lowercase
}


BCatalogData::~BCatalogData()
{
}


void
BCatalogData::UpdateFingerprint()
{
	fFingerprint = 0;
		// base implementation always yields the same fingerprint,
		// which means that no version-mismatch detection is possible.
}


status_t
BCatalogData::InitCheck() const
{
	return fInitCheck;
}


bool
BCatalogData::CanHaveData() const
{
	return false;
}


status_t
BCatalogData::GetData(const char *name, BMessage *msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::GetData(uint32 id, BMessage *msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::SetString(const char *string, const char *translated,
	const char *context, const char *comment)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::SetString(int32 id, const char *translated)
{
	return EOPNOTSUPP;
}


bool
BCatalogData::CanWriteData() const
{
	return false;
}


status_t
BCatalogData::SetData(const char *name, BMessage *msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::SetData(uint32 id, BMessage *msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::ReadFromFile(const char *path)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::ReadFromAttribute(const entry_ref &appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::ReadFromResource(const entry_ref &appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::WriteToFile(const char *path)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::WriteToAttribute(const entry_ref &appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t
BCatalogData::WriteToResource(const entry_ref &appOrAddOnRef)
{
	return EOPNOTSUPP;
}


void BCatalogData::MakeEmpty()
{
}


int32
BCatalogData::CountItems() const
{
	return 0;
}
