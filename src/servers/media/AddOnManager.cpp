/*
** Copyright 2004, the OpenBeOS project. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Authors: Marcus Overhagen, Axel Dörfler
*/


#include "AddOnManager.h"
#include "FormatManager.h"
#include "MetaFormat.h"
#include "media_server.h"
#include "debug.h"

#include <FindDirectory.h>
#include <Path.h>
#include <Entry.h>
#include <Directory.h>
#include <Autolock.h>
#include <image.h>

#include <stdio.h>
#include <string.h>


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


static status_t
register_decoder(const media_format_description *descriptions,
	int32 descriptionCount, media_format *format, uint32 flags)
{
	return gAddOnManager->MakeDecoderFormats(descriptions, descriptionCount, format, flags);
}

/* not yet used
static status_t
register_encoder(const media_format_description *descriptions,
	int32 descriptionCount, media_format *format, uint32 flags)
{
	return gAddOnManager->MakeEncoderFormats(descriptions, descriptionCount, format, flags);
}
*/

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


status_t
AddOnManager::GetDecoderForFormat(xfer_entry_ref *_decoderRef,
	const media_format &format)
{
	if ((format.type == B_MEDIA_ENCODED_VIDEO
		|| format.type == B_MEDIA_ENCODED_AUDIO
		|| format.type == B_MEDIA_MULTISTREAM)
		&& format.Encoding() == 0)
		return B_MEDIA_BAD_FORMAT;

	BAutolock locker(fLock);

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
}


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

	MediaPlugin *(*instantiate_plugin_func)();

	if (get_image_symbol(loader.Image(), "instantiate_plugin",
			B_SYMBOL_TYPE_TEXT, (void **)&instantiate_plugin_func) < B_OK) {
		printf("AddOnManager::RegisterAddOn(): can't find instantiate_plugin in \"%s\"\n",
			path.Path());
		return B_BAD_TYPE;
	}

	MediaPlugin *plugin = (*instantiate_plugin_func)();
	if (plugin == NULL) {
		printf("AddOnManager::RegisterAddOn(): instantiate_plugin in \"%s\" returned NULL\n",
			path.Path());
		return B_ERROR;
	}

	// ToDo: remove any old formats describing this add-on!!

	ReaderPlugin *reader = dynamic_cast<ReaderPlugin *>(plugin);
	if (reader != NULL)
		RegisterReader(reader, ref);

	DecoderPlugin *decoder = dynamic_cast<DecoderPlugin *>(plugin);
	if (decoder != NULL)
		RegisterDecoder(decoder, ref);

	delete plugin;
}


void
AddOnManager::RegisterAddOnsFor(BPath path)
{
	path.Append("media/plugins");

	BDirectory directory(path.Path());
	if (directory.InitCheck() != B_OK)
		return;

	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK)
		RegisterAddOn(entry);
}


void
AddOnManager::RegisterAddOns()
{
	BPath path;

	// user add-ons come first, so that they can lay over system
	// add-ons (add-ons are identified by name for registration)

	const directory_which directories[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY,
	};

	for (uint32 i = 0; i < sizeof(directories) / sizeof(directory_which); i++) {
		if (find_directory(directories[i], &path) == B_OK)
			RegisterAddOnsFor(path);
	}

	// ToDo: this is for our own convenience only, and should be removed
	//	in the final release
	path.SetTo("/boot/home/develop/openbeos/current/distro/x86.R1/beos/system/add-ons");
	RegisterAddOnsFor(path);
}


void
AddOnManager::RegisterReader(ReaderPlugin *reader, const entry_ref &ref)
{
	BAutolock locker(fLock);

	reader_info *pinfo;
	for (fReaderList.Rewind(); fReaderList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this reader
			return;
		}
	}

	printf("AddOnManager::RegisterReader, name %s\n", ref.name);

	reader_info info;
	info.ref = ref;

	fReaderList.Insert(info);
}


void
AddOnManager::RegisterDecoder(DecoderPlugin *decoder, const entry_ref &ref)
{
	BAutolock locker(fLock);

	decoder_info *pinfo;
	for (fDecoderList.Rewind(); fDecoderList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this decoder
			return;
		}
	}

	printf("AddOnManager::RegisterDecoder, name %s\n", ref.name);

	decoder_info info;
	info.ref = ref;

	fCurrentDecoder = &info;
	BPrivate::media::_gMakeFormatHook = register_decoder;

	if (decoder->RegisterDecoder() != B_OK) {
		printf("AddOnManager::RegisterDecoder(): decoder->RegisterDecoder() failed!\n");
		return;
	}

	fDecoderList.Insert(info);
	fCurrentDecoder = NULL;
	BPrivate::media::_gMakeFormatHook = NULL;
}


status_t 
AddOnManager::MakeEncoderFormats(const media_format_description *descriptions,
	int32 descriptionCount, media_format *format, uint32 flags)
{
	//ASSERT(fCurrentEncoder != NULL);

	status_t status = gFormatManager->RegisterEncoder(descriptions,
							descriptionCount, format, flags);
	if (status < B_OK)
		return status;

	//fCurrentEncoder->format = *format;
	return B_OK;
}


status_t 
AddOnManager::MakeDecoderFormats(const media_format_description *descriptions,
	int32 descriptionCount, media_format *format, uint32 flags)
{
	ASSERT(fCurrentDecoder != NULL);

	status_t status = gFormatManager->RegisterDecoder(descriptions,
							descriptionCount, format, flags);
	if (status < B_OK)
		return status;

	fCurrentDecoder->formats.Insert(*format);
	return B_OK;
}

