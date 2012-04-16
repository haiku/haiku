/*****************************************************************************/
// BaseTranslator
// Written by Michael Wilber, Haiku Translation Kit Team
//
// BaseTranslator.cpp
//
// The BaseTranslator class implements functionality common to most
// Translators so that this functionality need not be implemented over and
// over in each Translator.
//
//
// Copyright (c) 2004 Haiku, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <string.h>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "BaseTranslator.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BaseTranslator"


// ---------------------------------------------------------------
// Constructor
//
// Sets up the version info and the name of the translator so that
// these values can be returned when they are requested.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BaseTranslator::BaseTranslator(const char *name, const char *info,
	const int32 version, const translation_format *inFormats,
	int32 inCount, const translation_format *outFormats, int32 outCount,
	const char *settingsFile, const TranSetting *defaults, int32 defCount,
	uint32 tranGroup, uint32 tranType)
	:
	BTranslator()
{
	fSettings = new TranslatorSettings(settingsFile, defaults, defCount);
	fSettings->LoadSettings();
		// load settings from the Base Translator settings file

	fVersion = version;
	fName = new char[strlen(name) + 1];
	strcpy(fName, name);
	fInfo = new char[strlen(info) + 41];
	sprintf(fInfo, "%s v%d.%d.%d %s", info,
		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(fVersion)),
		static_cast<int>(B_TRANSLATION_MINOR_VERSION(fVersion)),
		static_cast<int>(B_TRANSLATION_REVISION_VERSION(fVersion)),
		__DATE__);

	fInputFormats = inFormats;
	fInputCount = (fInputFormats) ? inCount : 0;
	fOutputFormats = outFormats;
	fOutputCount = (fOutputFormats) ? outCount : 0;
	fTranGroup = tranGroup;
	fTranType = tranType;
}


// ---------------------------------------------------------------
// Destructor
//
// Does nothing
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
//
// NOTE: It may be the case, that under Be's libtranslation.so,
// that this destructor will never be called
BaseTranslator::~BaseTranslator()
{
	fSettings->Release();
	delete[] fName;
	delete[] fInfo;
}


// ---------------------------------------------------------------
// TranslatorName
//
// Returns the short name of the translator.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: a const char * to the short name of the translator
// ---------------------------------------------------------------
const char *
BaseTranslator::TranslatorName() const
{
	return fName;
}


// ---------------------------------------------------------------
// TranslatorInfo
//
// Returns a more verbose name for the translator than the one
// TranslatorName() returns. This usually includes version info.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: a const char * to the verbose name of the translator
// ---------------------------------------------------------------
const char *
BaseTranslator::TranslatorInfo() const
{
	return fInfo;
}


// ---------------------------------------------------------------
// TranslatorVersion
//
// Returns the integer representation of the current version of
// this translator.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
int32
BaseTranslator::TranslatorVersion() const
{
	return fVersion;
}


// ---------------------------------------------------------------
// InputFormats
//
// Returns a list of input formats supported by this translator.
//
// Preconditions:
//
// Parameters:	out_count,	The number of input formats
//							support is returned here.
//
// Postconditions:
//
// Returns: the array of input formats and the number of input
// formats through the out_count parameter
// ---------------------------------------------------------------
const translation_format *
BaseTranslator::InputFormats(int32 *out_count) const
{
	if (out_count) {
		*out_count = fInputCount;
		return fInputFormats;
	} else
		return NULL;
}


// ---------------------------------------------------------------
// OutputFormats
//
// Returns a list of output formats supported by this translator.
//
// Preconditions:
//
// Parameters:	out_count,	The number of output formats
//							support is returned here.
//
// Postconditions:
//
// Returns: the array of output formats and the number of output
// formats through the out_count parameter
// ---------------------------------------------------------------
const translation_format *
BaseTranslator::OutputFormats(int32 *out_count) const
{
	if (out_count) {
		*out_count = fOutputCount;
		return fOutputFormats;
	} else
		return NULL;
}


// ---------------------------------------------------------------
// identify_bits_header
//
// Determines if the data in inSource is in the
// B_TRANSLATOR_BITMAP ('bits') format. If it is, it returns
// info about the data in inSource to outInfo and pheader.
//
// Preconditions:
//
// Parameters:	inSource,	The source of the image data
//
//				outInfo,	Information about the translator
//							is copied here
//
//				amtread,	Amount of data read from inSource
//							before this function was called
//
//				read,		Pointer to the data that was read
// 							in before this function was called
//
//				pheader,	The bits header is copied here after
//							it is read in from inSource
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if the data does not look like
//								bits format data
//
// B_ERROR,	if the header data could not be converted to host
//			format
//
// B_OK,	if the data looks like bits data and no errors were
//			encountered
// ---------------------------------------------------------------
status_t
BaseTranslator::identify_bits_header(BPositionIO *inSource,
	translator_info *outInfo, TranslatorBitmap *pheader)
{
	TranslatorBitmap header;

	// read in the header
	ssize_t size = sizeof(TranslatorBitmap);
	if (inSource->Read(
		(reinterpret_cast<uint8 *> (&header)), size) != size)
		return B_NO_TRANSLATOR;

	// convert to host byte order
	if (swap_data(B_UINT32_TYPE, &header, sizeof(TranslatorBitmap),
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;

	// check if header values are reasonable
	if (header.colors != B_RGB32 &&
		header.colors != B_RGB32_BIG &&
		header.colors != B_RGBA32 &&
		header.colors != B_RGBA32_BIG &&
		header.colors != B_RGB24 &&
		header.colors != B_RGB24_BIG &&
		header.colors != B_RGB16 &&
		header.colors != B_RGB16_BIG &&
		header.colors != B_RGB15 &&
		header.colors != B_RGB15_BIG &&
		header.colors != B_RGBA15 &&
		header.colors != B_RGBA15_BIG &&
		header.colors != B_CMAP8 &&
		header.colors != B_GRAY8 &&
		header.colors != B_GRAY1 &&
		header.colors != B_CMYK32 &&
		header.colors != B_CMY32 &&
		header.colors != B_CMYA32 &&
		header.colors != B_CMY24)
		return B_NO_TRANSLATOR;
	if (header.rowBytes * (header.bounds.Height() + 1) != header.dataSize)
		return B_NO_TRANSLATOR;

	if (outInfo) {
		outInfo->type = B_TRANSLATOR_BITMAP;
		outInfo->group = B_TRANSLATOR_BITMAP;
		outInfo->quality = 0.2;
		outInfo->capability = 0.2;
		strlcpy(outInfo->name, B_TRANSLATE("Be Bitmap Format"),
			sizeof(outInfo->name));
		strcpy(outInfo->MIME, "image/x-be-bitmap");

		// Look for quality / capability info in fInputFormats
		for (int32 i = 0; i < fInputCount; i++) {
			if (fInputFormats[i].type == B_TRANSLATOR_BITMAP &&
				fInputFormats[i].group == B_TRANSLATOR_BITMAP) {
				outInfo->quality = fInputFormats[i].quality;
				outInfo->capability = fInputFormats[i].capability;
				strcpy(outInfo->name, fInputFormats[i].name);
				break;
			}
		}
	}

	if (pheader) {
		pheader->magic = header.magic;
		pheader->bounds = header.bounds;
		pheader->rowBytes = header.rowBytes;
		pheader->colors = header.colors;
		pheader->dataSize = header.dataSize;
	}

	return B_OK;
}


// ---------------------------------------------------------------
// BitsCheck
//
// Examines the input stream for B_TRANSLATOR_BITMAP format
// information and determines if BaseTranslator can handle
// the translation entirely, if it must pass the task of
// translation to the derived translator or if the stream cannot
// be decoded by the BaseTranslator or the derived translator.
//
// Preconditions:
//
// Parameters:	inSource,	where the data to examine is
//
//				ioExtension,	configuration settings for the
//								translator
//
//				outType,	The format that the user wants
//							the data in inSource to be
//							converted to. NOTE: This is passed by
//							reference so that it can modify the
//							outType that is seen by the
//							BaseTranslator and the derived
//							translator
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if this translator can't handle
//								the data in inSource
//
// B_ERROR,	if there was an error converting the data to the host
//			format
//
// B_BAD_VALUE, if the settings in ioExtension are bad
//
// B_OK,	if this translator understand the data and there were
//			no errors found
// ---------------------------------------------------------------
status_t
BaseTranslator::BitsCheck(BPositionIO *inSource, BMessage *ioExtension,
	uint32 &outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != fTranType)
		return B_NO_TRANSLATOR;

	// Convert the magic numbers to the various byte orders so that
	// I won't have to convert the data read in to see whether or not
	// it is a supported type
	const uint32 kBitsMagic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);

	// Read in the magic number and determine if it
	// is a supported type
	uint8 ch[4];
	if (inSource->Read(ch, 4) != 4)
		return B_NO_TRANSLATOR;
	inSource->Seek(-4, SEEK_CUR);
		// seek backward becuase functions used after this one
		// expect the stream to be at the beginning

	// Read settings from ioExtension
	if (ioExtension && fSettings->LoadSettings(ioExtension) < B_OK)
		return B_BAD_VALUE;

	uint32 sourceMagic;
	memcpy(&sourceMagic, ch, sizeof(uint32));
	if (sourceMagic == kBitsMagic)
		return B_OK;
	return B_OK + 1;
}


status_t
BaseTranslator::BitsIdentify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	status_t result = BitsCheck(inSource, ioExtension, outType);
	if (result == B_OK) {
		TranslatorBitmap bitmap;
		result = identify_bits_header(inSource, outInfo, &bitmap);
		if (result == B_OK)
			result = DerivedCanHandleImageSize(bitmap.bounds.Width() + 1.0,
				bitmap.bounds.Height() + 1.0);
	} else if (result >= B_OK) {
		// if NOT B_TRANSLATOR_BITMAP, it could be an image in the
		// derived format
		result = DerivedIdentify(inSource, inFormat, ioExtension,
			outInfo, outType);
	}
	return result;
}


// ---------------------------------------------------------------
// Identify
//
// Examines the data from inSource and determines if it is in a
// format that this translator knows how to work with.
//
// Preconditions:
//
// Parameters:	inSource,	where the data to examine is
//
//				inFormat,	a hint about the data in inSource,
//							it is ignored since it is only a hint
//
//				ioExtension,	configuration settings for the
//								translator
//
//				outInfo,	information about what data is in
//							inSource and how well this translator
//							can handle that data is stored here
//
//				outType,	The format that the user wants
//							the data in inSource to be
//							converted to
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if this translator can't handle
//								the data in inSource
//
// B_ERROR,	if there was an error converting the data to the host
//			format
//
// B_BAD_VALUE, if the settings in ioExtension are bad
//
// B_OK,	if this translator understand the data and there were
//			no errors found
// ---------------------------------------------------------------
status_t
BaseTranslator::Identify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	switch (fTranGroup) {
		case B_TRANSLATOR_BITMAP:
			return BitsIdentify(inSource, inFormat, ioExtension,
				outInfo, outType);

		default:
			return DerivedIdentify(inSource, inFormat, ioExtension,
				outInfo, outType);
	}
}


// ---------------------------------------------------------------
// translate_from_bits_to_bits
//
// Convert the data in inSource from the Be Bitmap format ('bits')
// to the format specified in outType (either bits or Base).
//
// Preconditions:
//
// Parameters:	inSource,	the bits data to translate
//
// 				amtread,	the amount of data already read from
//							inSource
//
//				read,		pointer to the data already read from
//							inSource
//
//				outType,	the type of data to convert to
//
//				outDestination,	where the output is written to
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if the data is not in a supported
//								format
//
// B_ERROR, if there was an error allocating memory or some other
//			error
//
// B_OK, if successfully translated the data from the bits format
// ---------------------------------------------------------------
status_t
BaseTranslator::translate_from_bits_to_bits(BPositionIO *inSource,
	uint32 outType,	BPositionIO *outDestination)
{
	TranslatorBitmap bitsHeader;
	bool bheaderonly = false, bdataonly = false;

	status_t result;
	result = identify_bits_header(inSource, NULL, &bitsHeader);
	if (result != B_OK)
		return result;

	// Translate B_TRANSLATOR_BITMAP to B_TRANSLATOR_BITMAP, easy enough :)
	if (outType == B_TRANSLATOR_BITMAP) {
		// write out bitsHeader (only if configured to)
		if (bheaderonly || (!bheaderonly && !bdataonly)) {
			if (swap_data(B_UINT32_TYPE, &bitsHeader,
				sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK)
				return B_ERROR;
			if (outDestination->Write(&bitsHeader,
				sizeof(TranslatorBitmap)) != sizeof(TranslatorBitmap))
				return B_ERROR;
		}

		// write out the data (only if configured to)
		if (bdataonly || (!bheaderonly && !bdataonly)) {
			uint8 buf[1024];
			uint32 remaining = B_BENDIAN_TO_HOST_INT32(bitsHeader.dataSize);
			ssize_t rd, writ;
			rd = inSource->Read(buf, 1024);
			while (rd > 0) {
				writ = outDestination->Write(buf, rd);
				if (writ < 0)
					break;
				remaining -= static_cast<uint32>(writ);
				rd = inSource->Read(buf, min(1024, remaining));
			}

			if (remaining > 0)
				return B_ERROR;
			else
				return B_OK;
		} else
			return B_OK;

	} else
		return B_NO_TRANSLATOR;
}


status_t
BaseTranslator::BitsTranslate(BPositionIO *inSource,
	const translator_info *inInfo, BMessage *ioExtension, uint32 outType,
	BPositionIO *outDestination)
{
	status_t result = BitsCheck(inSource, ioExtension, outType);
	if (result == B_OK && outType == B_TRANSLATOR_BITMAP) {
		result = translate_from_bits_to_bits(inSource, outType,
			outDestination);
	} else if (result >= B_OK) {
		// If NOT B_TRANSLATOR_BITMAP type it could be the derived format
		result = DerivedTranslate(inSource, inInfo, ioExtension, outType,
			outDestination, (result == B_OK));
	}
	return result;
}


// ---------------------------------------------------------------
// Translate
//
// Translates the data in inSource to the type outType and stores
// the translated data in outDestination.
//
// Preconditions:
//
// Parameters:	inSource,	the data to be translated
//
//				inInfo,	hint about the data in inSource (not used)
//
//				ioExtension,	configuration options for the
//								translator
//
//				outType,	the type to convert inSource to
//
//				outDestination,	where the translated data is
//								put
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if the options in ioExtension are bad
//
// B_NO_TRANSLATOR, if this translator doesn't understand the data
//
// B_ERROR, if there was an error allocating memory or converting
//          data
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
BaseTranslator::Translate(BPositionIO *inSource,
	const translator_info *inInfo, BMessage *ioExtension, uint32 outType,
	BPositionIO *outDestination)
{
	switch (fTranGroup) {
		case B_TRANSLATOR_BITMAP:
			return BitsTranslate(inSource, inInfo, ioExtension, outType,
				outDestination);

		default:
			return DerivedTranslate(inSource, inInfo, ioExtension, outType,
				outDestination, -1);
	}
}


// returns the current translator settings into ioExtension
status_t
BaseTranslator::GetConfigurationMessage(BMessage *ioExtension)
{
	return fSettings->GetConfigurationMessage(ioExtension);
}


// ---------------------------------------------------------------
// MakeConfigurationView
//
// Makes a BView object for configuring / displaying info about
// this translator.
//
// Preconditions:
//
// Parameters:	ioExtension,	configuration options for the
//								translator
//
//				outView,		the view to configure the
//								translator is stored here
//
//				outExtent,		the bounds of the view are
//								stored here
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
status_t
BaseTranslator::MakeConfigurationView(BMessage *ioExtension, BView **outView,
	BRect *outExtent)
{
	if (!outView || !outExtent)
		return B_BAD_VALUE;
	if (ioExtension && fSettings->LoadSettings(ioExtension) != B_OK)
		return B_BAD_VALUE;

	BView *view = NewConfigView(AcquireSettings());
		// implemented in derived class

	if (view) {
		*outView = view;
		if ((view->Flags() & B_SUPPORTS_LAYOUT) != 0)
			view->ResizeTo(view->ExplicitPreferredSize());

		*outExtent = view->Bounds();

		return B_OK;
	} else
		return BTranslator::MakeConfigurationView(ioExtension, outView,
			outExtent);
}


TranslatorSettings *
BaseTranslator::AcquireSettings()
{
	return fSettings->Acquire();
}


///////////////////////////////////////////////////////////
// Functions to be implemented by derived classes

status_t
BaseTranslator::DerivedIdentify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	return B_NO_TRANSLATOR;
}


status_t
BaseTranslator::DerivedTranslate(BPositionIO *inSource,
	const translator_info *inInfo, BMessage *ioExtension, uint32 outType,
	BPositionIO *outDestination, int32 baseType)
{
	return B_NO_TRANSLATOR;
}


status_t
BaseTranslator::DerivedCanHandleImageSize(float width, float height) const
{
	return B_OK;
}


BView *
BaseTranslator::NewConfigView(TranslatorSettings *settings)
{
	return NULL;
}


void
translate_direct_copy(BPositionIO *inSource, BPositionIO *outDestination)
{
	const size_t kbufsize = 2048;
	uint8 buffer[kbufsize];
	ssize_t ret = inSource->Read(buffer, kbufsize);
	while (ret > 0) {
		outDestination->Write(buffer, ret);
		ret = inSource->Read(buffer, kbufsize);
	}
}
