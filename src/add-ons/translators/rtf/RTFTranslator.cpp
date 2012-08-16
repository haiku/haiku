/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "RTFTranslator.h"
#include "ConfigView.h"
#include "RTF.h"
#include "convert.h"

#include <Catalog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RTFTranslator"

#define READ_BUFFER_SIZE 2048
#define DATA_BUFFER_SIZE 64


// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		RTF_TEXT_FORMAT,
		B_TRANSLATOR_TEXT,
		RTF_IN_QUALITY,
		RTF_IN_CAPABILITY,
		"text/rtf",
		"RichTextFormat file"
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
	if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT)
		return B_NO_TRANSLATOR;

	RTF::Parser parser(*stream);

	status_t status = parser.Identify();
	if (status != B_OK)
		return B_NO_TRANSLATOR;

	// return information about the data in the stream
	info->type = B_TRANSLATOR_TEXT; //RTF_TEXT_FORMAT;
	info->group = B_TRANSLATOR_TEXT;
	info->quality = RTF_IN_QUALITY;
	info->capability = RTF_IN_CAPABILITY;
	strlcpy(info->name, B_TRANSLATE("RichTextFormat file"),
		sizeof(info->name));
	strcpy(info->MIME, "text/rtf");

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
	if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT)
		return B_NO_TRANSLATOR;

	RTF::Parser parser(*source);

	RTF::Header header;
	status_t status = parser.Parse(header);
	if (status != B_OK)
		return status;

	// we support two different output formats

	if (outType == B_TRANSLATOR_TEXT)
		return convert_to_plain_text(header, *target);

	return convert_to_stxt(header, *target);
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

