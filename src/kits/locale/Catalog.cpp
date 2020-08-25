/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2003-2004,2012, Oliver Tappe, zooey@hirschkaefer.de
 * Distributed under the terms of the MIT License.
 */

#include <Catalog.h>

#include <Application.h>
#include <Autolock.h>
#include <CatalogData.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Node.h>
#include <Roster.h>


using BPrivate::MutableLocaleRoster;


//#pragma mark - BCatalog
BCatalog::BCatalog()
	:
	fCatalogData(NULL),
	fLock("Catalog")
{
}


BCatalog::BCatalog(const entry_ref& catalogOwner, const char* language,
	uint32 fingerprint)
	:
	fCatalogData(NULL),
	fLock("Catalog")
{
	SetTo(catalogOwner, language, fingerprint);
}


BCatalog::BCatalog(const char* signature, const char* language)
	:
	fCatalogData(NULL),
	fLock("Catalog")
{
	SetTo(signature, language);
}


BCatalog::~BCatalog()
{
	MutableLocaleRoster::Default()->UnloadCatalog(fCatalogData);
}


const char*
BCatalog::GetString(const char* string, const char* context,
	const char* comment)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return string;

	const char* translated;
	for (BCatalogData* cat = fCatalogData; cat != NULL; cat = cat->fNext) {
		translated = cat->GetString(string, context, comment);
		if (translated != NULL)
			return translated;
	}

	return string;
}


const char*
BCatalog::GetString(uint32 id)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return "";

	const char* translated;
	for (BCatalogData* cat = fCatalogData; cat != NULL; cat = cat->fNext) {
		translated = cat->GetString(id);
		if (translated != NULL)
			return translated;
	}

	return "";
}


status_t
BCatalog::GetData(const char* name, BMessage* msg)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	if (fCatalogData == NULL)
		return B_NO_INIT;

	status_t res;
	for (BCatalogData* cat = fCatalogData; cat != NULL; cat = cat->fNext) {
		res = cat->GetData(name, msg);
		if (res != B_NAME_NOT_FOUND && res != EOPNOTSUPP)
			return res;	// return B_OK if found, or specific error-code
	}

	return B_NAME_NOT_FOUND;
}


status_t
BCatalog::GetData(uint32 id, BMessage* msg)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	if (fCatalogData == NULL)
		return B_NO_INIT;

	status_t res;
	for (BCatalogData* cat = fCatalogData; cat != NULL; cat = cat->fNext) {
		res = cat->GetData(id, msg);
		if (res != B_NAME_NOT_FOUND && res != EOPNOTSUPP)
			return res;	// return B_OK if found, or specific error-code
	}

	return B_NAME_NOT_FOUND;
}


status_t
BCatalog::GetSignature(BString* sig)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	if (sig == NULL)
		return B_BAD_VALUE;

	if (fCatalogData == NULL)
		return B_NO_INIT;

	*sig = fCatalogData->fSignature;

	return B_OK;
}


status_t
BCatalog::GetLanguage(BString* lang)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	if (lang == NULL)
		return B_BAD_VALUE;

	if (fCatalogData == NULL)
		return B_NO_INIT;

	*lang = fCatalogData->fLanguageName;

	return B_OK;
}


status_t
BCatalog::GetFingerprint(uint32* fp)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	if (fp == NULL)
		return B_BAD_VALUE;

	if (fCatalogData == NULL)
		return B_NO_INIT;

	*fp = fCatalogData->fFingerprint;

	return B_OK;
}


status_t
BCatalog::SetTo(const entry_ref& catalogOwner, const char* language,
	uint32 fingerprint)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	MutableLocaleRoster::Default()->UnloadCatalog(fCatalogData);
	fCatalogData = MutableLocaleRoster::Default()->LoadCatalog(catalogOwner,
		language, fingerprint);

	return B_OK;
}


status_t
BCatalog::SetTo(const char* signature, const char* language)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	MutableLocaleRoster::Default()->UnloadCatalog(fCatalogData);
	fCatalogData = MutableLocaleRoster::Default()->LoadCatalog(signature,
		language);

	return B_OK;
}


status_t
BCatalog::InitCheck() const
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	return fCatalogData != NULL	? fCatalogData->InitCheck() : B_NO_INIT;
}


int32
BCatalog::CountItems() const
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return 0;

	return fCatalogData != NULL ? fCatalogData->CountItems() : 0;
}
