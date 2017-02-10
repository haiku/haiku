/*
** Distributed under the terms of the MIT License.
** Copyright 2003-2004,2012. All rights reserved.
**
** Authors:	Axel Dörfler, axeld@pinc-software.de
**			Oliver Tappe, zooey@hirschkaefer.de
*/

#include <Catalog.h>
#include <CatalogData.h>


// Provides an implementation of BCatalog for the build host, in effect only
// supporting setting and getting of catalog entries.


BCatalog::BCatalog()
	:
	fCatalogData(NULL)
{
}


BCatalog::BCatalog(const entry_ref& catalogOwner, const char *language,
	uint32 fingerprint)
	:
	fCatalogData(NULL)
{
}


BCatalog::~BCatalog()
{
}


const char *
BCatalog::GetString(const char *string, const char *context, const char *comment)
{
	if (fCatalogData == 0)
		return string;

	const char *translated;
	for (BCatalogData* cat = fCatalogData; cat != NULL; cat = cat->fNext) {
		translated = cat->GetString(string, context, comment);
		if (translated)
			return translated;
	}

	return string;
}


const char *
BCatalog::GetString(uint32 id)
{
	if (fCatalogData == 0)
		return "";

	const char *translated;
	for (BCatalogData* cat = fCatalogData; cat != NULL; cat = cat->fNext) {
		translated = cat->GetString(id);
		if (translated)
			return translated;
	}

	return "";
}


status_t
BCatalog::GetData(const char *name, BMessage *msg)
{
	if (fCatalogData == 0)
		return B_NO_INIT;

	status_t res;
	for (BCatalogData* cat = fCatalogData; cat != NULL; cat = cat->fNext) {
		res = cat->GetData(name, msg);
		if (res != B_NAME_NOT_FOUND && res != EOPNOTSUPP)
			return res;
				// return B_OK if found, or specific error-code
	}

	return B_NAME_NOT_FOUND;
}


status_t
BCatalog::GetData(uint32 id, BMessage *msg)
{
	if (fCatalogData == 0)
		return B_NO_INIT;

	status_t res;
	for (BCatalogData* cat = fCatalogData; cat != NULL; cat = cat->fNext) {
		res = cat->GetData(id, msg);
		if (res != B_NAME_NOT_FOUND && res != EOPNOTSUPP)
			return res;
				// return B_OK if found, or specific error-code
	}

	return B_NAME_NOT_FOUND;
}
