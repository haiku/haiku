/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2003-2004,2012, Oliver Tappe, zooey@hirschkaefer.de
 * Distributed under the terms of the MIT License.
 */

#include <Catalog.h>

#include <syslog.h>

#include <Application.h>
#include <Autolock.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Node.h>
#include <Roster.h>


using BPrivate::MutableLocaleRoster;


//#pragma mark - BCatalog
BCatalog::BCatalog()
	:
	fCatalog(NULL),
	fLock("Catalog")
{
}


BCatalog::BCatalog(const entry_ref& catalogOwner, const char* language,
	uint32 fingerprint)
	:
	fCatalog(NULL),
	fLock("Catalog")
{
	SetTo(catalogOwner, language, fingerprint);
}


BCatalog::~BCatalog()
{
	MutableLocaleRoster::Default()->UnloadCatalog(fCatalog);
}


const char*
BCatalog::GetString(const char* string, const char* context,
	const char* comment)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return string;

	const char* translated;
	for (BCatalogAddOn* cat = fCatalog; cat != NULL; cat = cat->fNext) {
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
	for (BCatalogAddOn* cat = fCatalog; cat != NULL; cat = cat->fNext) {
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

	if (fCatalog == NULL)
		return B_NO_INIT;

	status_t res;
	for (BCatalogAddOn* cat = fCatalog; cat != NULL; cat = cat->fNext) {
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

	if (fCatalog == NULL)
		return B_NO_INIT;

	status_t res;
	for (BCatalogAddOn* cat = fCatalog; cat != NULL; cat = cat->fNext) {
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

	if (fCatalog == NULL)
		return B_NO_INIT;

	*sig = fCatalog->fSignature;

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

	if (fCatalog == NULL)
		return B_NO_INIT;

	*lang = fCatalog->fLanguageName;

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

	if (fCatalog == NULL)
		return B_NO_INIT;

	*fp = fCatalog->fFingerprint;

	return B_OK;
}


status_t
BCatalog::SetTo(const entry_ref& catalogOwner, const char* language,
	uint32 fingerprint)
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	MutableLocaleRoster::Default()->UnloadCatalog(fCatalog);
	fCatalog = MutableLocaleRoster::Default()->LoadCatalog(catalogOwner,
		language, fingerprint);

	return B_OK;
}


status_t
BCatalog::InitCheck() const
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	return fCatalog != NULL	? fCatalog->InitCheck() : B_NO_INIT;
}


int32
BCatalog::CountItems() const
{
	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return 0;

	return fCatalog != NULL ? fCatalog->CountItems() : 0;
}


//#pragma mark - BCatalogAddOn
BCatalogAddOn::BCatalogAddOn(const char* signature, const char* language,
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


BCatalogAddOn::~BCatalogAddOn()
{
}


void
BCatalogAddOn::UpdateFingerprint()
{
	fFingerprint = 0;
		// base implementation always yields the same fingerprint,
		// which means that no version-mismatch detection is possible.
}


status_t
BCatalogAddOn::InitCheck() const
{
	return fInitCheck;
}


bool
BCatalogAddOn::CanHaveData() const
{
	return false;
}


status_t
BCatalogAddOn::GetData(const char* name, BMessage* msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::GetData(uint32 id, BMessage* msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::SetString(const char* string, const char* translated,
	const char* context, const char* comment)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::SetString(int32 id, const char* translated)
{
	return EOPNOTSUPP;
}


bool
BCatalogAddOn::CanWriteData() const
{
	return false;
}


status_t
BCatalogAddOn::SetData(const char* name, BMessage* msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::SetData(uint32 id, BMessage* msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::ReadFromFile(const char* path)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::ReadFromAttribute(const entry_ref& appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::ReadFromResource(const entry_ref& appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::WriteToFile(const char* path)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::WriteToAttribute(const entry_ref& appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::WriteToResource(const entry_ref& appOrAddOnRef)
{
	return EOPNOTSUPP;
}


void BCatalogAddOn::MakeEmpty()
{
}


int32
BCatalogAddOn::CountItems() const
{
	return 0;
}


void
BCatalogAddOn::SetNext(BCatalogAddOn* next)
{
	fNext = next;
}
