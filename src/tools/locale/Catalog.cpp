/*
** Distributed under the terms of the OpenBeOS License.
** Copyright 2003-2004. All rights reserved.
**
** Authors:	Axel Dörfler, axeld@pinc-software.de
**			Oliver Tappe, zooey@hirschkaefer.de
*/

#include <syslog.h>

#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Node.h>
#include <Roster.h>


BCatalog* be_catalog = NULL;
	// catalog used by translation macros
BCatalog* be_app_catalog = NULL;
	// app-catalog (useful for accessing app's catalog from inside an add-on,
	// since in an add-on, be_catalog will hold the add-on's catalog.


//#pragma mark - BCatalog
BCatalog::BCatalog()
	:
	fCatalog(NULL)
{
}


BCatalog::BCatalog(const entry_ref& catalogOwner, const char *language,
	uint32 fingerprint)
{
	// Unsupported - the build tools can't (and don't need to) load anything
	// this way.
}


BCatalog::~BCatalog()
{
	if (be_catalog == this)
		be_app_catalog = be_catalog = NULL;
	//be_locale_roster->UnloadCatalog(fCatalog);
}


const char *
BCatalog::GetString(const char *string, const char *context, const char *comment)
{
	const char *translated;
	for (BCatalogAddOn* cat = fCatalog; cat != NULL; cat = cat->fNext) {
		translated = cat->GetString(string, context, comment);
		if (translated)
			return translated;
	}
	return string;
}


const char *
BCatalog::GetString(uint32 id)
{
	const char *translated;
	for (BCatalogAddOn* cat = fCatalog; cat != NULL; cat = cat->fNext) {
		translated = cat->GetString(id);
		if (translated)
			return translated;
	}
	return "";
}


status_t
BCatalog::GetData(const char *name, BMessage *msg)
{
	if (!fCatalog)
		return B_NO_INIT;
	status_t res;
	for (BCatalogAddOn* cat = fCatalog; cat != NULL; cat = cat->fNext) {
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
	if (!fCatalog)
		return B_NO_INIT;
	status_t res;
	for (BCatalogAddOn* cat = fCatalog; cat != NULL; cat = cat->fNext) {
		res = cat->GetData(id, msg);
		if (res != B_NAME_NOT_FOUND && res != EOPNOTSUPP)
			return res;
				// return B_OK if found, or specific error-code
	}
	return B_NAME_NOT_FOUND;
}


//#pragma mark - BCatalogAddOn
BCatalogAddOn::BCatalogAddOn(const char *signature, const char *language,
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
BCatalogAddOn::GetData(const char *name, BMessage *msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::GetData(uint32 id, BMessage *msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::SetString(const char *string, const char *translated,
	const char *context, const char *comment)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::SetString(int32 id, const char *translated)
{
	return EOPNOTSUPP;
}


bool
BCatalogAddOn::CanWriteData() const
{
	return false;
}


status_t
BCatalogAddOn::SetData(const char *name, BMessage *msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::SetData(uint32 id, BMessage *msg)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::ReadFromFile(const char *path)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::ReadFromAttribute(const entry_ref &appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::ReadFromResource(const entry_ref &appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::WriteToFile(const char *path)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::WriteToAttribute(const entry_ref &appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t
BCatalogAddOn::WriteToResource(const entry_ref &appOrAddOnRef)
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
