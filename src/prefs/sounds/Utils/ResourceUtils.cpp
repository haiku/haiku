#include "ResourceUtils.h"
#include <Application.h>
#include <stdio.h>
#include <String.h>

ResourceUtils::ResourceUtils(BResources *rsrc)
	:BLocker()
	,fResource(NULL)
{
	if(rsrc == NULL)
		fResource = BApplication::AppResources();
	else
		fResource = rsrc;
}

ResourceUtils::ResourceUtils(const char *path)
	:BLocker()
	,fResource(NULL)
{
	if(path == NULL)
		fResource = BApplication::AppResources();
	else{
		BFile file(path, B_READ_WRITE); 
		BResources *res = new BResources();
		status_t err;
		if((err = res->SetTo(&file)) != B_OK)
			fResource = BApplication::AppResources();
		else
			fResource = res;
	}
}

ResourceUtils::~ResourceUtils()
{
	//if(fResource != BApplication::AppResources())
	//	delete fResource;
}

/*
 * Load icon by id.
 */
status_t
ResourceUtils::GetIconResource(int32 id, icon_size size, BBitmap *dest)
{
	if (size != B_LARGE_ICON && size != B_MINI_ICON )
		return B_ERROR;
	
	size_t len = 0;
	
	this->Lock();
	const void *data = fResource->LoadResource(size == B_LARGE_ICON ? 'ICON' : 'MICN',
		id, &len);
	this->Unlock();
	if (data == 0 || len != (size_t)(size == B_LARGE_ICON ? 1024 : 256)) {
		return B_ERROR;
	}

	dest->SetBits(data, (int32)len, 0, B_COLOR_8_BIT);
	return B_OK;
}

/*
 * Load icon by name.
 */
status_t
ResourceUtils::GetIconResource(const char* name, icon_size size, BBitmap *dest)
{
	if (size != B_LARGE_ICON && size != B_MINI_ICON )
		return B_ERROR;
	
	size_t len = 0;
	this->Lock();
	const void *data = fResource->LoadResource(size == B_LARGE_ICON ? 'ICON' : 'MICN',
		name, &len);
	this->Unlock();	
	if (data == 0 || len != (size_t)(size == B_LARGE_ICON ? 1024 : 256)) {
		return B_ERROR;
	}
	
	dest->SetBits(data, (int32)len, 0, B_COLOR_8_BIT);
	return B_OK;
}


/*
 * Load bitmap by id.
 */
status_t
ResourceUtils::GetBitmapResource(type_code type, int32 id, BBitmap **out)
{
	*out = NULL;
	
	size_t len = 0;
	
	this->Lock();
	const void *data = fResource->LoadResource(type, id, &len);
	this->Unlock();
	
	if (data == NULL) {
		return B_ERROR;
	}
	
	BMemoryIO stream(data, len);
	
	// Try to read as an archived bitmap.
	stream.Seek(0, SEEK_SET);
	BMessage archive;
	status_t err = archive.Unflatten(&stream);
	if (err != B_OK)
		return err;

	*out = new BBitmap(&archive);
	if (!*out)
		return B_ERROR;

	err = (*out)->InitCheck();
	if (err != B_OK) {
		delete *out;
		*out = NULL;
	}
	
	return err;
}	

/*
 * Load bitmap by name.
 */
status_t
ResourceUtils::GetBitmapResource(type_code type, const char* name, BBitmap **out)
{
	*out = NULL;	
	size_t len = 0;
	
	this->Lock();
	const void *data = fResource->LoadResource(type, name, &len);
	this->Unlock();
	if (data == NULL) {
		return B_ERROR;
	}
	
	BMemoryIO stream(data, len);
	
	// Try to read as an archived bitmap.
	stream.Seek(0, SEEK_SET);
	BMessage archive;
	status_t err = archive.Unflatten(&stream);
	if (err != B_OK)
		return err;

	*out = new BBitmap(&archive);
	if (!*out)
		return B_ERROR;

	err = (*out)->InitCheck();
	if (err != B_OK) {
		delete *out;
		*out = NULL;
	}
	
	return err;
}	

/*
 * Load bitmap by name.
 */
BBitmap*
ResourceUtils::GetBitmapResource(type_code type, const char* name)
{	
	size_t len = 0;
	this->Lock();
	const void *data = fResource->LoadResource(type, name, &len);
	this->Unlock();
	if (data == NULL) {
		return NULL;
	}
	
	BMemoryIO stream(data, len);
	
	// Try to read as an archived bitmap.
	stream.Seek(0, SEEK_SET);
	BMessage archive;
	status_t err = archive.Unflatten(&stream);
	if (err != B_OK)
		return NULL;

	BBitmap* out = new BBitmap(&archive);
	if (!out)
		return NULL;

	err = (out)->InitCheck();
	if (err != B_OK) {
		delete out;
		out = NULL;
	}
	
	return out;
}	

/*
 * Get string from resources.
 */
const char*
ResourceUtils::GetString(const char* name)
{
	size_t len = 0;
	this->Lock();
	const void* data;
	if(fResource->HasResource('CSTR',name))
	{
		data = fResource->LoadResource('CSTR', name, &len);
	}else
		data = name;
	this->Unlock();
	return (const char*)data;
}

/*
 * Get string from resources.
 */
status_t
ResourceUtils::GetString(const char* name,BString &outStr)
{
	size_t len = 0;
	this->Lock();
	const void* data;
	status_t err = B_OK;
	if(fResource->HasResource('CSTR',name))
	{
		data = fResource->LoadResource('CSTR', name, &len);
		outStr = (const char*)data;
	}else{
		err = B_ERROR;	
		outStr = name;
	}
	this->Unlock();
	return err;
}

/*
 * Set resources.
 */
void
ResourceUtils::SetResource(BResources *rsrc)
{
	fResource = rsrc;
}

/*
 * Set resources.
 */
void
ResourceUtils::SetResource(const char* path)
{
	BFile file(path, B_READ_WRITE); 
	BResources *res = new BResources();
	status_t err;
	if((err = res->SetTo(&file)) != B_OK)
		fResource = BApplication::AppResources();
	else
		fResource = res;
}

/*
 * Free fResource. But You must not free when load app resource.
 */
void
ResourceUtils::FreeResource()
{
	delete fResource;
}

/*
 * Preload resources.
 */
void
ResourceUtils::Preload(type_code type)
{
	fResource->PreloadResourceType(type);
}
