/*****************************************************************************/
//               File: TranslationUtils.h
//              Class: BTranslationUtils
//   Reimplimented by: Michael Wilber, Translation Kit Team
//   Reimplimentation: 2002-04
//
// Description: Utility functions for the Translation Kit
//
//
// Copyright (c) 2002 OpenBeOS Project
//
// Original Version: Copyright 1998, Be Incorporated, All Rights Reserved.
//                   Copyright 1995-1997, Jon Watte
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
#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <File.h>
#include <MenuItem.h>
#include <Resources.h>
#include <stdlib.h>
#include <TextView.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <TranslationUtils.h>

// ---------------------------------------------------------------
// Constructor
//
// Does nothing! :) This class has no data members.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BTranslationUtils::BTranslationUtils()
{
}

// ---------------------------------------------------------------
// Desstructor
//
// Does nothing! :) This class has no data members.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BTranslationUtils::~BTranslationUtils()
{
}

// ---------------------------------------------------------------
// Constructor
//
// Does nothing! :) This class has no data members.
//
// Preconditions:
//
// Parameters: kUtils, not used
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BTranslationUtils::BTranslationUtils(const BTranslationUtils &kUtils)
{
}

// ---------------------------------------------------------------
// operator=
//
// Does nothing! :) This class has no data members.
//
// Preconditions:
//
// Parameters: kUtils, not used
//
// Postconditions:
//
// Returns: reference to the object
// ---------------------------------------------------------------
BTranslationUtils &
BTranslationUtils::operator=(const BTranslationUtils &kUtils)
{
	return *this;
}

// ---------------------------------------------------------------
// GetBitmap
//
// Returns a BBitmap object for the bitmap file or resource
// kName. The user has to delete this object. It first tries
// to open kName as a file, then as a resource.
//
// Preconditions:
//
// Parameters: kName, the name of the bitmap file or resource to 
//                    be returned
//             roster, BTranslatorRoster used to do the translation
//
// Postconditions:
//
// Returns: NULL, if the file could not be opened and the
//                resource couldn't be found or couldn't be
//                translated to a BBitmap
//          BBitmap * to the bitmap reference by kName
// ---------------------------------------------------------------
BBitmap *
BTranslationUtils::GetBitmap(const char *kName, BTranslatorRoster *roster)
{
	BBitmap *pBitmap = GetBitmapFile(kName, roster);
		// Try loading a bitmap from the file named name
	
	// Try loading the bitmap as an application resource
	if (pBitmap == NULL)
		pBitmap = GetBitmap(B_TRANSLATOR_BITMAP, kName, roster);
	
	return pBitmap;
}

// ---------------------------------------------------------------
// GetBitmap
//
// Returns a BBitmap object for the bitmap resource identified by
// the type type with the resource id, id. 
// The user has to delete this object.
//
// Preconditions:
//
// Parameters: type, the type of resource to be loaded
//             id, the id for the resource to be loaded
//             roster, BTranslatorRoster used to do the translation
//
// Postconditions:
//
// Returns: NULL, if the resource couldn't be loaded or couldn't
//                be translated to a BBitmap
//          BBitmap * to the bitmap identified by type and id
// ---------------------------------------------------------------
BBitmap *
BTranslationUtils::GetBitmap(uint32 type, int32 id, BTranslatorRoster *roster)
{
	BResources *pResources = BApplication::AppResources();
		// Remember: pResources must not be freed because
		// it belongs to the application
	if (pResources == NULL || pResources->HasResource(type, id) == false)
		return NULL;
	
	// Load the bitmap resource from the application file 
	// pRawData should be NULL if the resource is an
	// unknown type or not available
	size_t bitmapSize = 0;
	const void *kpRawData = pResources->LoadResource(type, id, &bitmapSize);
	if (kpRawData == NULL || bitmapSize == 0)
		return NULL;
	
	BMemoryIO memio(kpRawData, bitmapSize);
		// Put the pointer to the raw image data into a BMemoryIO object
		// so that it can be used with BTranslatorRoster->Translate() in
		// the TranslateToBitmap() function
	
	return TranslateToBitmap(&memio, roster);
		// Translate the data in memio using the BTranslatorRoster roster
}

// ---------------------------------------------------------------
// GetBitmap
//
// Returns a BBitmap object for the bitmap resource identified by
// the type type with the resource name, kName. 
// The user has to delete this object. Note that a resource type
// and name does not uniquely identify a resource in a file.
//
// Preconditions:
//
// Parameters: type, the type of resource to be loaded
//             kName, the name of the resource to be loaded
//             roster, BTranslatorRoster used to do the translation
//
// Postconditions:
//
// Returns: NULL, if the resource couldn't be loaded or couldn't
//                be translated to a BBitmap
//          BBitmap * to the bitmap identified by type and kName
// ---------------------------------------------------------------
BBitmap *
BTranslationUtils::GetBitmap(uint32 type, const char *kName,
	BTranslatorRoster *roster)
{
	BResources *pResources = BApplication::AppResources();
		// Remember: pResources must not be freed because
		// it belongs to the application
	if (pResources == NULL || pResources->HasResource(type, kName) == false)
		return NULL;
	
	// Load the bitmap resource from the application file 
	size_t bitmapSize = 0;
	const void *kpRawData = pResources->LoadResource(type, kName, &bitmapSize);
	if (kpRawData == NULL || bitmapSize == 0)
		return NULL;
	
	BMemoryIO memio(kpRawData, bitmapSize);
		// Put the pointer to the raw image data into a BMemoryIO object so
		// that it can be used with BTranslatorRoster->Translate() 
	
	return TranslateToBitmap(&memio, roster);
		// Translate the data in memio using the BTranslatorRoster roster
}

// ---------------------------------------------------------------
// GetBitmapFile
//
// Returns a BBitmap object for the bitmap file named kName.
// The user has to delete this object.
//
// Preconditions:
//
// Parameters: kName, the name of the bitmap file
//             roster, BTranslatorRoster used to do the translation
//
// Postconditions:
//
// Returns: NULL, if the file couldn't be opened or couldn't
//                be translated to a BBitmap
//          BBitmap * to the bitmap file named kName
// ---------------------------------------------------------------
BBitmap *
BTranslationUtils::GetBitmapFile(const char *kName, BTranslatorRoster *roster)
{
	BFile bitmapFile(kName, B_READ_ONLY);
	if (bitmapFile.InitCheck() != B_OK)
		return NULL;

	return TranslateToBitmap(&bitmapFile, roster);
		// Translate the data in memio using the BTranslatorRoster roster
}

// ---------------------------------------------------------------
// GetBitmap
//
// Returns a BBitmap object for the bitmap file with the entry_ref
// kRef. The user has to delete this object.
//
// Preconditions:
//
// Parameters: kRef, the entry_ref for the bitmap file
//             roster, BTranslatorRoster used to do the translation
//
// Postconditions:
//
// Returns: NULL, if the file couldn't be opened or couldn't
//                be translated to a BBitmap
//          BBitmap * to the bitmap file referenced by kRef
// ---------------------------------------------------------------
BBitmap *
BTranslationUtils::GetBitmap(const entry_ref *kRef, BTranslatorRoster *roster)
{
	BFile bitmapFile(kRef, B_READ_ONLY);
	if (bitmapFile.InitCheck() != B_OK)
		return NULL;

	return TranslateToBitmap(&bitmapFile, roster);
		// Translate the data in bitmapFile using the BTranslatorRoster roster
}

// ---------------------------------------------------------------
// GetBitmap
//
// Returns a BBitmap object from the BPositionIO *stream. The
// user must delete the returned object
//
// Preconditions:
//
// Parameters: stream, the stream with bitmap data in it
//             roster, BTranslatorRoster used to do the translation
//
// Postconditions:
//
// Returns: NULL, if the stream couldn't be translated to a BBitmap
//          BBitmap * to the bitmap file named kName
// ---------------------------------------------------------------
BBitmap *
BTranslationUtils::GetBitmap(BPositionIO *stream, BTranslatorRoster *roster)
{	
	return TranslateToBitmap(stream, roster);
		// Translate the data in memio using the BTranslatorRoster roster
}

// ---------------------------------------------------------------
// GetStyledText
//
// This function translates the styled text in fromStream and 
// inserts it at the end of the text in intoView, using the
// BTranslatorRoster *roster to do the translation. The structs
// that make it possible to work with the translated data are
// defined in
// /boot/develop/headers/be/translation/TranslatorFormats.h
//
// Preconditions:
//
// Parameters: fromStream, the stream with the styled text
//             intoView, the view where the test will be inserted
//             roster, BTranslatorRoster used to do the translation
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if fromStream or intoView is NULL
//          B_ERROR, if any other error occurred
//          B_NO_ERROR, if successful
// ---------------------------------------------------------------
status_t
BTranslationUtils::GetStyledText(BPositionIO *fromStream, BTextView *intoView,
	BTranslatorRoster *roster)
{
	if (fromStream == NULL || intoView == NULL)
		return B_BAD_VALUE;
	
	// Use default Translator if none is specified 
	if (roster == NULL) {
		roster = BTranslatorRoster::Default();
		if (roster == NULL)
			return B_ERROR;
	}

	// Translate the file from whatever format it is in the file
	// to the B_STYLED_TEXT_FORMAT, placing the translated data into mallio
	BMallocIO mallio;
	if (roster->Translate(fromStream, NULL, NULL, &mallio,
		B_STYLED_TEXT_FORMAT) < B_OK)
		return B_ERROR;
	
	// make sure there is enough data to fill the stream header
	const size_t kStreamHeaderSize = sizeof(TranslatorStyledTextStreamHeader);
	if (mallio.BufferLength() < kStreamHeaderSize)
		return B_ERROR;
	
	// copy the stream header from the mallio buffer
	TranslatorStyledTextStreamHeader stm_header =
		*((TranslatorStyledTextStreamHeader *) mallio.Buffer());
	
	// convert the stm_header.header struct to the host format
	const size_t kRecordHeaderSize = sizeof(TranslatorStyledTextRecordHeader);
	if (swap_data(B_UINT32_TYPE, &stm_header.header, kRecordHeaderSize,
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	if (swap_data(B_INT32_TYPE, &stm_header.version, sizeof(int32),
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	if (stm_header.header.magic != 'STXT')
		return B_ERROR;
	
	// copy the text header from the mallio buffer
	uint32 offset = stm_header.header.header_size +
		stm_header.header.data_size;
	const size_t kTextHeaderSize = sizeof(TranslatorStyledTextTextHeader);
	if (mallio.BufferLength() < offset + kTextHeaderSize)
		return B_ERROR;
	
	TranslatorStyledTextTextHeader txt_header = 
		*((TranslatorStyledTextTextHeader *) ((char *) mallio.Buffer() +
			offset));
	
	// convert the stm_header.header struct to the host format
	if (swap_data(B_UINT32_TYPE, &txt_header.header, kRecordHeaderSize,
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	if (swap_data(B_INT32_TYPE, &txt_header.charset, sizeof(int32),
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	if (txt_header.header.magic != 'TEXT')
		return B_ERROR;
	if (txt_header.charset != B_UNICODE_UTF8)
		return B_ERROR;
	
	offset += txt_header.header.header_size;
	if (mallio.BufferLength() < offset + txt_header.header.data_size)
		return B_ERROR;
	
	const char *pTextData = ((const char *) mallio.Buffer()) + offset;
		// point text pointer at the actual character data
	
	if (mallio.BufferLength() > offset + txt_header.header.data_size) {
		// If the stream contains information beyond the text data
		// (which means that this data is probably styled text data)

		offset += txt_header.header.data_size;
		const size_t kStyleHeaderSize =
			sizeof(TranslatorStyledTextStyleHeader);
		if (mallio.BufferLength() < offset + kStyleHeaderSize)
			return B_ERROR;
		
		TranslatorStyledTextStyleHeader stl_header = 
			*((TranslatorStyledTextStyleHeader *)
				((char *) mallio.Buffer() + offset));
		if (swap_data(B_UINT32_TYPE, &stl_header.header, kRecordHeaderSize,
			B_SWAP_BENDIAN_TO_HOST) != B_OK)
			return B_ERROR;
		if (swap_data(B_UINT32_TYPE, &stl_header.apply_offset, sizeof(uint32),
			B_SWAP_BENDIAN_TO_HOST) != B_OK)
			return B_ERROR;
		if (swap_data(B_UINT32_TYPE, &stl_header.apply_length, sizeof(uint32),
			B_SWAP_BENDIAN_TO_HOST) != B_OK)
			return B_ERROR;
		if (stl_header.header.magic != 'STYL')
			return B_ERROR;
		
		offset += stl_header.header.header_size;
		if (mallio.BufferLength() < offset + stl_header.header.data_size)
			return B_ERROR;
		
		// set pRawData to the flattened run array data
		const void *kpRawData = (const void *) ((char *) mallio.Buffer() +
			offset);
		text_run_array *pRunArray = BTextView::UnflattenRunArray(kpRawData);
	
		if (pRunArray) {
			intoView->Insert(intoView->TextLength(), pTextData,
				txt_header.header.data_size, pRunArray);
			free(pRunArray);
			pRunArray = NULL;
		} else
			return B_ERROR;
	} else
		intoView->Insert(intoView->TextLength(), pTextData,
			txt_header.header.data_size);

	return B_NO_ERROR;
}

// ---------------------------------------------------------------
// PutStyledText
//
// This function takes styled text data from fromView and writes it to
// intoStream.  The plain text data and styled text data are combined
// when they are written to intoStream.  This is different than how
// a save operation in StyledEdit works.  With StyledEdit, it writes
// plain text data to the file, but puts the styled text data in
// the "styles" attribute.  In other words, this function writes
// styled text data to files in a manner that isn't human readable.
// 
// So, if you want to write styled text
// data to a file, and you want it to behave the way StyledEdit does,
// you want to use the BTranslationUtils::WriteStyledEditFile() function.
//
// Preconditions:
//
// Parameters: fromView, the view with the styled text in it
//             intoStream, the stream where the styled text is put
//             roster, not used
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if fromView or intoStream is NULL
//          B_ERROR, if anything else went wrong
//          B_NO_ERROR, if successful
// ---------------------------------------------------------------
status_t
BTranslationUtils::PutStyledText(BTextView *fromView, BPositionIO *intoStream,
	BTranslatorRoster *roster)
{
	if (fromView == NULL || intoStream == NULL)
		return B_BAD_VALUE;
	
	int32 textLength = fromView->TextLength();
	if (textLength < 0)
		return B_ERROR;
	
	const char *pTextData = fromView->Text();
		// its OK if the result of fromView->Text() is NULL
	
	int32 runArrayLength = 0;
	text_run_array *pRunArray = fromView->RunArray(0, textLength,
		&runArrayLength);
	if (pRunArray == NULL)
		return B_ERROR;
	
	int32 flatRunArrayLength = 0;
	void *pflatRunArray =
		BTextView::FlattenRunArray(pRunArray, &flatRunArrayLength);
	if (pflatRunArray == NULL) {
		free(pRunArray);
		pRunArray = NULL;
		
		return B_ERROR;
	}
	
	// Rather than use a goto, I put a whole bunch of code that
	// could error out inside of a loop, and break out of the loop
	// if there is an error. If there is no error, loop is set
	// to false.  This is so that I don't have to put free()
	// calls everywhere there could be an error.
	
	// This block of code is where I do all of the writting of the
	// data to the stream.  I've gathered all of the data that I
	// need at this point.
	bool loop = true;
	while (loop) {
		const size_t kStreamHeaderSize =
			sizeof(TranslatorStyledTextStreamHeader);
		TranslatorStyledTextStreamHeader stm_header;
		stm_header.header.magic = 'STXT';
		stm_header.header.header_size = kStreamHeaderSize;
		stm_header.header.data_size = 0;
		stm_header.version = 100;
		
		// convert the stm_header.header struct to the host format
		const size_t kRecordHeaderSize =
			sizeof(TranslatorStyledTextRecordHeader);
		if (swap_data(B_UINT32_TYPE, &stm_header.header, kRecordHeaderSize,
			B_SWAP_HOST_TO_BENDIAN) != B_OK)
			break;
		if (swap_data(B_INT32_TYPE, &stm_header.version, sizeof(int32),
			B_SWAP_HOST_TO_BENDIAN) != B_OK)
			break;
		
		const size_t kTextHeaderSize = sizeof(TranslatorStyledTextTextHeader);
		TranslatorStyledTextTextHeader txt_header;
		txt_header.header.magic = 'TEXT';
		txt_header.header.header_size = kTextHeaderSize;
		txt_header.header.data_size = textLength;
		txt_header.charset = B_UNICODE_UTF8;
		
		// convert the stm_header.header struct to the host format
		if (swap_data(B_UINT32_TYPE, &txt_header.header, kRecordHeaderSize,
			B_SWAP_HOST_TO_BENDIAN) != B_OK)
			break;
		if (swap_data(B_INT32_TYPE, &txt_header.charset, sizeof(int32),
			B_SWAP_HOST_TO_BENDIAN) != B_OK)
			break;
		
		const size_t kStyleHeaderSize =
			sizeof(TranslatorStyledTextStyleHeader);
		TranslatorStyledTextStyleHeader stl_header;
		stl_header.header.magic = 'STYL';
		stl_header.header.header_size = kStyleHeaderSize;
		stl_header.header.data_size = flatRunArrayLength;
		stl_header.apply_offset = 0;
		stl_header.apply_length = textLength;
		
		// convert the stl_header.header struct to the host format
		if (swap_data(B_UINT32_TYPE, &stl_header.header, kRecordHeaderSize,
			B_SWAP_HOST_TO_BENDIAN) != B_OK)
			break;
		if (swap_data(B_UINT32_TYPE, &stl_header.apply_offset, sizeof(uint32),
			B_SWAP_HOST_TO_BENDIAN) != B_OK)
			break;
		if (swap_data(B_UINT32_TYPE, &stl_header.apply_length, sizeof(uint32),
			B_SWAP_HOST_TO_BENDIAN) != B_OK)
			break;
		
		// Here, you can see the structure of the styled text data by
		// observing the order that the various structs and data are
		// written to the stream
		ssize_t amountWritten = 0;
		amountWritten = intoStream->Write(&stm_header, kStreamHeaderSize);
		if ((size_t) amountWritten != kStreamHeaderSize)
			break;
		amountWritten = intoStream->Write(&txt_header, kTextHeaderSize);
		if ((size_t) amountWritten != kTextHeaderSize)
			break;
		amountWritten = intoStream->Write(pTextData, textLength);
		if (amountWritten != textLength)
			break;
		amountWritten = intoStream->Write(&stl_header, kStyleHeaderSize);
		if ((size_t) amountWritten != kStyleHeaderSize)
			break;
		amountWritten = intoStream->Write(pflatRunArray, flatRunArrayLength);
		if (amountWritten != flatRunArrayLength)
			break;
		
		loop = false;
			// gracefully break out of the loop
	} // end of while(loop)
	
	free(pflatRunArray);
	pflatRunArray = NULL;
	free(pRunArray);
	pRunArray = NULL;
	
	if (loop)
		return B_ERROR;
	else
		return B_NO_ERROR;
}

// ---------------------------------------------------------------
// WriteStyledEditFile
//
// This function writes the styled text data from fromView
// and stores it in the file intoFile.
//
// This function is similar to PutStyledText() except that it
// only writes styled text data to files and it puts the
// plain text data in the file and stores the styled data as
// the attribute "styles".
//
// You can use PutStyledText() to write styled text data
// to files, but it writes the data in a format that isn't
// human readable.
//
// It is important to note that this function doesn't
// write files in exactly the same manner that you get
// when you do a File->Save operation in StyledEdit.
// This function doesn't write all of the attributes
// that StyledEdit does, even though it easily could.
//
// Preconditions:
//
// Parameters: fromView, the view with the styled text
//             intoFile, the file where the styled text
//                       is written to
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if either parameter is NULL
//          B_ERROR, if anything else went wrong
//          B_OK, if successful
// ---------------------------------------------------------------
status_t
BTranslationUtils::WriteStyledEditFile(BTextView *fromView, BFile *intoFile)
{
	if (fromView == NULL || intoFile == NULL)		
		return B_BAD_VALUE;
	
	int32 textLength = fromView->TextLength();
	if (textLength < 0)
		return B_ERROR;
	
	const char *kpTextData = fromView->Text();
	if (kpTextData == NULL && textLength != 0)
		return B_ERROR;
	
	// Write plain text data to file
	ssize_t amtWritten = intoFile->Write(kpTextData, textLength);
	if (amtWritten != textLength)
		return B_ERROR;
	
	// Write attributes
	// BEOS:TYPE
	// (this is so that the BeOS will recognize this file as a text file)
	amtWritten = intoFile->WriteAttr("BEOS:TYPE", 'MIMS', 0, "text/plain", 11);
	if ((size_t) amtWritten != 11)
		return B_ERROR;
	
	text_run_array *pRunArray = fromView->RunArray(0, fromView->TextLength());
	if (pRunArray == NULL)
		return B_ERROR;
	
	int32 runArraySize = 0;
	void *pflatRunArray = BTextView::FlattenRunArray(pRunArray, &runArraySize);
	if (pflatRunArray == NULL) {
		free(pRunArray);
		pRunArray = NULL;
		return B_ERROR;
	}
	if (runArraySize < 0) {
		free(pflatRunArray);
		pflatRunArray = NULL;
		free(pRunArray);
		pRunArray = NULL;
		return B_ERROR;
	}
	
	// This is how the styled text data is stored in the file
	// (the trick is that it isn't actually stored in the file, its stored 
	// as an attribute in the file's node)
	amtWritten = intoFile->WriteAttr("styles", B_RAW_TYPE, 0, pflatRunArray,
		runArraySize);
	free(pflatRunArray);
	pflatRunArray = NULL;		
	free(pRunArray);
	pRunArray = NULL;
	if (amtWritten == runArraySize)
		return B_OK;
	else
		return B_ERROR;
}

// ---------------------------------------------------------------
// GetDefaultSettings
//
// Each translator can have default settings, set by the
// "translations" control panel. You can read these settings to
// pass on to a translator using one of these functions.
//
// Preconditions:
//
// Parameters: forTranslator, the translator the settings are for
//             roster, the roster used to get the settings
//
// Postconditions:
//
// Returns: NULL, if anything went wrong
//          BMessage * of configuration data for forTranslator
// ---------------------------------------------------------------
BMessage *
BTranslationUtils::GetDefaultSettings(translator_id forTranslator,
	BTranslatorRoster *roster)
{
	// Use default Translator if none is specified 
	if (roster == NULL) {
		roster = BTranslatorRoster::Default();
		if (roster == NULL)
			return NULL;
	}
	
	BMessage *pMessage = new BMessage();
	if (pMessage == NULL)
		return NULL;
	
	status_t result = roster->GetConfigurationMessage(forTranslator, pMessage);
	switch (result) {
		case B_OK:
			break;
			
		case B_NO_TRANSLATOR:
			// Be's version seems to just pass an empty
			// BMessage for this case, well, in some cases anyway
			break;
			
		case B_NOT_INITIALIZED:
			delete pMessage;
			pMessage = NULL;
			break;
			
		case B_BAD_VALUE:
			delete pMessage;
			pMessage = NULL;
			break;
			
		default:
			break;
	}
	
	return pMessage;
}

// ---------------------------------------------------------------
// GetDefaultSettings
//
// Attempts to find the translator settings for 
// the translator named kTranslatorName with a version of
// translatorVersion.
//
// Preconditions:
//
// Parameters: kTranslatorName, the name of the translator
//                              the settings are for
//             translatorVersion, the version of the translator
//                                to retrieve
//
// Postconditions:
//
// Returns: NULL, if anything went wrong
//          BMessage * of configuration data for kTranslatorName
// ---------------------------------------------------------------
BMessage *
BTranslationUtils::GetDefaultSettings(const char *kTranslatorName,
	int32 translatorVersion)
{
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	translator_id *translators = NULL;
	int32 numTranslators = 0;
	if (roster == NULL
		|| roster->GetAllTranslators(&translators, &numTranslators) != B_OK)
		return NULL;

	// Cycle through all of the default translators
	// looking for a translator that matches the name and version
	// that I was given
	BMessage *pMessage = NULL;
	const char *currentTranName = NULL, *currentTranInfo = NULL;
	int32 currentTranVersion = 0;
	for (int i = 0; i < numTranslators; i++) {

		if (roster->GetTranslatorInfo(translators[i], &currentTranName,
			&currentTranInfo, &currentTranVersion) == B_OK) {

			if (currentTranVersion == translatorVersion
				&& strcmp(currentTranName, kTranslatorName) == 0) {
				pMessage = GetDefaultSettings(translators[i], roster);
				break;				
			}
		}
	}
	
	delete[] translators;
	return pMessage;
}

// ---------------------------------------------------------------
// AddTranslationItems
//
// Envious of that "Save As" menu in ShowImage? Well, you can have your own!
// AddTranslationItems will add menu items for all translations from the
// basic format you specify (B_TRANSLATOR_BITMAP, B_TRANSLATOR_TEXT etc).
// The translator ID and format constant chosen will be added to the message
// that is sent to you when the menu item is selected.
//
// The following code is a modified version of code
// written by Jon Watte from
// http://www.b500.com/bepage/TranslationKit2.html
//
// Preconditions:
//
// Parameters: intoMenu, the menu where the entries are created
//             fromType, the type of translators to put on
//                       intoMenu
//             kModel, the BMessage model for creating the menu
//                     if NULL, B_TRANSLATION_MENU is used
//             kTranslationIdName, the name used for
//                                 translator_id in the menuitem,
//                                 if NULL, be:translator is used
//             kTranslatorTypeName, the name used for
//                                  output format id in the menuitem
//             roster, BTranslatorRoster used to find translators
//                     if NULL, the default translators are used
//             
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if intoMenu is NULL
//          B_OK, if successful
//          error value if not successful
// ---------------------------------------------------------------
status_t
BTranslationUtils::AddTranslationItems(BMenu *intoMenu, uint32 fromType,
	const BMessage *kModel, const char *kTranslatorIdName,
	const char *kTranslatorTypeName, BTranslatorRoster *roster)
{
	if (!intoMenu)
		return B_BAD_VALUE;
		
	if (!roster)
		roster = BTranslatorRoster::Default();

	if (!kTranslatorIdName)
		kTranslatorIdName = "be:translator";

	if (!kTranslatorTypeName)
		kTranslatorTypeName = "be:type";

	translator_id * ids = NULL;
	int32 count = 0;
	status_t err = roster->GetAllTranslators(&ids, &count);
	if (err < B_OK)
		return err;

	for (int tix = 0; tix < count; tix++) {
		const translation_format *formats = NULL;
		int32 numFormats = 0;
		bool ok = false;
		err = roster->GetInputFormats(ids[tix], &formats, &numFormats);
		if (err == B_OK) {
			for (int iix = 0; iix < numFormats; iix++) {
				if (formats[iix].type == fromType) {
					ok = true;
					break;
				}
			}
		}
		if (!ok)
			continue;

		err = roster->GetOutputFormats(ids[tix], &formats, &numFormats); 
		if (err == B_OK) {
			for (int oix = 0; oix < numFormats; oix++) { 
				if (formats[oix].type != fromType) { 
					BMessage *itemmsg; 
					if (kModel)
						itemmsg = new BMessage(*kModel);
					else
						itemmsg = new BMessage(B_TRANSLATION_MENU);
					itemmsg->AddInt32(kTranslatorIdName, ids[tix]);
					itemmsg->AddInt32(kTranslatorTypeName, formats[oix].type);
					intoMenu->AddItem(
						new BMenuItem(formats[oix].name, itemmsg));
				}
			}
		}
	}

	delete[] ids;
	return B_OK;
}

// ---------------------------------------------------------------
// TranslateToBitmap
//
// Translates the image data from pio to the type type using the
// supplied BTranslatorRoster. If BTranslatorRoster is not supplied 
// the default BTranslatorRoster is used. This function is used
// indirectly by the GetBitmap functions
//
// Preconditions:
//
// Parameters: pio, the stream to get the bitmap from
//             type, the type of data in the stream
//             roster, BTranslatorRoster used to do the
//                     translation, if NULL the default
//                     translators are used
//
// Postconditions:
//
// Returns: NULL, if anything went wrong
//          BBitmap * for the bitmap data from pio if successful
// ---------------------------------------------------------------
BBitmap *
BTranslationUtils::TranslateToBitmap(BPositionIO *pio,
	BTranslatorRoster *roster)
{
	if (pio == NULL)
		return NULL;
	
	// Use default Translator if none is specified 
	if (roster == NULL) {
		roster = BTranslatorRoster::Default();
		if (roster == NULL)
			return NULL;
	}

	// Translate the file from whatever format it is in the file
	// to the type format so that it can be stored in a BBitmap
	BBitmapStream bitmapStream;
	if (roster->Translate(pio, NULL, NULL, &bitmapStream,
		B_TRANSLATOR_BITMAP) < B_OK)
		return NULL;
	
	// Detach the BBitmap from the BBitmapStream so the user
	// of this function can do what they please with it.
	BBitmap *pBitmap = NULL;
	if (bitmapStream.DetachBitmap(&pBitmap) == B_NO_ERROR)
		return pBitmap;
	else
		return NULL;
}

