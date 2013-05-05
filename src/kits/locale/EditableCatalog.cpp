/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2003-2004,2012, Oliver Tappe, zooey@hirschkaefer.de
 * Distributed under the terms of the MIT License.
 */

#include <EditableCatalog.h>

#include <CatalogData.h>
#include <MutableLocaleRoster.h>


using BPrivate::MutableLocaleRoster;


namespace BPrivate {
EditableCatalog::EditableCatalog(const char* type, const char* signature,
	const char* language)
{
	fCatalogData = MutableLocaleRoster::Default()->CreateCatalog(type,
		signature, language);
}


EditableCatalog::~EditableCatalog()
{
}


status_t
EditableCatalog::SetString(const char* string, const char* translated,
	const char* context, const char* comment)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->SetString(string, translated, context, comment);
}


status_t
EditableCatalog::SetString(int32 id, const char* translated)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->SetString(id, translated);
}


bool
EditableCatalog::CanWriteData() const
{
	if (fCatalogData == NULL)
		return false;

	return fCatalogData->CanWriteData();
}


status_t
EditableCatalog::SetData(const char* name, BMessage* msg)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->SetData(name, msg);
}


status_t
EditableCatalog::SetData(uint32 id, BMessage* msg)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->SetData(id, msg);
}


status_t
EditableCatalog::ReadFromFile(const char* path)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->ReadFromFile(path);
}


status_t
EditableCatalog::ReadFromAttribute(const entry_ref& appOrAddOnRef)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->ReadFromAttribute(appOrAddOnRef);
}


status_t
EditableCatalog::ReadFromResource(const entry_ref& appOrAddOnRef)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->ReadFromResource(appOrAddOnRef);
}


status_t
EditableCatalog::WriteToFile(const char* path)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->WriteToFile(path);
}


status_t
EditableCatalog::WriteToAttribute(const entry_ref& appOrAddOnRef)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->WriteToAttribute(appOrAddOnRef);
}


status_t
EditableCatalog::WriteToResource(const entry_ref& appOrAddOnRef)
{
	if (fCatalogData == NULL)
		return B_NO_INIT;

	return fCatalogData->WriteToResource(appOrAddOnRef);
}


void EditableCatalog::MakeEmpty()
{
	if (fCatalogData == NULL)
		fCatalogData->MakeEmpty();
}


BCatalogData*
EditableCatalog::CatalogData()
{
	return fCatalogData;
}


} // namespace BPrivate
