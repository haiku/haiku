/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "SharedIcons.h"

#include <ControlLook.h>
#include <IconUtils.h>
#include <Resources.h>

#include "BitmapHolder.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "support.h"


BitmapHolderRef SharedIcons::sIconStarBlue12Scaled;
BitmapHolderRef SharedIcons::sIconStarBlue16Scaled;
BitmapHolderRef SharedIcons::sIconStarGrey16Scaled;
BitmapHolderRef SharedIcons::sIconInstalled16Scaled;
BitmapHolderRef SharedIcons::sIconArrowLeft22Scaled;
BitmapHolderRef SharedIcons::sIconArrowRight22Scaled;
BitmapHolderRef SharedIcons::sIconHTMLPackage16Scaled;
BitmapHolderRef SharedIcons::sNative16Scaled;


/*static*/ BitmapHolderRef
SharedIcons::IconStarBlue12Scaled()
{
	if (!SharedIcons::sIconStarBlue12Scaled.IsSet()) {
		SharedIcons::sIconStarBlue12Scaled
			= SharedIcons::_CreateIconForResource(RSRC_STAR_BLUE, 12);
	}
	return SharedIcons::sIconStarBlue12Scaled;
}


/*static*/ BitmapHolderRef
SharedIcons::IconStarBlue16Scaled()
{
	if (!SharedIcons::sIconStarBlue16Scaled.IsSet()) {
		SharedIcons::sIconStarBlue16Scaled
			= SharedIcons::_CreateIconForResource(RSRC_STAR_BLUE, 16);
	}
	return SharedIcons::sIconStarBlue16Scaled;
}


/*static*/ BitmapHolderRef
SharedIcons::IconStarGrey16Scaled()
{
	if (!SharedIcons::sIconStarGrey16Scaled.IsSet()) {
		SharedIcons::sIconStarGrey16Scaled
			= SharedIcons::_CreateIconForResource(RSRC_STAR_GREY, 16);
	}
	return SharedIcons::sIconStarGrey16Scaled;
}


/*static*/ BitmapHolderRef
SharedIcons::IconInstalled16Scaled()
{
	if (!SharedIcons::sIconInstalled16Scaled.IsSet()) {
		SharedIcons::sIconInstalled16Scaled
			= SharedIcons::_CreateIconForResource(RSRC_INSTALLED, 16);
	}
	return SharedIcons::sIconInstalled16Scaled;
}


/*static*/ BitmapHolderRef
SharedIcons::IconArrowLeft22Scaled()
{
	if (!SharedIcons::sIconArrowLeft22Scaled.IsSet()) {
		SharedIcons::sIconArrowLeft22Scaled
			= SharedIcons::_CreateIconForResource(RSRC_ARROW_LEFT, 22);
	}
	return SharedIcons::sIconArrowLeft22Scaled;
}


/*static*/ BitmapHolderRef
SharedIcons::IconArrowRight22Scaled()
{
	if (!SharedIcons::sIconArrowRight22Scaled.IsSet()) {
		SharedIcons::sIconArrowRight22Scaled
			= SharedIcons::_CreateIconForResource(RSRC_ARROW_RIGHT, 22);
	}
	return SharedIcons::sIconArrowRight22Scaled;
}


/*static*/ BitmapHolderRef
SharedIcons::IconHTMLPackage16Scaled()
{
	if (!SharedIcons::sIconHTMLPackage16Scaled.IsSet()) {
		SharedIcons::sIconHTMLPackage16Scaled
			= SharedIcons::_CreateIconForMimeType("text/html", 16);
	}
	return SharedIcons::sIconHTMLPackage16Scaled;
}


/*static*/ BitmapHolderRef
SharedIcons::IconNative16Scaled()
{
	if (!SharedIcons::sNative16Scaled.IsSet())
		SharedIcons::sNative16Scaled = SharedIcons::_CreateIconForResource(RSRC_NATIVE, 16);
	return SharedIcons::sNative16Scaled;
}


/*static*/ void
SharedIcons::UnsetAllIcons()
{
	sIconStarBlue12Scaled.Unset();
	sIconStarBlue16Scaled.Unset();
	sIconStarGrey16Scaled.Unset();
	sIconInstalled16Scaled.Unset();
	sIconArrowLeft22Scaled.Unset();
	sIconArrowRight22Scaled.Unset();
	sNative16Scaled.Unset();

	sIconHTMLPackage16Scaled.Unset();
}


/*static*/ BitmapHolderRef
SharedIcons::_CreateIconForResource(int32 resourceID, uint32 size)
{
	BitmapHolderRef result(NULL);

	if (SharedIcons::_CreateIconForResourceChecked(resourceID, size, &result) != B_OK) {
		HDERROR("unable to create bitmap for resource [%d]", resourceID);
		debugger("unable to create bitmap for resource");
			// the resource is bundled into the build product so not being able to find it is an
			// illegal state.
	}

	return result;
}


/*static*/ BitmapHolderRef
SharedIcons::_CreateIconForMimeType(const char* mimeType, uint32 size)
{
	BitmapHolderRef result(NULL);

	if (SharedIcons::_CreateIconForMimeTypeChecked(mimeType, size, &result) != B_OK)
		HDERROR("unable to create bitmap for mime type [%s]", mimeType);

	return result;
}


/*static*/ status_t
SharedIcons::_CreateIconForResourceChecked(int32 resourceID, uint32 size,
	BitmapHolderRef* bitmapHolderRef)
{
	if (size > MAX_IMAGE_SIZE || size == 0)
		return B_BAD_VALUE;

	BResources resources;
	status_t status = get_app_resources(resources);

	size_t dataSize;
	const void* data = NULL;

	if (status == B_OK)
		data = resources.LoadResource(B_VECTOR_ICON_TYPE, resourceID, &dataSize);

	if (data == NULL) {
		HDERROR("unable to read the resource [%d]", resourceID);
		status = B_ERROR;
	}

	BSize iconSize = BControlLook::ComposeIconSize(size);
	BBitmap* bitmap = new BBitmap(BRect(BPoint(0, 0), iconSize), 0, B_RGBA32);
	status = bitmap->InitCheck();

	if (status == B_OK)
		status = BIconUtils::GetVectorIcon(reinterpret_cast<const uint8*>(data), dataSize, bitmap);

	if (status != B_OK) {
		HDERROR("unable to parse the resource [%d] as a vector icon", resourceID);
		delete bitmap;
		bitmap = NULL;
	} else {
		*bitmapHolderRef = BitmapHolderRef(new(std::nothrow) BitmapHolder(bitmap), true);
	}

	return status;
}


/*static*/ status_t
SharedIcons::_CreateIconForMimeTypeChecked(const char* mimeTypeStr, uint32 size,
	BitmapHolderRef* bitmapHolderRef)
{
	if (size > MAX_IMAGE_SIZE || size == 0)
		return B_BAD_VALUE;

	BMimeType mimeType(mimeTypeStr);
	status_t status = mimeType.InitCheck();

	uint8* data;
	size_t dataSize;

	if (status == B_OK)
		status = mimeType.GetIcon(&data, &dataSize);

	BBitmap* bitmap = NULL;

	if (status == B_OK) {
		BSize iconSize = BControlLook::ComposeIconSize(size);
		bitmap = new BBitmap(BRect(BPoint(0, 0), iconSize), 0, B_RGBA32);
		status = bitmap->InitCheck();
	}

	if (status == B_OK)
		status = BIconUtils::GetVectorIcon(data, dataSize, bitmap);

	if (status != B_OK) {
		delete bitmap;
		bitmap = NULL;
	} else {
		*bitmapHolderRef = BitmapHolderRef(new(std::nothrow) BitmapHolder(bitmap), true);
	}

	return status;
}
