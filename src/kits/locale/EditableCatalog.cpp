/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2003-2004,2012, Oliver Tappe, zooey@hirschkaefer.de
 * Distributed under the terms of the MIT License.
 */

#include <EditableCatalog.h>

#include <MutableLocaleRoster.h>


using BPrivate::MutableLocaleRoster;


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


BCatalogAddOn*
EditableCatalog::CatalogAddOn()
{
	return fCatalog;
}


} // namespace BPrivate
