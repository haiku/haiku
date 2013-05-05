/*
 * Copyright 2007-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "RAWTranslator.h"
#include "ConfigView.h"
#include "RAW.h"

#include <Catalog.h>
#include <BufferIO.h>
#include <Messenger.h>
#include <TranslatorRoster.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RAWTranslator"


class FreeAllocation {
	public:
		FreeAllocation(void* buffer)
			:
			fBuffer(buffer)
		{
		}

		~FreeAllocation()
		{
			free(fBuffer);
		}

	private:
		void*	fBuffer;
};

struct progress_data {
	BMessenger	monitor;
	BMessage	message;
};

// Extensions that ShowImage supports
const char* kDocumentCount = "/documentCount";
const char* kDocumentIndex = "/documentIndex";
const char* kProgressMonitor = "/progressMonitor";
const char* kProgressMessage = "/progressMessage";


// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		RAW_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		RAW_IN_QUALITY,
		RAW_IN_CAPABILITY,
		"image/x-vnd.adobe-dng",
		"Adobe Digital Negative"
	},
	{
		RAW_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		RAW_IN_QUALITY,
		RAW_IN_CAPABILITY,
		"image/x-vnd.photo-raw",
		"Digital Photo RAW image"
	}
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (RAWTranslator)"
	}
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false}
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);

const char* kShortName1 = B_TRANSLATE("RAW images");
const char* kShortInfo1 = B_TRANSLATE("RAW image translator");


//	#pragma mark -


RAWTranslator::RAWTranslator()
	: BaseTranslator(kShortName1, kShortInfo1,
		RAW_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"RAWTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, RAW_IMAGE_FORMAT)
{
}


RAWTranslator::~RAWTranslator()
{
}


status_t
RAWTranslator::DerivedIdentify(BPositionIO *stream,
	const translation_format *format, BMessage *settings,
	translator_info *info, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	BBufferIO io(stream, 128 * 1024, false);
	DCRaw raw(io);
	status_t status;

	try {
		status = raw.Identify();
	} catch (status_t error) {
		status = error;
	}

	if (status < B_OK)
		return B_NO_TRANSLATOR;

	image_meta_info meta;
	raw.GetMetaInfo(meta);

	if (settings) {
		int32 count = raw.CountImages();

		// Add page count to ioExtension
		settings->RemoveName(kDocumentCount);
		settings->AddInt32(kDocumentCount, count);

		// Check if a document index has been specified
		int32 index;
		if (settings->FindInt32(kDocumentIndex, &index) == B_OK)
			index--;
		else
			index = 0;

		if (index < 0 || index >= count)
			return B_NO_TRANSLATOR;
	}

	info->type = RAW_IMAGE_FORMAT;
	info->group = B_TRANSLATOR_BITMAP;
	info->quality = RAW_IN_QUALITY;
	info->capability = RAW_IN_CAPABILITY;
	snprintf(info->name, sizeof(info->name), 
		B_TRANSLATE_COMMENT("%s RAW image", "Parameter (%s) is the name of "
		"the manufacturer (like 'Canon')"), meta.manufacturer);
	strcpy(info->MIME, "image/x-vnd.photo-raw");

	return B_OK;
}


/*static*/ void
RAWTranslator::_ProgressMonitor(const char* message, float percentage,
	void* _data)
{
	progress_data& data = *(progress_data*)_data;

	BMessage update(data.message);
	update.AddString("message", message);
	update.AddFloat("percent", percentage);
	update.AddInt64("time", system_time());

	data.monitor.SendMessage(&update);
}


status_t
RAWTranslator::DerivedTranslate(BPositionIO* stream,
	const translator_info* info, BMessage* settings,
	uint32 outType, BPositionIO* target, int32 baseType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP || baseType != 0)
		return B_NO_TRANSLATOR;

	BBufferIO io(stream, 1024 * 1024, false);
	DCRaw raw(io);

	bool headerOnly = false;

	progress_data progressData;
	BMessenger monitor;

	if (settings != NULL) {
		settings->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &headerOnly);

		bool half;
		if (settings->FindBool("raw:half_size", &half) == B_OK && half)
			raw.SetHalfSize(true);

		if (settings->FindMessenger(kProgressMonitor,
				&progressData.monitor) == B_OK
			&& settings->FindMessage(kProgressMessage,
				&progressData.message) == B_OK) {
			raw.SetProgressMonitor(&_ProgressMonitor, &progressData);
			_ProgressMonitor("Reading Image Data", 0, &progressData);
		}
	}

	int32 imageIndex = 0;
	uint8* buffer = NULL;
	size_t bufferSize;
	status_t status;

	try {
		status = raw.Identify();

		if (status == B_OK && settings) {
			// Check if a document index has been specified
			if (settings->FindInt32(kDocumentIndex, &imageIndex) == B_OK)
				imageIndex--;
			else
				imageIndex = 0;

			if (imageIndex < 0 || imageIndex >= (int32)raw.CountImages())
				status = B_BAD_VALUE;
		}
		if (status == B_OK && !headerOnly)
			status = raw.ReadImageAt(imageIndex, buffer, bufferSize);
	} catch (status_t error) {
		status = error;
	}

	if (status < B_OK)
		return B_NO_TRANSLATOR;

	FreeAllocation _(buffer);
		// frees the buffer on destruction

	image_meta_info meta;
	raw.GetMetaInfo(meta);

	image_data_info data;
	raw.ImageAt(imageIndex, data);

	if (!data.is_raw) {
		// let others handle embedded JPEG data
		BMemoryIO io(buffer, bufferSize);
		BMessage buffer;
		if (meta.flip != 1) {
			// preserve orientation
			if (settings == NULL)
				settings = &buffer;
			settings->AddInt32("exif:orientation", meta.flip);
		}

		BTranslatorRoster* roster = BTranslatorRoster::Default();
		return roster->Translate(&io, NULL, settings, target, outType);
	}

	// retrieve EXIF data
	off_t exifOffset;
	size_t exifLength;
	bool bigEndian;
	if (settings != NULL && raw.GetEXIFTag(exifOffset, exifLength, bigEndian) == B_OK) {
		uint8* exifBuffer = (uint8*)malloc(exifLength + 16);
		if (exifBuffer != NULL) {
			// add fake TIFF header to EXIF data
			struct {
				uint16	endian;
				uint16	tiff_marker;
				uint32	offset;
				uint16	null;
			} _PACKED header;
			header.endian = bigEndian ? 'MM' : 'II';
			header.tiff_marker = 42;
			header.offset = 16;
			header.null = 0;
			memcpy(exifBuffer, &header, sizeof(header));

			if (io.ReadAt(exifOffset, exifBuffer + 16, exifLength)
					== (ssize_t)exifLength)
				settings->AddData("exif", B_RAW_TYPE, exifBuffer, exifLength + 16);

			free(exifBuffer);
		}
	}
	uint32 dataSize = data.output_width * 4 * data.output_height;

	TranslatorBitmap header;
	header.magic = B_TRANSLATOR_BITMAP;
	header.bounds.Set(0, 0, data.output_width - 1, data.output_height - 1);
	header.rowBytes = data.output_width * 4;
	header.colors = B_RGB32;
	header.dataSize = dataSize;

	// write out Be's Bitmap header
	swap_data(B_UINT32_TYPE, &header, sizeof(TranslatorBitmap),
		B_SWAP_HOST_TO_BENDIAN);
	ssize_t bytesWritten = target->Write(&header, sizeof(TranslatorBitmap));
	if (bytesWritten < B_OK)
		return bytesWritten;

	if ((size_t)bytesWritten != sizeof(TranslatorBitmap))
		return B_IO_ERROR;

	if (headerOnly)
		return B_OK;

	bytesWritten = target->Write(buffer, dataSize);
	if (bytesWritten < B_OK)
		return bytesWritten;

	if ((size_t)bytesWritten != dataSize)
		return B_IO_ERROR;

	return B_OK;
}


BView *
RAWTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new ConfigView();
}


//	#pragma mark -


BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new RAWTranslator();
}

