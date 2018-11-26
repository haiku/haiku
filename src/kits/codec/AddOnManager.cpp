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

#include <Architecture.h>
#include <AutoDeleter.h>
#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <image.h>
#include <Path.h>

#include "MediaDebug.h"

#include "FormatManager.h"
#include "MetaFormat.h"


namespace BCodecKit {
namespace BPrivate {

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


//	#pragma mark -


AddOnManager::AddOnManager()
	:
	fLock("add-on manager"),
	fNextWriterFormatFamilyID(0),
	fNextEncoderCodecInfoID(0)
{
}


AddOnManager::~AddOnManager()
{
}


AddOnManager AddOnManager::sInstance;


/* static */ AddOnManager*
AddOnManager::GetInstance()
{
	return &sInstance;
}


status_t
AddOnManager::GetDecoderForFormat(entry_ref* _decoderRef,
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
	RegisterAddOns();

	// Since the list of decoders is unsorted, we need to search for
	// a decoder by add-on directory, in order to maintain the shadowing
	// of system add-ons by user add-ons, in case they offer decoders
	// for the same format.

	char** directories = NULL;
	size_t directoryCount = 0;

	if (find_paths_etc(get_architecture(), B_FIND_PATH_ADD_ONS_DIRECTORY,
			"media/plugins", B_FIND_PATH_EXISTING_ONLY, &directories,
			&directoryCount) != B_OK) {
		printf("AddOnManager::GetDecoderForFormat: failed to locate plugins\n");
		return B_ENTRY_NOT_FOUND;
	}

	MemoryDeleter directoriesDeleter(directories);

	BPath path;
	for (uint i = 0; i < directoryCount; i++) {
		path.SetTo(directories[i]);
		if (_FindDecoder(format, path, _decoderRef))
			return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
AddOnManager::GetEncoderForFormat(entry_ref* _encoderRef,
	const media_format& outputFormat)
{
	if ((outputFormat.type == B_MEDIA_RAW_VIDEO
			|| outputFormat.type == B_MEDIA_RAW_AUDIO)) {
		return B_MEDIA_BAD_FORMAT;
	}

	if (outputFormat.type == B_MEDIA_NO_TYPE
			|| outputFormat.type == B_MEDIA_UNKNOWN_TYPE) {
		return B_MEDIA_BAD_FORMAT;
	}

	BAutolock locker(fLock);
	RegisterAddOns();

	char** directories = NULL;
	size_t directoryCount = 0;

	if (find_paths_etc(get_architecture(), B_FIND_PATH_ADD_ONS_DIRECTORY,
			"media/plugins", B_FIND_PATH_EXISTING_ONLY, &directories,
			&directoryCount) != B_OK) {
		printf("AddOnManager::GetDecoderForFormat: failed to locate plugins\n");
		return B_ENTRY_NOT_FOUND;
	}

	MemoryDeleter directoriesDeleter(directories);

	BPath path;
	for (uint i = 0; i < directoryCount; i++) {
		path.SetTo(directories[i]);
		if (_FindEncoder(outputFormat, path, _encoderRef))
			return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
AddOnManager::GetReaders(entry_ref* outRefs, int32* outCount,
	int32 maxCount)
{
	BAutolock locker(fLock);
	RegisterAddOns();

	*outCount = 0;

	// See GetDecoderForFormat() for why we need to scan the list by path.

	char** directories = NULL;
	size_t directoryCount = 0;

	if (find_paths_etc(get_architecture(), B_FIND_PATH_ADD_ONS_DIRECTORY,
			"media/plugins", B_FIND_PATH_EXISTING_ONLY, &directories,
			&directoryCount) != B_OK) {
		printf("AddOnManager::GetReaders: failed to locate plugins\n");
		return B_ENTRY_NOT_FOUND;
	}

	MemoryDeleter directoriesDeleter(directories);

	BPath path;
	for (uint i = 0; i < directoryCount; i++) {
		path.SetTo(directories[i]);
		_GetReaders(path, outRefs, outCount, maxCount);
	}

	return B_OK;
}


status_t
AddOnManager::GetStreamers(entry_ref* outRefs, int32* outCount,
	int32 maxCount)
{
	BAutolock locker(fLock);
	RegisterAddOns();

	int32 count = 0;
	streamer_info* info;
	for (fStreamerList.Rewind(); fStreamerList.GetNext(&info);) {
			if (count == maxCount)
				break;

			*outRefs = info->ref;
			outRefs++;
			count++;
	}

	*outCount = count;
	return B_OK;
}


status_t
AddOnManager::GetEncoder(entry_ref* _encoderRef, int32 id)
{
	BAutolock locker(fLock);
	RegisterAddOns();

	encoder_info* info;
	for (fEncoderList.Rewind(); fEncoderList.GetNext(&info);) {
		// check if the encoder matches the supplied format
		if (info->internalID == (uint32)id) {
			*_encoderRef = info->ref;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
AddOnManager::GetWriter(entry_ref* _ref, uint32 internalID)
{
	BAutolock locker(fLock);
	RegisterAddOns();

	writer_info* info;
	for (fWriterList.Rewind(); fWriterList.GetNext(&info);) {
		if (info->internalID == internalID) {
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
	RegisterAddOns();

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
	RegisterAddOns();

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
AddOnManager::RegisterAddOns()
{
	// Check if add-ons are already registered.
	if (!fReaderList.IsEmpty() || !fWriterList.IsEmpty()
		|| !fDecoderList.IsEmpty() || !fEncoderList.IsEmpty()) {
		return;
	}

	char** directories = NULL;
	size_t directoryCount = 0;

	if (find_paths_etc(get_architecture(), B_FIND_PATH_ADD_ONS_DIRECTORY,
			"media/plugins", B_FIND_PATH_EXISTING_ONLY, &directories,
			&directoryCount) != B_OK) {
		return;
	}

	MemoryDeleter directoriesDeleter(directories);

	BPath path;
	for (uint i = 0; i < directoryCount; i++) {
		BDirectory directory;
		if (directory.SetTo(directories[i]) == B_OK) {
			entry_ref ref;
			while(directory.GetNextRef(&ref) == B_OK)
				_RegisterAddOn(ref);
		}
	}
}


status_t
AddOnManager::_RegisterAddOn(const entry_ref& ref)
{
	BPath path(&ref);

	ImageLoader loader(path);
	status_t status = loader.InitCheck();
	if (status != B_OK)
		return status;

	BMediaPlugin* (*instantiate_plugin_func)();

	if (get_image_symbol(loader.Image(), "instantiate_plugin",
			B_SYMBOL_TYPE_TEXT, (void**)&instantiate_plugin_func) < B_OK) {
		printf("AddOnManager::_RegisterAddOn(): can't find instantiate_plugin "
			"in \"%s\"\n", path.Path());
		return B_BAD_TYPE;
	}

	BMediaPlugin* plugin = (*instantiate_plugin_func)();
	if (plugin == NULL) {
		printf("AddOnManager::_RegisterAddOn(): instantiate_plugin in \"%s\" "
			"returned NULL\n", path.Path());
		return B_ERROR;
	}

	BReaderPlugin* reader = dynamic_cast<BReaderPlugin*>(plugin);
	if (reader != NULL)
		_RegisterReader(reader, ref);

	BDecoderPlugin* decoder = dynamic_cast<BDecoderPlugin*>(plugin);
	if (decoder != NULL)
		_RegisterDecoder(decoder, ref);

	BWriterPlugin* writer = dynamic_cast<BWriterPlugin*>(plugin);
	if (writer != NULL)
		_RegisterWriter(writer, ref);

	BEncoderPlugin* encoder = dynamic_cast<BEncoderPlugin*>(plugin);
	if (encoder != NULL)
		_RegisterEncoder(encoder, ref);

	BStreamerPlugin* streamer = dynamic_cast<BStreamerPlugin*>(plugin);
	if (streamer != NULL)
		_RegisterStreamer(streamer, ref);

	delete plugin;

	return B_OK;
}


status_t
AddOnManager::_UnregisterAddOn(const entry_ref& ref)
{
	BAutolock locker(fLock);

	// Remove any Readers exported by this add-on
	reader_info* readerInfo;
	for (fReaderList.Rewind(); fReaderList.GetNext(&readerInfo);) {
		if (readerInfo->ref == ref) {
			fReaderList.RemoveCurrent();
			break;
		}
	}

	// Remove any Decoders exported by this add-on
	decoder_info* decoderInfo;
	for (fDecoderList.Rewind(); fDecoderList.GetNext(&decoderInfo);) {
		if (decoderInfo->ref == ref) {
			media_format* format;
			for (decoderInfo->formats.Rewind();
					decoderInfo->formats.GetNext(&format);) {
				FormatManager::GetInstance()->RemoveFormat(*format);
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
			fWriterList.RemoveCurrent();
			break;
		}
	}

	encoder_info* encoderInfo;
	for (fEncoderList.Rewind(); fEncoderList.GetNext(&encoderInfo);) {
		if (encoderInfo->ref == ref) {
			fEncoderList.RemoveCurrent();
			// Keep going, since we add multiple encoder infos per add-on.
		}
	}

	return B_OK;
}


void
AddOnManager::_RegisterReader(BReaderPlugin* reader, const entry_ref& ref)
{
	BAutolock locker(fLock);

	reader_info* pinfo;
	for (fReaderList.Rewind(); fReaderList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this reader
			return;
		}
	}

	reader_info info;
	info.ref = ref;

	fReaderList.Insert(info);
}


void
AddOnManager::_RegisterDecoder(BDecoderPlugin* plugin, const entry_ref& ref)
{
	BAutolock locker(fLock);

	decoder_info* pinfo;
	for (fDecoderList.Rewind(); fDecoderList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this decoder
			return;
		}
	}

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
AddOnManager::_RegisterWriter(BWriterPlugin* writer, const entry_ref& ref)
{
	BAutolock locker(fLock);

	writer_info* pinfo;
	for (fWriterList.Rewind(); fWriterList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this writer
			return;
		}
	}

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
AddOnManager::_RegisterEncoder(BEncoderPlugin* plugin, const entry_ref& ref)
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

	// Get list of supported encoders...

	encoder_info info;
	info.ref = ref;
	info.internalID = fNextEncoderCodecInfoID++;

	int32 cookie = 0;

	while (true) {
		memset(&info.codecInfo, 0, sizeof(media_codec_info));
		info.intputFormat.Clear();
		info.outputFormat.Clear();
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


void
AddOnManager::_RegisterStreamer(BStreamerPlugin* streamer, const entry_ref& ref)
{
	BAutolock locker(fLock);

	streamer_info* pInfo;
	for (fStreamerList.Rewind(); fStreamerList.GetNext(&pInfo);) {
		if (!strcmp(pInfo->ref.name, ref.name)) {
			// We already know this streamer
			return;
		}
	}

	streamer_info info;
	info.ref = ref;
	fStreamerList.Insert(info);
}


bool
AddOnManager::_FindDecoder(const media_format& format, const BPath& path,
	entry_ref* _decoderRef)
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

			*_decoderRef = info->ref;
			return true;
		}
	}
	return false;
}


bool
AddOnManager::_FindEncoder(const media_format& format, const BPath& path,
	entry_ref* _encoderRef)
{
	node_ref nref;
	BDirectory directory;
	if (directory.SetTo(path.Path()) != B_OK
		|| directory.GetNodeRef(&nref) != B_OK) {
		return false;
	}

	encoder_info* info;
	for (fEncoderList.Rewind(); fEncoderList.GetNext(&info);) {
		if (info->ref.directory != nref.node)
			continue;

		// check if the encoder matches the supplied format
		if (info->outputFormat.Matches(&format)) {
			*_encoderRef = info->ref;
			return true;
		}
		continue;
	}
	return false;
}


void
AddOnManager::_GetReaders(const BPath& path, entry_ref* outRefs,
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


} // namespace BPrivate
} // namespace BCodecKit
