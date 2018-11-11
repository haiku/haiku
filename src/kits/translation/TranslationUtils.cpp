/*
 * Copyright 2002-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*! Utility functions for the Translation Kit */


#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Entry.h>
#include <File.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <ObjectList.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <UTF8.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


using namespace BPrivate;


BTranslationUtils::BTranslationUtils()
{
}


BTranslationUtils::~BTranslationUtils()
{
}


BTranslationUtils::BTranslationUtils(const BTranslationUtils &kUtils)
{
}


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
		// the GetBitmap(BPositionIO *, BTranslatorRoster *) function

	return GetBitmap(&memio, roster);
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

	return GetBitmap(&memio, roster);
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
	if (!be_app || !kName || kName[0] == '\0')
		return NULL;

	BPath path;
	if (kName[0] != '/') {
		// If kName is a relative path, use the path of the application's
		// executable as the base for the relative path
		app_info info;
		if (be_app->GetAppInfo(&info) != B_OK)
			return NULL;
		BEntry appRef(&info.ref);
		if (path.SetTo(&appRef) != B_OK)
			return NULL;
		if (path.GetParent(&path) != B_OK)
			return NULL;
		if (path.Append(kName) != B_OK)
			return NULL;

	} else if (path.SetTo(kName) != B_OK)
		return NULL;

	BFile bitmapFile(path.Path(), B_READ_ONLY);
	if (bitmapFile.InitCheck() != B_OK)
		return NULL;

	return GetBitmap(&bitmapFile, roster);
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

	return GetBitmap(&bitmapFile, roster);
		// Translate the data in bitmapFile using the BTranslatorRoster roster
}

// ---------------------------------------------------------------
// GetBitmap
//
// Returns a BBitmap object from the BPositionIO *stream. The
// user must delete the returned object. This GetBitmap function
// is used by the other GetBitmap functions to do all of the
// "real" work.
//
// Preconditions:
//
// Parameters: stream, the stream with bitmap data in it
//             roster, BTranslatorRoster used to do the translation
//
// Postconditions:
//
// Returns: NULL, if the stream couldn't be translated to a BBitmap
//          BBitmap * for the bitmap data from pio if successful
// ---------------------------------------------------------------
BBitmap *
BTranslationUtils::GetBitmap(BPositionIO *stream, BTranslatorRoster *roster)
{
	if (stream == NULL)
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
	if (roster->Translate(stream, NULL, NULL, &bitmapStream,
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


/*!
	This function translates the styled text in fromStream and
	inserts it at the end of the text in intoView, using the
	BTranslatorRoster *roster to do the translation. The structs
	that make it possible to work with the translated data are
	defined in
	/boot/develop/headers/be/translation/TranslatorFormats.h

	\param source the stream with the styled text
	\param intoView the view where the test will be inserted
		roster, BTranslatorRoster used to do the translation
	\param the encoding to use, defaults to UTF-8

	\return B_BAD_VALUE, if fromStream or intoView is NULL
	\return B_ERROR, if any other error occurred
	\return B_OK, if successful
*/
status_t
BTranslationUtils::GetStyledText(BPositionIO* source, BTextView* intoView,
	const char* encoding, BTranslatorRoster* roster)
{
	if (source == NULL || intoView == NULL)
		return B_BAD_VALUE;

	// Use default Translator if none is specified
	if (roster == NULL) {
		roster = BTranslatorRoster::Default();
		if (roster == NULL)
			return B_ERROR;
	}

	BMessage config;
	if (encoding != NULL && encoding[0])
		config.AddString("be:encoding", encoding);

	// Translate the file from whatever format it is to B_STYLED_TEXT_FORMAT
	// we understand
	BMallocIO mallocIO;
	if (roster->Translate(source, NULL, &config, &mallocIO,
			B_STYLED_TEXT_FORMAT) < B_OK)
		return B_BAD_TYPE;

	const uint8* buffer = (const uint8*)mallocIO.Buffer();

	// make sure there is enough data to fill the stream header
	const size_t kStreamHeaderSize = sizeof(TranslatorStyledTextStreamHeader);
	if (mallocIO.BufferLength() < kStreamHeaderSize)
		return B_BAD_DATA;

	// copy the stream header from the mallio buffer
	TranslatorStyledTextStreamHeader header =
		*(reinterpret_cast<const TranslatorStyledTextStreamHeader *>(buffer));

	// convert the stm_header.header struct to the host format
	const size_t kRecordHeaderSize = sizeof(TranslatorStyledTextRecordHeader);
	swap_data(B_UINT32_TYPE, &header.header, kRecordHeaderSize, B_SWAP_BENDIAN_TO_HOST);
	swap_data(B_INT32_TYPE, &header.version, sizeof(int32), B_SWAP_BENDIAN_TO_HOST);

	if (header.header.magic != 'STXT')
		return B_BAD_TYPE;

	// copy the text header from the mallocIO buffer

	uint32 offset = header.header.header_size + header.header.data_size;
	const size_t kTextHeaderSize = sizeof(TranslatorStyledTextTextHeader);
	if (mallocIO.BufferLength() < offset + kTextHeaderSize)
		return B_BAD_DATA;

	TranslatorStyledTextTextHeader textHeader =
		*(const TranslatorStyledTextTextHeader *)(buffer + offset);

	// convert the stm_header.header struct to the host format
	swap_data(B_UINT32_TYPE, &textHeader.header, kRecordHeaderSize, B_SWAP_BENDIAN_TO_HOST);
	swap_data(B_INT32_TYPE, &textHeader.charset, sizeof(int32), B_SWAP_BENDIAN_TO_HOST);

	if (textHeader.header.magic != 'TEXT' || textHeader.charset != B_UNICODE_UTF8)
		return B_BAD_TYPE;

	offset += textHeader.header.header_size;
	if (mallocIO.BufferLength() < offset + textHeader.header.data_size) {
		// text buffer misses its end; handle this gracefully
		textHeader.header.data_size = mallocIO.BufferLength() - offset;
	}

	const char* text = (const char*)buffer + offset;
		// point text pointer at the actual character data
	bool hasStyles = false;

	if (mallocIO.BufferLength() > offset + textHeader.header.data_size) {
		// If the stream contains information beyond the text data
		// (which means that this data is probably styled text data)

		offset += textHeader.header.data_size;
		const size_t kStyleHeaderSize =
			sizeof(TranslatorStyledTextStyleHeader);
		if (mallocIO.BufferLength() >= offset + kStyleHeaderSize) {
			TranslatorStyledTextStyleHeader styleHeader =
				*(reinterpret_cast<const TranslatorStyledTextStyleHeader *>(buffer + offset));
			swap_data(B_UINT32_TYPE, &styleHeader.header, kRecordHeaderSize, B_SWAP_BENDIAN_TO_HOST);
			swap_data(B_UINT32_TYPE, &styleHeader.apply_offset, sizeof(uint32), B_SWAP_BENDIAN_TO_HOST);
			swap_data(B_UINT32_TYPE, &styleHeader.apply_length, sizeof(uint32), B_SWAP_BENDIAN_TO_HOST);
			if (styleHeader.header.magic == 'STYL') {
				offset += styleHeader.header.header_size;
				if (mallocIO.BufferLength() >= offset + styleHeader.header.data_size)
					hasStyles = true;
			}
		}
	}

	text_run_array *runArray = NULL;
	if (hasStyles)
		runArray = BTextView::UnflattenRunArray(buffer + offset);

	if (runArray != NULL) {
		intoView->Insert(intoView->TextLength(),
			text, textHeader.header.data_size, runArray);
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		BTextView::FreeRunArray(runArray);
#else
		free(runArray);
#endif
	} else {
		intoView->Insert(intoView->TextLength(), text,
			textHeader.header.data_size);
	}

	return B_OK;
}


status_t
BTranslationUtils::GetStyledText(BPositionIO* source, BTextView* intoView,
	BTranslatorRoster* roster)
{
	return GetStyledText(source, intoView, NULL, roster);
}


/*!
	This function takes styled text data from fromView and writes it to
	intoStream.  The plain text data and styled text data are combined
	when they are written to intoStream.  This is different than how
	a save operation in StyledEdit works.  With StyledEdit, it writes
	plain text data to the file, but puts the styled text data in
	the "styles" attribute.  In other words, this function writes
	styled text data to files in a manner that isn't human readable.

	So, if you want to write styled text
	data to a file, and you want it to behave the way StyledEdit does,
	you want to use the BTranslationUtils::WriteStyledEditFile() function.

	\param fromView, the view with the styled text in it
	\param intoStream, the stream where the styled text is put
		roster, not used

	\return B_BAD_VALUE, if fromView or intoStream is NULL
	\return B_ERROR, if anything else went wrong
	\return B_NO_ERROR, if successful
*/
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
	text_run_array *runArray = fromView->RunArray(0, textLength,
		&runArrayLength);
	if (runArray == NULL)
		return B_ERROR;

	int32 flatRunArrayLength = 0;
	void *pflatRunArray =
		BTextView::FlattenRunArray(runArray, &flatRunArrayLength);
	if (pflatRunArray == NULL) {
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		BTextView::FreeRunArray(runArray);
#else
		free(runArray);
#endif
		return B_ERROR;
	}

	// Rather than use a goto, I put a whole bunch of code that
	// could error out inside of a loop, and break out of the loop
	// if there is an error.

	// This block of code is where I do all of the writing of the
	// data to the stream. I've gathered all of the data that I
	// need at this point.
	bool ok = false;
	while (!ok) {
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

		ok = true;
			// gracefully break out of the loop
	}

	free(pflatRunArray);
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	BTextView::FreeRunArray(runArray);
#else
	free(runArray);
#endif

	return ok ? B_OK : B_ERROR;
}


/*!
	\brief Writes the styled text data from \a view to the specified \a file.

	This function is similar to PutStyledText() except that it
	only writes styled text data to files and it puts the
	plain text data in the file and stores the styled data as
	the attribute "styles".

	You can use PutStyledText() to write styled text data
	to files, but it writes the data in a format that isn't
	human readable.

	\param view the view with the styled text
	\param file the file where the styled text is written to
	\param the encoding to use, defaults to UTF-8

	\return B_BAD_VALUE, if either parameter is NULL
		B_OK, if successful, and any possible file error
		if writing failed.
*/
status_t
BTranslationUtils::WriteStyledEditFile(BTextView* view, BFile* file, const char *encoding)
{
	if (view == NULL || file == NULL)
		return B_BAD_VALUE;

	int32 textLength = view->TextLength();
	if (textLength < 0)
		return B_ERROR;

	const char *text = view->Text();
	if (text == NULL && textLength != 0)
		return B_ERROR;

	// move to the start of the file if not already there
	status_t status = file->Seek(0, SEEK_SET);
	if (status != B_OK)
		return status;

	const BCharacterSet* characterSet = NULL;
	if (encoding != NULL && strcmp(encoding, ""))
		characterSet = BCharacterSetRoster::FindCharacterSetByName(encoding);
	if (characterSet == NULL) {
		// default encoding - UTF-8
		// Write plain text data to file
		ssize_t bytesWritten = file->Write(text, textLength);
		if (bytesWritten != textLength) {
			if (bytesWritten < B_OK)
				return bytesWritten;

			return B_ERROR;
		}

		// be:encoding, defaults to UTF-8 (65535)
		// Note that the B_UNICODE_UTF8 constant is 0 and for some reason
		// not appropriate for use here.
		int32 value = 65535;
		file->WriteAttr("be:encoding", B_INT32_TYPE, 0, &value, sizeof(value));
	} else {
		// we need to convert the text
		uint32 id = characterSet->GetConversionID();
		const char* outText = view->Text();
		int32 sourceLength = textLength;
		int32 state = 0;

		textLength = 0;

		do {
			char buffer[32768];
			int32 length = sourceLength;
			int32 bufferSize = sizeof(buffer);
			status = convert_from_utf8(id, outText, &length, buffer, &bufferSize, &state);
			if (status != B_OK)
				return status;

			ssize_t bytesWritten = file->Write(buffer, bufferSize);
			if (bytesWritten < B_OK)
				return bytesWritten;

			sourceLength -= length;
			textLength += bytesWritten;
			outText += length;
		} while (sourceLength > 0);

		BString encodingStr(encoding);
		file->WriteAttrString("be:encoding", &encodingStr);
	}

	// truncate any extra text
	status = file->SetSize(textLength);
	if (status != B_OK)
		return status;

	// Write attributes. We don't report an error anymore after this point,
	// as attributes aren't that crucial - not all volumes support attributes.
	// However, if writing one attribute fails, no further attributes are
	// tried to be written.

	BNodeInfo info(file);
	char type[B_MIME_TYPE_LENGTH];
	if (info.GetType(type) != B_OK) {
		// This file doesn't have a file type yet, so let's set it
		if (info.SetType("text/plain") < B_OK)
			return B_OK;
	}

	// word wrap setting, turned on by default
	int32 wordWrap = view->DoesWordWrap() ? 1 : 0;
	ssize_t bytesWritten = file->WriteAttr("wrap", B_INT32_TYPE, 0,
		&wordWrap, sizeof(int32));
	if (bytesWritten != sizeof(int32))
		return B_OK;

	// alignment, default is B_ALIGN_LEFT
	int32 alignment = view->Alignment();
	bytesWritten = file->WriteAttr("alignment", B_INT32_TYPE, 0,
		&alignment, sizeof(int32));
	if (bytesWritten != sizeof(int32))
		return B_OK;

	// Write text_run_array, ie. the styles of the text

	text_run_array *runArray = view->RunArray(0, view->TextLength());
	if (runArray != NULL) {
		int32 runArraySize = 0;
		void *flattenedRunArray = BTextView::FlattenRunArray(runArray, &runArraySize);
		if (flattenedRunArray != NULL) {
			file->WriteAttr("styles", B_RAW_TYPE, 0, flattenedRunArray,
				runArraySize);
		}

		free(flattenedRunArray);
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		BTextView::FreeRunArray(runArray);
#else
		free(runArray);
#endif
	}

	return B_OK;
}


status_t
BTranslationUtils::WriteStyledEditFile(BTextView* view, BFile* file)
{
	return WriteStyledEditFile(view, file, NULL);
}


/*!
	Each translator can have default settings, set by the
	"translations" control panel. You can read these settings to
	pass on to a translator using one of these functions.

	\param forTranslator, the translator the settings are for
		roster, the roster used to get the settings

	\return BMessage of configuration data for forTranslator - you own
		this message and have to free it when you're done with it.
	\return NULL, if anything went wrong
*/
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

	BMessage *message = new BMessage();
	if (message == NULL)
		return NULL;

	status_t result = roster->GetConfigurationMessage(forTranslator, message);
	if (result != B_OK && result != B_NO_TRANSLATOR) {
		// Be's version seems to just pass an empty BMessage
		// in case of B_NO_TRANSLATOR, well, in some cases anyway
		delete message;
		return NULL;
	}

	return message;
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

	BObjectList<translator_info> infoList;

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

		// Get supported output formats
		err = roster->GetOutputFormats(ids[tix], &formats, &numFormats);
		if (err == B_OK) {
			for (int oix = 0; oix < numFormats; oix++) {
				if (formats[oix].type != fromType) {
					infoList.AddItem(_BuildTranslatorInfo(ids[tix],
						const_cast<translation_format*>(&formats[oix])));
				}
			}
		}
	}

	// Sort alphabetically by name
	infoList.SortItems(&_CompareTranslatorInfoByName);

	// Now add the menu items
	for (int i = 0; i < infoList.CountItems(); i++) {
		translator_info* info = infoList.ItemAt(i);

		BMessage *itemmsg;
		if (kModel)
			itemmsg = new BMessage(*kModel);
		else
			itemmsg = new BMessage(B_TRANSLATION_MENU);
		itemmsg->AddInt32(kTranslatorIdName, info->translator);
		itemmsg->AddInt32(kTranslatorTypeName, info->type);
		intoMenu->AddItem(new BMenuItem(info->name, itemmsg));

		// Delete object created in _BuildTranslatorInfo
		delete info;
	}

	delete[] ids;
	return B_OK;
}


translator_info*
BTranslationUtils::_BuildTranslatorInfo(const translator_id id, const translation_format* format)
{
	// Caller must delete
	translator_info* info = new translator_info;

	info->translator = id;
	info->type = format->type;
	info->group = format->group;
	info->quality = format->quality;
	info->capability = format->capability;
	strlcpy(info->name, format->name, sizeof(info->name));
	strlcpy(info->MIME, format->MIME, sizeof(info->MIME));

	return info;
}


int
BTranslationUtils::_CompareTranslatorInfoByName(const translator_info* info1, const translator_info* info2)
{
	return strcasecmp(info1->name, info2->name);
}
