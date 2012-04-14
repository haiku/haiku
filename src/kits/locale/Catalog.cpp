/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2003-2004, Oliver Tappe, zooey@hirschkaefer.de
 * Distributed under the terms of the MIT License.
 */

#include <Catalog.h>

#include <syslog.h>

#include <Application.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Node.h>
#include <Roster.h>


using BPrivate::MutableLocaleRoster;


//#pragma mark - BCatalog
BCatalog::BCatalog()
	:
	fCatalog(NULL)
{
}


BCatalog::BCatalog(const entry_ref& catalogOwner, const char* language,
	uint32 fingerprint)
{
	fCatalog = MutableLocaleRoster::Default()->LoadCatalog(catalogOwner,
		language, fingerprint);
}


BCatalog::~BCatalog()
{
	MutableLocaleRoster::Default()->UnloadCatalog(fCatalog);
}


const char*
BCatalog::GetString(const char* string, const char* context,
	const char* comment)
{
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
	if (fp == NULL)
		return B_BAD_VALUE;

	if (fCatalog == NULL)
		return B_NO_INIT;

	*fp = fCatalog->fFingerprint;

	return B_OK;
}


status_t
BCatalog::SetCatalog(const entry_ref& catalogOwner, uint32 fingerprint)
{
	// This is not thread safe. It is used only in ReadOnlyBootPrompt and should
	// not do harm there, but not sure what to do about it…
	MutableLocaleRoster::Default()->UnloadCatalog(fCatalog);
	fCatalog = MutableLocaleRoster::Default()->LoadCatalog(catalogOwner, NULL,
		fingerprint);

	return B_OK;
}


status_t
BCatalog::InitCheck() const
{
	return fCatalog != NULL	? fCatalog->InitCheck() : B_NO_INIT;
}


int32
BCatalog::CountItems() const
{
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


//#pragma mark - EditableCatalog
namespace BPrivate {
EditableCatalog::EditableCatalog(const char* type, const char* signature,
	const char* language)
{
	fCatalog = MutableLocaleRoster::Default()->CreateCatalog(type, signature,
		language);
}


EditableCatalog::~EditableCatalog()
{
}


status_t
EditableCatalog::SetString(const char* string, const char* translated,
	const char* context, const char* comment)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->SetString(string, translated, context, comment);
}


status_t
EditableCatalog::SetString(int32 id, const char* translated)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->SetString(id, translated);
}


bool
EditableCatalog::CanWriteData() const
{
	if (fCatalog == NULL)
		return false;

	return fCatalog->CanWriteData();
}


status_t
EditableCatalog::SetData(const char* name, BMessage* msg)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->SetData(name, msg);
}


status_t
EditableCatalog::SetData(uint32 id, BMessage* msg)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->SetData(id, msg);
}


status_t
EditableCatalog::ReadFromFile(const char* path)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->ReadFromFile(path);
}


status_t
EditableCatalog::ReadFromAttribute(const entry_ref& appOrAddOnRef)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->ReadFromAttribute(appOrAddOnRef);
}


status_t
EditableCatalog::ReadFromResource(const entry_ref& appOrAddOnRef)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->ReadFromResource(appOrAddOnRef);
}


status_t
EditableCatalog::WriteToFile(const char* path)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->WriteToFile(path);
}


status_t
EditableCatalog::WriteToAttribute(const entry_ref& appOrAddOnRef)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->WriteToAttribute(appOrAddOnRef);
}


status_t
EditableCatalog::WriteToResource(const entry_ref& appOrAddOnRef)
{
	if (fCatalog == NULL)
		return B_NO_INIT;

	return fCatalog->WriteToResource(appOrAddOnRef);
}


void EditableCatalog::MakeEmpty()
{
	if (fCatalog == NULL)
		fCatalog->MakeEmpty();
}


} // namespace BPrivate
