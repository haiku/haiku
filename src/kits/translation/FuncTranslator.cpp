/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!
	This file contains the BTranslator based object for
	function based translators, aka, the translators
	that don't use the make_nth_translator() mechanism.

	This class is used by the BTranslatorRoster class
	so that function based translators, make_nth_translator()
	translators and private BTranslator objects could be
	accessed in the same way.
*/


#include "FuncTranslator.h"

#include <string.h>


namespace BPrivate {

BFuncTranslator::BFuncTranslator(const translator_data& data)
{
	fData = data;
}


BFuncTranslator::~BFuncTranslator()
{
}


const char *
BFuncTranslator::TranslatorName() const
{
	return fData.name;
}


const char *
BFuncTranslator::TranslatorInfo() const
{
	return fData.info;
}


int32
BFuncTranslator::TranslatorVersion() const
{
	return fData.version;
}


const translation_format *
BFuncTranslator::InputFormats(int32* _count) const
{
	if (_count == NULL || fData.input_formats == NULL)
		return NULL;

	int32 count = 0;
	while (fData.input_formats[count].type) {
		count++;
	}

	*_count = count;
	return fData.input_formats;
}


const translation_format *
BFuncTranslator::OutputFormats(int32* _count) const
{
	if (_count == NULL || fData.output_formats == NULL)
		return NULL;

	int32 count = 0;
	while (fData.output_formats[count].type) {
		count++;
	}

	*_count = count;
	return fData.output_formats;
}


status_t
BFuncTranslator::Identify(BPositionIO* source, const translation_format* format,
	BMessage* ioExtension, translator_info* info, uint32 type)
{
	if (fData.identify_hook == NULL)
		return B_ERROR;

	return fData.identify_hook(source, format, ioExtension, info, type);
}


status_t
BFuncTranslator::Translate(BPositionIO* source, const translator_info *info,
	BMessage* ioExtension, uint32 type, BPositionIO* destination)
{
	if (fData.translate_hook == NULL)
		return B_ERROR;

	return fData.translate_hook(source, info, ioExtension, type, destination);
}


status_t
BFuncTranslator::MakeConfigurationView(BMessage* ioExtension,
	BView** _view, BRect* _extent)
{
	if (fData.make_config_hook == NULL)
		return B_ERROR;

	return fData.make_config_hook(ioExtension, _view, _extent);
}


status_t
BFuncTranslator::GetConfigurationMessage(BMessage* ioExtension)
{
	if (fData.get_config_message_hook == NULL)
		return B_ERROR;

	return fData.get_config_message_hook(ioExtension);
}

}	// namespace BPrivate
