/*
** Copyright 2004, the OpenBeOS project. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Authors: Marcus Overhagen, Axel Dörfler
*/


#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

#include <image.h>
#include <stdio.h>
#include <string.h>

#include "AddOnManager.h"

//	#pragma mark ImageLoader

/** The ImageLoader class is a convenience class to temporarily load
 *	an image file, and unload it on deconstruction automatically.
 */

class ImageLoader {
	public:
		ImageLoader(BPath &path)
		{
			fImage = load_add_on(path.Path());
		}

		~ImageLoader()
		{
			if (fImage >= B_OK)
				unload_add_on(fImage);
		}

		status_t InitCheck() const { return fImage; }
		image_id Image() const { return fImage; }

	private:
		image_id	fImage;
};


//	#pragma mark -


AddOnManager::AddOnManager()
	:
 	fLock("add-on manager")
{
}


AddOnManager::~AddOnManager()
{
}


void
AddOnManager::LoadState()
{
	RegisterAddOns();
}


void
AddOnManager::SaveState()
{
}


/*status_t
AddOnManager::GetDecoderForFormat(xfer_entry_ref *_decoderRef,
	const media_format &format)
{
	if ((format.type == B_MEDIA_ENCODED_VIDEO
		|| format.type == B_MEDIA_ENCODED_AUDIO
		|| format.type == B_MEDIA_MULTISTREAM)
		&& format.Encoding() == 0)
		return B_MEDIA_BAD_FORMAT;
	if (format.type == B_MEDIA_NO_TYPE
		|| format.type == B_MEDIA_UNKNOWN_TYPE)
		return B_MEDIA_BAD_FORMAT;


	BAutolock locker(fLock);

	printf("AddOnManager::GetDecoderForFormat: searching decoder for encoding %ld\n", format.Encoding());

	decoder_info *info;
	for (fDecoderList.Rewind(); fDecoderList.GetNext(&info);) {
		media_format *decoderFormat;
		for (info->formats.Rewind(); info->formats.GetNext(&decoderFormat);) {
			// check if the decoder matches the supplied format
			if (!decoderFormat->Matches(&format))
				continue;

			printf("AddOnManager::GetDecoderForFormat: found decoder %s for encoding %ld\n",
				info->ref.name, decoderFormat->Encoding());

			*_decoderRef = info->ref;
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;	
}
									

status_t
AddOnManager::GetReaders(xfer_entry_ref *out_res, int32 *out_count, int32 max_count)
{
	BAutolock locker(fLock);

	*out_count = 0;

	fReaderList.Rewind();
	reader_info *info;
	for (*out_count = 0; fReaderList.GetNext(&info) && *out_count <= max_count; *out_count += 1)
		out_res[*out_count] = info->ref;

	return B_OK;
}*/


status_t
AddOnManager::RegisterAddOn(BEntry &entry)
{
	BPath path(&entry);

	entry_ref ref;
	status_t status = entry.GetRef(&ref);
	if (status < B_OK)
		return status;

	printf("AddOnManager::RegisterAddOn(): trying to load \"%s\"\n", path.Path());

	ImageLoader loader(path);
	if ((status = loader.InitCheck()) < B_OK)
		return status;
		
	BEntry parent;
	entry.GetParent(&parent);
	BPath parentPath(&parent);
	BString pathString = parentPath.Path();
	
	if (pathString.FindFirst("input_server/devices")>0) {
		BInputServerDevice *(*instantiate_func)();

		if (get_image_symbol(loader.Image(), "instantiate_input_device",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			printf("AddOnManager::RegisterAddOn(): can't find instantiate_input_device in \"%s\"\n",
				path.Path());
			return B_BAD_TYPE;
		}
	
		BInputServerDevice *isd = (*instantiate_func)();
		if (isd == NULL) {
			printf("AddOnManager::RegisterAddOn(): instantiate_input_device in \"%s\" returned NULL\n",
				path.Path());
			return B_ERROR;
		}
		
		RegisterDevice(isd, ref);
	
	} else if (pathString.FindFirst("input_server/filters")>0) {
		BInputServerFilter *(*instantiate_func)();

		if (get_image_symbol(loader.Image(), "instantiate_input_filter",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			printf("AddOnManager::RegisterAddOn(): can't find instantiate_input_filter in \"%s\"\n",
				path.Path());
			return B_BAD_TYPE;
		}
	
		BInputServerFilter *isf = (*instantiate_func)();
		if (isf == NULL) {
			printf("AddOnManager::RegisterAddOn(): instantiate_input_filter in \"%s\" returned NULL\n",
				path.Path());
			return B_ERROR;
		}
		
		RegisterFilter(isf, ref);
	
	} else if (pathString.FindFirst("input_server/methods")>0) {
		BInputServerMethod *(*instantiate_func)();

		if (get_image_symbol(loader.Image(), "instantiate_input_method",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			printf("AddOnManager::RegisterAddOn(): can't find instantiate_input_method in \"%s\"\n",
				path.Path());
			return B_BAD_TYPE;
		}
	
		BInputServerMethod *ism = (*instantiate_func)();
		if (ism == NULL) {
			printf("AddOnManager::RegisterAddOn(): instantiate_input_method in \"%s\" returned NULL\n",
				path.Path());
			return B_ERROR;
		}
		
		RegisterMethod(ism, ref);
	
	}
	
	return B_OK;
}


void
AddOnManager::RegisterAddOns()
{
	class IAHandler : public AddOnMonitorHandler {
	private:
		AddOnManager * fManager;
	public:
		IAHandler(AddOnManager * manager) {
			fManager = manager;
		}
		virtual void	AddOnCreated(const add_on_entry_info * entry_info) {
		}
		virtual void	AddOnEnabled(const add_on_entry_info * entry_info) {
			entry_ref ref;
			make_entry_ref(entry_info->dir_nref.device, entry_info->dir_nref.node,
			               entry_info->name, &ref);
			BEntry entry(&ref, false);
			fManager->RegisterAddOn(entry);
		}
		virtual void	AddOnDisabled(const add_on_entry_info * entry_info) {
		}
		virtual void	AddOnRemoved(const add_on_entry_info * entry_info) {
		}
	};

	const directory_which directories[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY,
	};
	const char subDirectories[][24] = {
//		"input_server/devices",
		"input_server/filters",
		"input_server/methods",
	};
	fHandler = new IAHandler(this);
	fAddOnMonitor = new AddOnMonitor(fHandler);

	node_ref nref;
	BDirectory directory;
	BPath path;
	for (uint i = 0 ; i < sizeof(directories) / sizeof(directory_which) ; i++)
		for (uint j = 0 ; j < 2 ; j++) {
			if ((find_directory(directories[i], &path) == B_OK)
				&& (path.Append(subDirectories[j]) == B_OK)
				&& (directory.SetTo(path.Path()) == B_OK) 
				&& (directory.GetNodeRef(&nref) == B_OK)) {
				fHandler->AddDirectory(&nref);
			}
		}

	// ToDo: this is for our own convenience only, and should be removed
	//	in the final release
	//if ((directory.SetTo("/boot/home/develop/openbeos/current/distro/x86.R1/beos/system/add-ons/media/plugins") == B_OK)
	//	&& (directory.GetNodeRef(&nref) == B_OK)) {
	//	fHandler->AddDirectory(&nref);
	//}
}


void
AddOnManager::RegisterDevice(BInputServerDevice *isd, const entry_ref &ref)
{
	BAutolock locker(fLock);

	device_info *pinfo;
	for (fDeviceList.Rewind(); fDeviceList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this device
			return;
		}
	}

	PRINT(("AddOnManager::RegisterDevice, name %s\n", ref.name));

	device_info info;
	info.ref = ref;

	fDeviceList.Insert(info);
}


void
AddOnManager::RegisterFilter(BInputServerFilter *filter, const entry_ref &ref)
{
	BAutolock locker(fLock);

	filter_info *pinfo;
	for (fFilterList.Rewind(); fFilterList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this ref
			return;
		}
	}

	PRINT(("%s, name %s\n", __PRETTY_FUNCTION__, ref.name));

	filter_info info;
	info.ref = ref;

	fFilterList.Insert(info);
}


void
AddOnManager::RegisterMethod(BInputServerMethod *method, const entry_ref &ref)
{
	BAutolock locker(fLock);

	method_info *pinfo;
	for (fMethodList.Rewind(); fMethodList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this ref
			return;
		}
	}

	PRINT(("%s, name %s\n", __PRETTY_FUNCTION__, ref.name));

	method_info info;
	info.ref = ref;

	fMethodList.Insert(info);
}


