/*
 * Copyright 2004-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marcus Overhagen
 *		Axel Dörfler
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "AddOnManager.h"

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <image.h>
#include <Path.h>

#include <safemode_defs.h>
#include <syscalls.h>

#include "debug.h"
#include "media_server.h"

#include "FormatManager.h"
#include "MetaFormat.h"


//	#pragma mark - ImageLoader

/*!	The ImageLoader class is a convenience class to temporarily load
	an image file, and unload it on deconstruction automatically.
*/
class ImageLoader {
public:
	ImageLoader(BPath& path)
	{
		fImage = load_add_on(path.Path());
	}

	~ImageLoader()
	{
		if (fImage >= B_OK)
			unload_add_on(fImage);
	}

	status_t InitCheck() const { return fImage >= 0 ? B_OK : fImage; }
	image_id Image() const { return fImage; }

private:
	image_id	fImage;
};


static const directory_which sDirectories[] = {
	B_USER_ADDONS_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY,
	B_SYSTEM_ADDONS_DIRECTORY
};


//	#pragma mark -


AddOnManager::AddOnManager()
	:
 	fLock("add-on manager"),
 	fNextWriterFormatFamilyID(0),
 	fNextEncoderCodecInfoID(0),
 	fAddOnMonitorHandler(NULL),
 	fAddOnMonitor(NULL)
{
}


AddOnManager::~AddOnManager()
{
	if (fAddOnMonitor != NULL && fAddOnMonitor->Lock()) {
		fAddOnMonitor->RemoveHandler(fAddOnMonitorHandler);
		delete fAddOnMonitorHandler;
		fAddOnMonitor->Quit();
	}
}


void
AddOnManager::LoadState()
{
	_RegisterAddOns();
}


void
AddOnManager::SaveState()
{
}


status_t
AddOnManager::GetDecoderForFormat(xfer_entry_ref* _decoderRef,
	const media_format& format)
{
	if ((format.type == B_MEDIA_ENCODED_VIDEO
			|| format.type == B_MEDIA_ENCODED_AUDIO
			|| format.type == B_MEDIA_MULTISTREAM)
		&& format.Encoding() == 0) {
		return B_MEDIA_BAD_FORMAT;
	}
	if (format.type == B_MEDIA_NO_TYPE || format.type == B_MEDIA_UNKNOWN_TYPE)
		return B_MEDIA_BAD_FORMAT;

	BAutolock locker(fLock);

	printf("AddOnManager::GetDecoderForFormat: searching decoder for encoding "
		"%ld\n", format.Encoding());

	// Since the list of decoders is unsorted, we need to search for
	// a decoder by add-on directory, in order to maintain the shadowing
	// of system add-ons by user add-ons, in case they offer decoders
	// for the same format.

	BPath path;
	for (uint i = 0; i < sizeof(sDirectories) / sizeof(directory_which); i++) {
		if (find_directory(sDirectories[i], &path) != B_OK
			|| path.Append("media/plugins") != B_OK) {
			printf("AddOnManager::GetDecoderForFormat: failed to construct "
				"path for directory %u\n", i);
			continue;
		}
		if (_FindDecoder(format, path, _decoderRef))
			return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
AddOnManager::GetReaders(xfer_entry_ref* outRefs, int32* outCount,
	int32 maxCount)
{
	BAutolock locker(fLock);

	*outCount = 0;

	// See GetDecoderForFormat() for why we need to scan the list by path.

	BPath path;
	for (uint i = 0; i < sizeof(sDirectories) / sizeof(directory_which); i++) {
		if (find_directory(sDirectories[i], &path) != B_OK
			|| path.Append("media/plugins") != B_OK) {
			printf("AddOnManager::GetReaders: failed to construct "
				"path for directory %u\n", i);
			continue;
		}
		_GetReaders(path, outRefs, outCount, maxCount);
	}
	
	return B_OK;
}


status_t
AddOnManager::GetEncoder(xfer_entry_ref* _encoderRef, int32 id)
{
	BAutolock locker(fLock);

	encoder_info* info;
	for (fEncoderList.Rewind(); fEncoderList.GetNext(&info);) {
		// check if the encoder matches the supplied format
		if (info->internalID == (uint32)id) {
			printf("AddOnManager::GetEncoderForFormat: found encoder %s for "
				"id %ld\n", info->ref.name, id);

			*_encoderRef = info->ref;
			return B_OK;
		}
	}

	printf("AddOnManager::GetEncoderForFormat: failed to find encoder for id "
		"%ld\n", id);

	return B_ENTRY_NOT_FOUND;
}


status_t
AddOnManager::GetWriter(xfer_entry_ref* _ref, uint32 internalID)
{
	BAutolock locker(fLock);

	writer_info* info;
	for (fWriterList.Rewind(); fWriterList.GetNext(&info);) {
		if (info->internalID == internalID) {
			printf("AddOnManager::GetWriter: found writer %s for "
				"internal_id %lu\n", info->ref.name, internalID);

			*_ref = info->ref;
			return B_OK;
		}
	}

	return B_ERROR;
}


status_t
AddOnManager::GetFileFormat(media_file_format* _fileFormat, int32 cookie)
{
	BAutolock locker(fLock);

	media_file_format* fileFormat;
	if (fWriterFileFormats.Get(cookie, &fileFormat)) {
		*_fileFormat = *fileFormat;
		return B_OK;
	}

	return B_BAD_INDEX;
}


status_t
AddOnManager::GetCodecInfo(media_codec_info* _codecInfo,
	media_format_family* _formatFamily,
	media_format* _inputFormat, media_format* _outputFormat, int32 cookie)
{
	BAutolock locker(fLock);

	encoder_info* info;
	if (fEncoderList.Get(cookie, &info)) {
		*_codecInfo = info->codecInfo;
		*_formatFamily = info->formatFamily;
		*_inputFormat = info->intputFormat;
		*_outputFormat = info->outputFormat;
		return B_OK;
	}

	return B_BAD_INDEX;
}


// #pragma mark -


void
AddOnManager::_RegisterAddOns()
{
	class CodecHandler : public AddOnMonitorHandler {
	private:
		AddOnManager* fManager;

	public:
		CodecHandler(AddOnManager* manager)
		{
			fManager = manager;
		}

		virtual void AddOnCreated(const add_on_entry_info* entryInfo)
		{
		}

		virtual void AddOnEnabled(const add_on_entry_info* entryInfo)
		{
			entry_ref ref;
			make_entry_ref(entryInfo->dir_nref.device,
				entryInfo->dir_nref.node, entryInfo->name, &ref);
			fManager->_RegisterAddOn(ref);
		}

		virtual void AddOnDisabled(const add_on_entry_info* entryInfo)
		{
			entry_ref ref;
			make_entry_ref(entryInfo->dir_nref.device,
				entryInfo->dir_nref.node, entryInfo->name, &ref);
			fManager->_UnregisterAddOn(ref);
		}

		virtual void AddOnRemoved(const add_on_entry_info* entryInfo)
		{
		}
	};

	fAddOnMonitorHandler = new CodecHandler(this);
	fAddOnMonitor = new AddOnMonitor(fAddOnMonitorHandler);

	// get safemode option for disabling user add-ons

	char buffer[16];
	size_t size = sizeof(buffer);

	bool disableUserAddOns = _kern_get_safemode_option(
			B_SAFEMODE_DISABLE_USER_ADD_ONS, buffer, &size) == B_OK
		&& (!strcasecmp(buffer, "true")
			|| !strcasecmp(buffer, "yes")
			|| !strcasecmp(buffer, "on")
			|| !strcasecmp(buffer, "enabled")
			|| !strcmp(buffer, "1"));

	node_ref nref;
	BDirectory directory;
	BPath path;
	for (uint i = 0; i < sizeof(sDirectories) / sizeof(directory_which); i++) {
		if (disableUserAddOns && i <= 1)
			continue;

		if (find_directory(sDirectories[i], &path) == B_OK
			&& path.Append("media/plugins") == B_OK
			&& directory.SetTo(path.Path()) == B_OK
			&& directory.GetNodeRef(&nref) == B_OK) {
			fAddOnMonitorHandler->AddDirectory(&nref);
				// NOTE: This may already start registering add-ons in the
				// AddOnMonitor looper thread after the call returns!
		}
	}
}


status_t
AddOnManager::_RegisterAddOn(const entry_ref& ref)
{
	BPath path(&ref);

	printf("AddOnManager::_RegisterAddOn(): trying to load \"%s\"\n",
		path.Path());

	ImageLoader loader(path);
	status_t status = loader.InitCheck();
	if (status != B_OK)
		return status;

	MediaPlugin* (*instantiate_plugin_func)();

	if (get_image_symbol(loader.Image(), "instantiate_plugin",
			B_SYMBOL_TYPE_TEXT, (void**)&instantiate_plugin_func) < B_OK) {
		printf("AddOnManager::_RegisterAddOn(): can't find instantiate_plugin "
			"in \"%s\"\n", path.Path());
		return B_BAD_TYPE;
	}

	MediaPlugin* plugin = (*instantiate_plugin_func)();
	if (plugin == NULL) {
		printf("AddOnManager::_RegisterAddOn(): instantiate_plugin in \"%s\" "
			"returned NULL\n", path.Path());
		return B_ERROR;
	}

	ReaderPlugin* reader = dynamic_cast<ReaderPlugin*>(plugin);
	if (reader != NULL)
		_RegisterReader(reader, ref);

	DecoderPlugin* decoder = dynamic_cast<DecoderPlugin*>(plugin);
	if (decoder != NULL)
		_RegisterDecoder(decoder, ref);

	WriterPlugin* writer = dynamic_cast<WriterPlugin*>(plugin);
	if (writer != NULL)
		_RegisterWriter(writer, ref);

	EncoderPlugin* encoder = dynamic_cast<EncoderPlugin*>(plugin);
	if (encoder != NULL)
		_RegisterEncoder(encoder, ref);

	delete plugin;

	return B_OK;
}


status_t
AddOnManager::_UnregisterAddOn(const entry_ref& ref)
{
BPath path(&ref);
printf("AddOnManager::_UnregisterAddOn(): trying to unload \"%s\"\n",
	path.Path());

	BAutolock locker(fLock);

	// Remove any Readers exported by this add-on
	reader_info* readerInfo;
	for (fReaderList.Rewind(); fReaderList.GetNext(&readerInfo);) {
		if (readerInfo->ref == ref) {
printf("removing reader '%s'\n", readerInfo->ref.name);
			fReaderList.RemoveCurrent();
			break;
		}
	}

	// Remove any Decoders exported by this add-on
	decoder_info* decoderInfo;
	for (fDecoderList.Rewind(); fDecoderList.GetNext(&decoderInfo);) {
		if (decoderInfo->ref == ref) {
printf("removing decoder '%s'\n", decoderInfo->ref.name);
			media_format* format;
			for (decoderInfo->formats.Rewind();
				decoderInfo->formats.GetNext(&format);) {
				gFormatManager->RemoveFormat(*format);
			}
			fDecoderList.RemoveCurrent();
			break;
		}
	}

	// Remove any Writers exported by this add-on
	writer_info* writerInfo;
	for (fWriterList.Rewind(); fWriterList.GetNext(&writerInfo);) {
		if (writerInfo->ref == ref) {
			// Remove any formats from this writer
			media_file_format* writerFormat;
			for (fWriterFileFormats.Rewind();
				fWriterFileFormats.GetNext(&writerFormat);) {
				if (writerFormat->id.internal_id == writerInfo->internalID)
					fWriterFileFormats.RemoveCurrent();
			}
printf("removing writer '%s'\n", writerInfo->ref.name);
			fWriterList.RemoveCurrent();
			break;
		}
	}

	encoder_info* encoderInfo;
	for (fEncoderList.Rewind(); fEncoderList.GetNext(&encoderInfo);) {
		if (encoderInfo->ref == ref) {
printf("removing encoder '%s', id %lu\n", encoderInfo->ref.name,
	encoderInfo->internalID);
			fEncoderList.RemoveCurrent();
			// Keep going, since we add multiple encoder infos per add-on.
		}
	}

	return B_OK;
}


void
AddOnManager::_RegisterReader(ReaderPlugin* reader, const entry_ref& ref)
{
	BAutolock locker(fLock);

	reader_info* pinfo;
	for (fReaderList.Rewind(); fReaderList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this reader
			return;
		}
	}

	printf("AddOnManager::_RegisterReader, name %s\n", ref.name);

	reader_info info;
	info.ref = ref;

	fReaderList.Insert(info);
}


void
AddOnManager::_RegisterDecoder(DecoderPlugin* plugin, const entry_ref& ref)
{
	BAutolock locker(fLock);

	decoder_info* pinfo;
	for (fDecoderList.Rewind(); fDecoderList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this decoder
			return;
		}
	}

	printf("AddOnManager::_RegisterDecoder, name %s\n", ref.name);

	decoder_info info;
	info.ref = ref;

	media_format* formats = 0;
	size_t count = 0;
	if (plugin->GetSupportedFormats(&formats, &count) != B_OK) {
		printf("AddOnManager::_RegisterDecoder(): plugin->GetSupportedFormats"
			"(...) failed!\n");
		return;
	}
	for (uint i = 0 ; i < count ; i++)
		info.formats.Insert(formats[i]);

	fDecoderList.Insert(info);
}


void
AddOnManager::_RegisterWriter(WriterPlugin* writer, const entry_ref& ref)
{
	BAutolock locker(fLock);

	writer_info* pinfo;
	for (fWriterList.Rewind(); fWriterList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this writer
			return;
		}
	}

	printf("AddOnManager::_RegisterWriter, name %s\n", ref.name);

	writer_info info;
	info.ref = ref;
	info.internalID = fNextWriterFormatFamilyID++;

	// Get list of support media_file_formats...
	const media_file_format* fileFormats = NULL;
	size_t count = 0;
	if (writer->GetSupportedFileFormats(&fileFormats, &count) != B_OK) {
		printf("AddOnManager::_RegisterWriter(): "
			"plugin->GetSupportedFileFormats(...) failed!\n");
		return;
	}
	for (uint i = 0 ; i < count ; i++) {
		// Generate a proper ID before inserting this format, this encodes
		// the specific plugin in the media_file_format.
		media_file_format fileFormat = fileFormats[i];
		fileFormat.id.node = ref.directory;
		fileFormat.id.device = ref.device;
		fileFormat.id.internal_id = info.internalID;

		fWriterFileFormats.Insert(fileFormat);
	}

	fWriterList.Insert(info);
}


void
AddOnManager::_RegisterEncoder(EncoderPlugin* plugin, const entry_ref& ref)
{
	BAutolock locker(fLock);

	encoder_info* pinfo;
	for (fEncoderList.Rewind(); fEncoderList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// We already know this encoder. When we reject encoders with
			// the same name, we allow the user to overwrite system encoders
			// in her home folder.
			return;
		}
	}

	printf("AddOnManager::_RegisterEncoder, name %s\n", ref.name);

	// Get list of supported encoders...

	encoder_info info;
	info.ref = ref;
	info.internalID = fNextEncoderCodecInfoID++;

	int32 cookie = 0;

	while (true) {
		memset(&info.codecInfo, 0, sizeof(media_codec_info));
		memset(&info.intputFormat, 0, sizeof(media_format));
		memset(&info.outputFormat, 0, sizeof(media_format));
		if (plugin->RegisterNextEncoder(&cookie,
			&info.codecInfo, &info.formatFamily, &info.intputFormat,
			&info.outputFormat) != B_OK) {
			break;
		}
		info.codecInfo.id = info.internalID;
		// NOTE: info.codecInfo.sub_id is for private use by the Encoder,
		// we don't touch it, but it is maintained and passed back to the
		// EncoderPlugin in NewEncoder(media_codec_info).

		if (!fEncoderList.Insert(info))
			break;
	}
}


bool
AddOnManager::_FindDecoder(const media_format& format, const BPath& path,
	xfer_entry_ref* _decoderRef)
{
	node_ref nref;
	BDirectory directory;
	if (directory.SetTo(path.Path()) != B_OK
		|| directory.GetNodeRef(&nref) != B_OK) {
		return false;
	}

	decoder_info* info;
	for (fDecoderList.Rewind(); fDecoderList.GetNext(&info);) {
		if (info->ref.directory != nref.node)
			continue;

		media_format* decoderFormat;
		for (info->formats.Rewind(); info->formats.GetNext(&decoderFormat);) {
			// check if the decoder matches the supplied format
			if (!decoderFormat->Matches(&format))
				continue;

			printf("AddOnManager::GetDecoderForFormat: found decoder %s/%s "
				"for encoding %ld\n", path.Path(), info->ref.name,
				decoderFormat->Encoding());

			*_decoderRef = info->ref;
			return true;
		}
	}
	return false;
}


void
AddOnManager::_GetReaders(const BPath& path, xfer_entry_ref* outRefs,
	int32* outCount, int32 maxCount)
{
	node_ref nref;
	BDirectory directory;
	if (directory.SetTo(path.Path()) != B_OK
		|| directory.GetNodeRef(&nref) != B_OK) {
		return;
	}

	reader_info* info;
	for (fReaderList.Rewind(); fReaderList.GetNext(&info)
		&& *outCount < maxCount;) {
		if (info->ref.directory != nref.node)
			continue;

		outRefs[*outCount] = info->ref;
		(*outCount)++;
	}
}
