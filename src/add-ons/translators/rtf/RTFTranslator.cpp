/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "RTFTranslator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Catalog.h>

#include "ConfigView.h"
#include "convert.h"
#include "RTF.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RTFTranslator"

#define READ_BUFFER_SIZE 2048
#define DATA_BUFFER_SIZE 64


#define TEXT_IN_QUALITY 0.4
#define TEXT_IN_CAPABILITY 0.6

#define STXT_IN_QUALITY 0.5
#define STXT_IN_CAPABILITY 0.5

#define RTF_OUT_QUALITY RTF_IN_QUALITY
#define RTF_OUT_CAPABILITY RTF_IN_CAPABILITY

// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		RTF_TEXT_FORMAT,
		B_TRANSLATOR_TEXT,
		RTF_IN_QUALITY,
		RTF_IN_CAPABILITY,
		"text/rtf",
		"RichTextFormat file"
	},
	{
		B_TRANSLATOR_TEXT,
		B_TRANSLATOR_TEXT,
		TEXT_IN_QUALITY,
		TEXT_IN_CAPABILITY,
		"text/plain",
		"Plain text file"
	},
	{
		B_STYLED_TEXT_FORMAT,
		B_TRANSLATOR_TEXT,
		STXT_IN_QUALITY,
		STXT_IN_CAPABILITY,
		"text/x-vnd.Be-stxt",
		"Be styled text file"
	}
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_TEXT,
		B_TRANSLATOR_TEXT,
		TEXT_OUT_QUALITY,
		TEXT_OUT_CAPABILITY,
		"text/plain",
		"Plain text file"
	},
	{
		B_STYLED_TEXT_FORMAT,
		B_TRANSLATOR_TEXT,
		STXT_OUT_QUALITY,
		STXT_OUT_CAPABILITY,
		"text/x-vnd.Be-stxt",
		"Be styled text file"
	},
	{
		RTF_TEXT_FORMAT,
		B_TRANSLATOR_TEXT,
		RTF_OUT_QUALITY,
		RTF_OUT_CAPABILITY,
		"text/rtf",
		"RichTextFormat file"
	}
};


RTFTranslator::RTFTranslator()
{
	char info[256];
	sprintf(info, B_TRANSLATE("Rich Text Format translator v%d.%d.%d %s"),
		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(RTF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VERSION(RTF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVISION_VERSION(
			RTF_TRANSLATOR_VERSION)),
		__DATE__);

	fInfo = strdup(info);
}


RTFTranslator::~RTFTranslator()
{
	free(fInfo);
}


const char *
RTFTranslator::TranslatorName() const
{
	return B_TRANSLATE("RTF text files");
}


const char *
RTFTranslator::TranslatorInfo() const
{
	return B_TRANSLATE("Rich Text Format translator");
}


int32
RTFTranslator::TranslatorVersion() const
{
	return RTF_TRANSLATOR_VERSION;
}


const translation_format *
RTFTranslator::InputFormats(int32 *_outCount) const
{
	if (_outCount == NULL)
		return NULL;

	*_outCount = sizeof(sInputFormats) / sizeof(translation_format);
	return sInputFormats;
}


const translation_format *
RTFTranslator::OutputFormats(int32 *_outCount) const
{
	*_outCount = sizeof(sOutputFormats) / sizeof(translation_format);
	return sOutputFormats;
}


status_t
RTFTranslator::Identify(BPositionIO *stream,
	const translation_format *format, BMessage *ioExtension,
	translator_info *info, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_TEXT;
	else if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT
		&& outType != RTF_TEXT_FORMAT)
		return B_NO_TRANSLATOR;


	RTF::Parser parser(*stream);
	status_t status = parser.Identify();

	if (status == B_OK) {
		// Source data is RTF. We can translate to RTF (no-op), plaintext, or
		// styled text.

		// return information about the data in the stream
		info->type = B_TRANSLATOR_TEXT; //RTF_TEXT_FORMAT;
		info->group = B_TRANSLATOR_TEXT;
		info->quality = RTF_IN_QUALITY;
		info->capability = RTF_IN_CAPABILITY;
		strlcpy(info->name, B_TRANSLATE("RichTextFormat file"),
			sizeof(info->name));
		strcpy(info->MIME, "text/rtf");
	} else {
		// Not an RTF file. We can only work with it if we are translating to
		// RTF.
		if (outType != RTF_TEXT_FORMAT)
			return B_NO_TRANSLATOR;

		stream->Seek(0, SEEK_SET);
		TranslatorStyledTextStreamHeader header;
		stream->Read(&header, sizeof(header));
		swap_data(B_UINT32_TYPE, &header, sizeof(header),
			B_SWAP_BENDIAN_TO_HOST);
		stream->Seek(0, SEEK_SET);
		if (header.header.magic == B_STYLED_TEXT_FORMAT
			&& header.header.header_size == (int32)sizeof(header)
			&& header.header.data_size == 0
			&& header.version == 100) {
			info->type = B_STYLED_TEXT_FORMAT;
			info->group = B_TRANSLATOR_TEXT;
			info->quality = STXT_IN_QUALITY;
			info->capability = STXT_IN_CAPABILITY;
			strlcpy(info->name, B_TRANSLATE("Be style text file"),
				sizeof(info->name));
			strcpy(info->MIME, "text/x-vnd.Be-stxt");
		} else {
			info->type = B_TRANSLATOR_TEXT;
			info->group = B_TRANSLATOR_TEXT;
			info->quality = TEXT_IN_QUALITY;
			info->capability = TEXT_IN_CAPABILITY;
			strlcpy(info->name, B_TRANSLATE("Plain text file"),
				sizeof(info->name));
			strcpy(info->MIME, "text/plain");
		}
	}
	return B_OK;
}


status_t
RTFTranslator::Translate(BPositionIO *source,
	const translator_info *inInfo, BMessage *ioExtension,
	uint32 outType, BPositionIO *target)
{
	if (target == NULL || source == NULL)
		return B_BAD_VALUE;

	if (!outType)
		outType = B_TRANSLATOR_TEXT;
	if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT
		&& outType != RTF_TEXT_FORMAT)
		return B_NO_TRANSLATOR;

	if (strncmp(inInfo->MIME, "text/rtf", 8) == 0) {
		RTF::Parser parser(*source);

		RTF::Header header;
		status_t status = parser.Parse(header);
		if (status != B_OK)
			return status;

		if (outType == B_TRANSLATOR_TEXT)
			return convert_to_plain_text(header, *target);
		else
			return convert_to_stxt(header, *target);

	} else if (inInfo->type == B_TRANSLATOR_TEXT) {
		return convert_plain_text_to_rtf(*source, *target);
	} else if (inInfo->type == B_STYLED_TEXT_FORMAT) {
		return convert_styled_text_to_rtf(source, target);
	} else
		return B_BAD_VALUE;

}


status_t
RTFTranslator::MakeConfigurationView(BMessage *ioExtension, BView **_view,
	BRect *_extent)
{
	if (_view == NULL || _extent == NULL)
		return B_BAD_VALUE;

	BView *view = new ConfigView(BRect(0, 0, 225, 175));
	if (view == NULL)
		return BTranslator::MakeConfigurationView(ioExtension, _view, _extent);

	*_view = view;
	*_extent = view->Bounds();
	return B_OK;
}


//	#pragma mark -


BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new RTFTranslator();
}

