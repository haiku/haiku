/*****************************************************************************/
//               File: R4xTranslator.cpp
//              Class: BR4xTranslator
//             Author: Michael Wilber, Translation Kit Team
// Originally Created: 2002-06-11
//
// Description: This class is a BTranslator based object for
//              BeOS R4.0 and R4.5 type translators, aka, the translators
//              that don't use the make_nth_translator() mechanism.
//
//              This class is used by the OpenBeOS BTranslatorRoster
//              so that R4x translators, post R4.5 translators and private
//              BTranslator objects could be accessed in the same way. 
//
//
// Copyright (c) 2002 OpenBeOS Project
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

#include <R4xTranslator.h>

// ---------------------------------------------------------------
// Constructor
//
// Initializes class data.
//
// Preconditions:
//
// Parameters: kpData, data and function pointers that do all of
//                     the useful work for this class
//
// Postconditions: If kpData is freed before the BR4xTranslator,
//                 the BR4xTranslator will fail to work and 
//                 could the computer to crash. kpData may point
//                 to a translator add-on image, this image
//                 should not be unloaded before the 
//                 BR4xTranslator is freed.
//                 
//
// Returns:
// ---------------------------------------------------------------
BR4xTranslator::BR4xTranslator(const translator_data *kpData) : BTranslator()
{
	fpData = new translator_data;
	
	fpData->translatorName = kpData->translatorName;
	fpData->translatorInfo = kpData->translatorInfo;
	fpData->translatorVersion = kpData->translatorVersion;
	fpData->inputFormats = kpData->inputFormats;
	fpData->outputFormats = kpData->outputFormats;
	
	fpData->Identify = kpData->Identify;
	fpData->Translate = kpData->Translate;
	fpData->MakeConfig = kpData->MakeConfig;
	fpData->GetConfigMessage = kpData->GetConfigMessage;
}

// ---------------------------------------------------------------
// Destructor
//
// Frees memory used by this object. Note that none of the members
// of the struct have delete used on them, this is because most
// are const pointers and the one that isn't is a regular int32.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BR4xTranslator::~BR4xTranslator()
{
	delete fpData;
}

// ---------------------------------------------------------------
// TranslatorName
//
// Returns a short name for the translator that this object stores
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
const char *BR4xTranslator::TranslatorName() const
{
	return fpData->translatorName;
}

// ---------------------------------------------------------------
// TranslatorInfo
//
// Returns a verbose name/description for the translator that this
// object stores
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
const char *BR4xTranslator::TranslatorInfo() const
{
	return fpData->translatorInfo;
}

// ---------------------------------------------------------------
// TranslatorVersion
//
// Returns the version number for this translator
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
int32 BR4xTranslator::TranslatorVersion() const
{
	return fpData->translatorVersion;
}

// ---------------------------------------------------------------
// InputFormats
//
// Returns the list of supported input formats and the count
// of supported input formats
//
// Preconditions:
//
// Parameters: out_count, the number of input formats is stored
//                        here after the function completes
//
// Postconditions:
//
// Returns: the list of supported input formats
// ---------------------------------------------------------------
const translation_format *BR4xTranslator::InputFormats(int32 *out_count) const
{
	int32 i;
	for (i = 0; fpData->inputFormats[i].type; i++);
	
	*out_count = i;
	return fpData->inputFormats;
}

// ---------------------------------------------------------------
// OutputFormats
//
// Returns the list of supported output formats
//
// Preconditions:
//
// Parameters: out_count, the number of output formats is stored
//                        here after the function completes
//
// Postconditions:
//
// Returns: the list of supported output formats
// ---------------------------------------------------------------
const translation_format *BR4xTranslator::OutputFormats(int32 *out_count) const
{
	int32 i;
	for (i = 0; fpData->outputFormats[i].type; i++);
	
	*out_count = i;
	return fpData->outputFormats;
}

// ---------------------------------------------------------------
// Identify
//
// If the translator understands how to convert the data contained
// in inSource to media type outType, it fills outInfo with 
// details about the input format and return B_OK. If it doesn't
// know how to translate the data, it returns B_NO_TRANSLATOR.
//
// The actual work for this function is done by the translator
// add-on at the other end of the Identify function pointer.
// So, there's no telling the actual behavior of this function
// it depends on the translator add-on.
//
// Preconditions:
//
// Parameters: inSource, the data that wants to be converted
//             inFormat, (can be null) hint about the data in
//                       inSource
//             ioExtension, (can be null) contains additional
//                          information for the translator
//                          add-on
//             outInfo, information about the capabilities
//                      of this translator
//             outType, the output type
//             
//
// Postconditions:
//
// Returns: B_OK if this translator can handle the data,
//          B_NO_TRANSLATOR if it can't
// ---------------------------------------------------------------
status_t BR4xTranslator::Identify(BPositionIO *inSource, 
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	return fpData->Identify(inSource, inFormat, ioExtension, outInfo, outType);
}

// ---------------------------------------------------------------
// Translate
//
// The translator translates data from inSource to format outType,
// writing the output to outDestination.
//
// The actual work for this function is done by the translator
// add-on at the other end of the Translate function pointer.
// So, there's no telling the actual behavior of this function
// it depends on the translator add-on.
//
// Preconditions:
//
// Parameters: inSource, data to be translated
//             inInfo, hint about the data in inSource
//             ioExtension, contains configuration information
//             outType, the format to translate the data to
//             outDestination, where the source is translated to
//
// Postconditions:
//
// Returns: B_OK, if it converted the data 
//          B_NO_TRANSLATOR, if it couldn't
//          some other value, if it feels like it
// ---------------------------------------------------------------
status_t BR4xTranslator::Translate(BPositionIO *inSource, 
	const translator_info *inInfo, BMessage *ioExtension, uint32 outType, 
	BPositionIO *outDestination)
{
	return fpData->Translate(inSource, inInfo, ioExtension, outType,
		outDestination);
}

// ---------------------------------------------------------------
// MakeConfigurationView
//
// This creates a BView object that allows the user to configure
// the options for the translator. Not all translators support
// this feature, and for those that don't, this function
// returns B_NO_TRANSLATOR.
//
// Preconditions:
//
// Parameters: ioExtension, the settings for the translator
//             outView, the view created by the translator
//             outExtent, the bounds of the view
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR, if this function is not support
//          anything else, whatever the translator feels like 
//                         returning
// ---------------------------------------------------------------
status_t BR4xTranslator::MakeConfigurationView(BMessage *ioExtension,
	BView **outView, BRect *outExtent)
{
	if (fpData->MakeConfig)
		return fpData->MakeConfig(ioExtension, outView, outExtent);
	else
		return B_ERROR;
}

// ---------------------------------------------------------------
// GetConfigurationMessage
//
// This function stores the current configuration for the
// translator into ioExtension. Not all translators
// support this function.
//
// Preconditions:
//
// Parameters: ioExtension, where the configuration is stored
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR, if function is not support
//          something else, if it is
// ---------------------------------------------------------------
status_t BR4xTranslator::GetConfigurationMessage(BMessage *ioExtension)
{
	if (fpData->GetConfigMessage)
		return fpData->GetConfigMessage(ioExtension);
	else
		return B_ERROR;	
}

