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


BCatalog::BCatalog(const char *signature, const char *language, 
	int32 fingerprint)
{
	fCatalog = be_locale_roster->LoadCatalog(signature, language, fingerprint);
}


BCatalog::~BCatalog()
{
	if (be_catalog == this)
		be_app_catalog = be_catalog = NULL;
	be_locale_roster->UnloadCatalog(fCatalog);
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


status_t 
BCatalog::GetAppCatalog(BCatalog* catalog) 
{
	app_info appInfo;
	if (!be_app || be_app->GetAppInfo(&appInfo) != B_OK)
		return B_ENTRY_NOT_FOUND;
	BString sig(appInfo.signature);

	// drop supertype from mimetype (should be "application/"):
	int32 pos = sig.FindFirst('/');
	if (pos >= 0)
		sig.Remove(0, pos+1);

	// try to fetch fingerprint from app-file (attribute):
	int32 fingerprint = 0;
	BNode appNode(&appInfo.ref);
	appNode.ReadAttr(BLocaleRoster::kCatFingerprintAttr, B_INT32_TYPE, 0, 
		&fingerprint, sizeof(int32));
	// try to load catalog (with given fingerprint):
	catalog->fCatalog
		= be_locale_roster->LoadCatalog(sig.String(), NULL,	fingerprint);

	// load native embedded id-based catalog. If such a catalog exists, 
	// we can fall back to native strings for id-based access, too.
	BCatalogAddOn *embeddedCatalog 
		= be_locale_roster->LoadEmbeddedCatalog(&appInfo.ref);
	if (embeddedCatalog) {
		if (!catalog->fCatalog)
			// embedded catalog is the only catalog that was found:
			catalog->fCatalog = embeddedCatalog;
		else {
			// append embedded catalog to list of loaded catalogs:
			BCatalogAddOn *currCat = catalog->fCatalog;
			while (currCat->fNext)
				currCat = currCat->fNext;
			currCat->fNext = embeddedCatalog;
		}
	}

	// make app-catalog the current catalog for translation-macros:
	be_app_catalog = be_catalog = catalog;

	return catalog->InitCheck();
}


//#pragma mark - BCatalogAddOn
BCatalogAddOn::BCatalogAddOn(const char *signature, const char *language,
	int32 fingerprint)
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
BCatalogAddOn::ReadFromAttribute(entry_ref *appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t 
BCatalogAddOn::ReadFromResource(entry_ref *appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t 
BCatalogAddOn::WriteToFile(const char *path)
{
	return EOPNOTSUPP;
}


status_t 
BCatalogAddOn::WriteToAttribute(entry_ref *appOrAddOnRef)
{
	return EOPNOTSUPP;
}


status_t 
BCatalogAddOn::WriteToResource(entry_ref *appOrAddOnRef)
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


//#pragma mark - EditableCatalog
namespace BPrivate {
EditableCatalog::EditableCatalog(const char *type, const char *signature, 
	const char *language)
{
	fCatalog = be_locale_roster->CreateCatalog(type, signature, language);
}


EditableCatalog::~EditableCatalog()
{
}


status_t 
EditableCatalog::SetString(const char *string, const char *translated,
	const char *context, const char *comment)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->SetString(string, translated, context, comment);
}


status_t 
EditableCatalog::SetString(int32 id, const char *translated)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->SetString(id, translated);
}


bool 
EditableCatalog::CanWriteData() const
{
	if (!fCatalog)
		return false;
	return fCatalog->CanWriteData();
}


status_t 
EditableCatalog::SetData(const char *name, BMessage *msg)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->SetData(name, msg);
}


status_t 
EditableCatalog::SetData(uint32 id, BMessage *msg)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->SetData(id, msg);
}


status_t 
EditableCatalog::ReadFromFile(const char *path)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->ReadFromFile(path);
}


status_t 
EditableCatalog::ReadFromAttribute(entry_ref *appOrAddOnRef)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->ReadFromAttribute(appOrAddOnRef);
}


status_t 
EditableCatalog::ReadFromResource(entry_ref *appOrAddOnRef)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->ReadFromResource(appOrAddOnRef);
}


status_t 
EditableCatalog::WriteToFile(const char *path)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->WriteToFile(path);
}


status_t 
EditableCatalog::WriteToAttribute(entry_ref *appOrAddOnRef)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->WriteToAttribute(appOrAddOnRef);
}


status_t 
EditableCatalog::WriteToResource(entry_ref *appOrAddOnRef)
{
	if (!fCatalog)
		return B_NO_INIT;
	return fCatalog->WriteToResource(appOrAddOnRef);
}


void EditableCatalog::MakeEmpty()
{
	if (fCatalog)
		fCatalog->MakeEmpty();
}


} // namespace BPrivate
