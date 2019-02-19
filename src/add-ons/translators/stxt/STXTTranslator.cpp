/*
 * Copyright 2002-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "STXTTranslator.h"
#include "STXTView.h"

#include <Catalog.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <MimeType.h>
#include <String.h>
#include <TextEncoding.h>
#include <UTF8.h>

#include <algorithm>
#include <new>
#include <string.h>
#include <stdio.h>
#include <stdint.h>


using namespace BPrivate;
using namespace std;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "STXTTranslator"

#define READ_BUFFER_SIZE 32768
#define DATA_BUFFER_SIZE 2048

// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
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

// ---------------------------------------------------------------
// make_nth_translator
//
// Creates a STXTTranslator object to be used by BTranslatorRoster
//
// Preconditions:
//
// Parameters: n,		The translator to return. Since
//						STXTTranslator only publishes one
//						translator, it only returns a
//						STXTTranslator if n == 0
//
//             you, 	The image_id of the add-on that
//						contains code (not used).
//
//             flags,	Has no meaning yet, should be 0.
//
// Postconditions:
//
// Returns: NULL if n is not zero,
//          a new STXTTranslator if n is zero
// ---------------------------------------------------------------
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new (std::nothrow) STXTTranslator();

	return NULL;
}


//	#pragma mark -


/*!
	Determines if the data in inSource is of the STXT format.

	\param header the STXT stream header read in by Identify() or Translate()
	\param inSource the stream with the STXT data
	\param outInfo information about the type of data from inSource is stored here
	\param outType the desired output type for the data in inSource
	\param ptxtheader if this is not NULL, the TEXT header from
		inSource is copied to it
*/
status_t
identify_stxt_header(const TranslatorStyledTextStreamHeader &header,
	BPositionIO *inSource, translator_info *outInfo, uint32 outType,
	TranslatorStyledTextTextHeader *ptxtheader = NULL)
{
	const ssize_t ktxtsize = sizeof(TranslatorStyledTextTextHeader);
	const ssize_t kstylsize = sizeof(TranslatorStyledTextStyleHeader);

	uint8 buffer[max(ktxtsize, kstylsize)];

	// Check the TEXT header
	TranslatorStyledTextTextHeader txtheader;
	if (inSource->Read(buffer, ktxtsize) != ktxtsize)
		return B_NO_TRANSLATOR;

	memcpy(&txtheader, buffer, ktxtsize);
	if (swap_data(B_UINT32_TYPE, &txtheader, ktxtsize,
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;

	if (txtheader.header.magic != 'TEXT'
		|| txtheader.header.header_size != sizeof(TranslatorStyledTextTextHeader)
		|| txtheader.charset != B_UNICODE_UTF8)
		return B_NO_TRANSLATOR;

	// skip the text data
	off_t seekresult, pos;
	pos = header.header.header_size + txtheader.header.header_size
		+ txtheader.header.data_size;
	seekresult = inSource->Seek(txtheader.header.data_size,
		SEEK_CUR);
	if (seekresult < pos)
		return B_NO_TRANSLATOR;
	if (seekresult > pos)
		return B_ERROR;

	// check the STYL header (not all STXT files have this)
	ssize_t read = 0;
	TranslatorStyledTextStyleHeader stylheader;
	read = inSource->Read(buffer, kstylsize);
	if (read < 0)
		return read;
	if (read != kstylsize && read != 0)
		return B_NO_TRANSLATOR;

	// If there is a STYL header
	if (read == kstylsize) {
		memcpy(&stylheader, buffer, kstylsize);
		if (swap_data(B_UINT32_TYPE, &stylheader, kstylsize,
			B_SWAP_BENDIAN_TO_HOST) != B_OK)
			return B_ERROR;

		if (stylheader.header.magic != 'STYL'
			|| stylheader.header.header_size !=
				sizeof(TranslatorStyledTextStyleHeader))
			return B_NO_TRANSLATOR;
	}

	// if output TEXT header is supplied, fill it with data
	if (ptxtheader) {
		ptxtheader->header.magic = txtheader.header.magic;
		ptxtheader->header.header_size = txtheader.header.header_size;
		ptxtheader->header.data_size = txtheader.header.data_size;
		ptxtheader->charset = txtheader.charset;
	}

	// return information about the data in the stream
	outInfo->type = B_STYLED_TEXT_FORMAT;
	outInfo->group = B_TRANSLATOR_TEXT;
	outInfo->quality = STXT_IN_QUALITY;
	outInfo->capability = STXT_IN_CAPABILITY;
	strlcpy(outInfo->name, B_TRANSLATE("Be styled text file"),
		sizeof(outInfo->name));
	strcpy(outInfo->MIME, "text/x-vnd.Be-stxt");

	return B_OK;
}


/*!
	Determines if the data in \a inSource is of the UTF8 plain

	\param data buffer containing data already read (must be at
		least DATA_BUFFER_SIZE bytes large)
	\param nread number of bytes that have already been read from the stream
	\param header the STXT stream header read in by Identify() or Translate()
	\param inSource the stream with the STXT data
	\param outInfo information about the type of data from inSource is stored here
	\param outType the desired output type for the data in inSource
*/
status_t
identify_text(uint8* data, int32 bytesRead, BPositionIO* source,
	translator_info* outInfo, uint32 outType, const char*& encoding)
{
	ssize_t readLater = source->Read(data + bytesRead, DATA_BUFFER_SIZE - bytesRead);
	if (readLater < B_OK)
		return B_NO_TRANSLATOR;

	bytesRead += readLater;

	BPrivate::BTextEncoding textEncoding((char*)data, (size_t)bytesRead);
	encoding = textEncoding.GetName();
	if (strlen(encoding) == 0) {
		/* No valid character encoding found! */
		return B_NO_TRANSLATOR;
	}

	float capability = TEXT_IN_CAPABILITY;
	if (bytesRead < 20)
		capability = .1f;

	// return information about the data in the stream
	outInfo->type = B_TRANSLATOR_TEXT;
	outInfo->group = B_TRANSLATOR_TEXT;
	outInfo->quality = TEXT_IN_QUALITY;
	outInfo->capability = capability;

	strlcpy(outInfo->name, B_TRANSLATE("Plain text file"),
		sizeof(outInfo->name));

	//strlcpy(outInfo->MIME, type.Type(), sizeof(outInfo->MIME));
	strcpy(outInfo->MIME, "text/plain");
	return B_OK;
}


// ---------------------------------------------------------------
// translate_from_stxt
//
// Translates the data in inSource to the type outType and stores
// the translated data in outDestination.
//
// Preconditions:
//
// Parameters:	inSource,	the data to be translated
//
//				outDestination,	where the translated data is
//								put
//
//				outType,	the type to convert inSource to
//
//				txtheader, 	the TEXT header from inSource
//
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if outType is invalid
//
// B_NO_TRANSLATOR, if this translator doesn't understand the data
//
// B_ERROR, if there was an error allocating memory or converting
//          data
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
translate_from_stxt(BPositionIO *inSource, BPositionIO *outDestination,
		uint32 outType, const TranslatorStyledTextTextHeader &txtheader)
{
	if (inSource->Seek(0, SEEK_SET) != 0)
		return B_ERROR;

	const ssize_t kstxtsize = sizeof(TranslatorStyledTextStreamHeader);
	const ssize_t ktxtsize = sizeof(TranslatorStyledTextTextHeader);

	bool btoplain;
	if (outType == B_TRANSLATOR_TEXT)
		btoplain = true;
	else if (outType == B_STYLED_TEXT_FORMAT)
		btoplain = false;
	else
		return B_BAD_VALUE;

	uint8 buffer[READ_BUFFER_SIZE];
	ssize_t nread = 0, nwritten = 0, nreed = 0, ntotalread = 0;

	// skip to the actual text data when outputting a
	// plain text file
	if (btoplain) {
		if (inSource->Seek(kstxtsize + ktxtsize, SEEK_CUR) !=
			kstxtsize + ktxtsize)
			return B_ERROR;
	}

	// Read data from inSource
	// When outputing B_TRANSLATOR_TEXT, the loop stops when all of
	// the text data has been read and written.
	// When outputting B_STYLED_TEXT_FORMAT, the loop stops when all
	// of the data from inSource has been read and written.
	if (btoplain)
		nreed = min((size_t)READ_BUFFER_SIZE,
			(size_t)txtheader.header.data_size - ntotalread);
	else
		nreed = READ_BUFFER_SIZE;
	nread = inSource->Read(buffer, nreed);
	while (nread > 0) {
		nwritten = outDestination->Write(buffer, nread);
		if (nwritten != nread)
			return B_ERROR;

		if (btoplain) {
			ntotalread += nread;
			nreed = min((size_t)READ_BUFFER_SIZE,
				(size_t)txtheader.header.data_size - ntotalread);
		} else
			nreed = READ_BUFFER_SIZE;
		nread = inSource->Read(buffer, nreed);
	}

	if (btoplain && static_cast<ssize_t>(txtheader.header.data_size) !=
		ntotalread)
		// If not all of the text data was able to be read...
		return B_NO_TRANSLATOR;
	else
		return B_OK;
}

// ---------------------------------------------------------------
// output_headers
//
// Outputs the Stream and Text headers from the B_STYLED_TEXT_FORMAT
// to outDestination, setting the data_size member of the text header
// to text_data_size
//
// Preconditions:
//
// Parameters:	outDestination,	where the translated data is
//								put
//
//				text_data_size, number of bytes in data section
//							    of the TEXT header
//
//
// Postconditions:
//
// Returns:
//
// B_ERROR, if there was an error writing to outDestination or
// 	an error with converting the byte order
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
output_headers(BPositionIO *outDestination, uint32 text_data_size)
{
	const int32 kHeadersSize = sizeof(TranslatorStyledTextStreamHeader) +
		sizeof(TranslatorStyledTextTextHeader);
	status_t result;
	TranslatorStyledTextStreamHeader stxtheader;
	TranslatorStyledTextTextHeader txtheader;

	uint8 buffer[kHeadersSize];

	stxtheader.header.magic = 'STXT';
	stxtheader.header.header_size = sizeof(TranslatorStyledTextStreamHeader);
	stxtheader.header.data_size = 0;
	stxtheader.version = 100;
	memcpy(buffer, &stxtheader, stxtheader.header.header_size);

	txtheader.header.magic = 'TEXT';
	txtheader.header.header_size = sizeof(TranslatorStyledTextTextHeader);
	txtheader.header.data_size = text_data_size;
	txtheader.charset = B_UNICODE_UTF8;
	memcpy(buffer + stxtheader.header.header_size, &txtheader,
		txtheader.header.header_size);

	// write out headers in Big Endian byte order
	result = swap_data(B_UINT32_TYPE, buffer, kHeadersSize,
		B_SWAP_HOST_TO_BENDIAN);
	if (result == B_OK) {
		ssize_t nwritten = 0;
		nwritten = outDestination->Write(buffer, kHeadersSize);
		if (nwritten != kHeadersSize)
			return B_ERROR;
		else
			return B_OK;
	}

	return result;
}

// ---------------------------------------------------------------
// output_styles
//
// Writes out the actual style information into outDestination
// using the data from pflatRunArray
//
// Preconditions:
//
// Parameters:	outDestination,	where the translated data is
//								put
//
//				text_size,		size in bytes of the text in
//								outDestination
//
//				data_size,		size of pflatRunArray
//
// Postconditions:
//
// Returns:
//
// B_ERROR, if there was an error writing to outDestination or
// 	an error with converting the byte order
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
output_styles(BPositionIO *outDestination, uint32 text_size,
	uint8 *pflatRunArray, ssize_t data_size)
{
	const ssize_t kstylsize = sizeof(TranslatorStyledTextStyleHeader);

	uint8 buffer[kstylsize];

	// output STYL header
	TranslatorStyledTextStyleHeader stylheader;
	stylheader.header.magic = 'STYL';
	stylheader.header.header_size =
		sizeof(TranslatorStyledTextStyleHeader);
	stylheader.header.data_size = data_size;
	stylheader.apply_offset = 0;
	stylheader.apply_length = text_size;

	memcpy(buffer, &stylheader, kstylsize);
	if (swap_data(B_UINT32_TYPE, buffer, kstylsize,
		B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	if (outDestination->Write(buffer, kstylsize) != kstylsize)
		return B_ERROR;

	// output actual style information
	if (outDestination->Write(pflatRunArray,
		data_size) != data_size)
		return B_ERROR;

	return B_OK;
}


/*!
	Convert the plain text (UTF8) from inSource to plain or
	styled text in outDestination
*/
status_t
translate_from_text(BPositionIO* source, const char* outEncoding, bool forceEncoding,
	BPositionIO* destination, uint32 outType)
{
	if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT)
		return B_BAD_VALUE;

	// find the length of the text
	off_t size = source->Seek(0, SEEK_END);
	if (size < 0)
		return (status_t)size;
	if (size > UINT32_MAX && outType == B_STYLED_TEXT_FORMAT)
		return B_NOT_SUPPORTED;

	status_t status = source->Seek(0, SEEK_SET);
	if (status < B_OK)
		return status;

	if (outType == B_STYLED_TEXT_FORMAT) {
		// output styled text headers
		status = output_headers(destination, (uint32)size);
		if (status != B_OK)
			return status;
	}

	class MallocBuffer {
		public:
			MallocBuffer() : fBuffer(NULL), fSize(0) {}
			~MallocBuffer() { free(fBuffer); }

			void* Buffer() { return fBuffer; }
			size_t Size() const { return fSize; }

			status_t
			Allocate(size_t size)
			{
				fBuffer = malloc(size);
				if (fBuffer != NULL) {
					fSize = size;
					return B_OK;
				}
				return B_NO_MEMORY;
			}

		private:
			void*	fBuffer;
			size_t	fSize;
	} encodingBuffer;

	BNode* node = dynamic_cast<BNode*>(source);
	BString encoding(outEncoding);
	if (node != NULL) {
		// determine encoding, if available
		bool hasAttribute = false;
		if (encoding.String() && !forceEncoding) {
			attr_info info;
			node->GetAttrInfo("be:encoding", &info);

			if ((info.type == B_STRING_TYPE) && (node->ReadAttrString(
					"be:encoding", &encoding) == B_OK)) {
				hasAttribute = true;
			} else if (info.type == B_INT32_TYPE) {
				// Try the BeOS version of the atribute, which used an int32
				// and a well-known list of encodings.
				int32 value;
				ssize_t bytesRead = node->ReadAttr("be:encoding", B_INT32_TYPE, 0,
					&value, sizeof(value));
				if (bytesRead == (ssize_t)sizeof(value)) {
					if (value != 65535) {
						const BCharacterSet* characterSet
							= BCharacterSetRoster::GetCharacterSetByConversionID(value);
						if (characterSet != NULL)
							encoding = characterSet->GetName();
					}
				}
			}
		} else {
			hasAttribute = true;
				// we don't write the encoding in this case
		}

		if (!encoding.IsEmpty())
			encodingBuffer.Allocate(READ_BUFFER_SIZE * 4);

		if (!hasAttribute && !encoding.IsEmpty()) {
			// add encoding attribute, so that someone opening the file can
			// retrieve it for persistance
			node->WriteAttrString("be:encoding", &encoding);
		}
	}

	off_t outputSize = 0;
	ssize_t bytesRead;

	BPrivate::BTextEncoding codec(encoding.String());

	// output the actual text part of the data
	do {
		uint8 buffer[READ_BUFFER_SIZE];
		bytesRead = source->Read(buffer, READ_BUFFER_SIZE);
		if (bytesRead < B_OK)
			return bytesRead;
		if (bytesRead == 0)
			break;

		if (encodingBuffer.Size() == 0) {
			// default, no encoding
			ssize_t bytesWritten = destination->Write(buffer, bytesRead);
			if (bytesWritten != bytesRead) {
				if (bytesWritten < B_OK)
					return bytesWritten;

				return B_ERROR;
			}

			outputSize += bytesRead;
		} else {
			// decode text file to UTF-8
			const char* pos = (char*)buffer;
			size_t encodingLength;
			int32 bytesLeft = bytesRead;
			size_t bytes;
			do {
				encodingLength = READ_BUFFER_SIZE * 4;
				bytes = bytesLeft;

				status = codec.Decode(pos, bytes,
					(char*)encodingBuffer.Buffer(), encodingLength);
				if (status < B_OK) {
					return status;
				}

				ssize_t bytesWritten = destination->Write(encodingBuffer.Buffer(),
					encodingLength);
				if (bytesWritten < (ssize_t)encodingLength) {
					if (bytesWritten < B_OK)
						return bytesWritten;

					return B_ERROR;
				}

				pos += bytes;
				bytesLeft -= bytes;
				outputSize += encodingLength;
			} while (encodingLength > 0 && bytesLeft > 0);
		}
	} while (bytesRead > 0);

	if (outType != B_STYLED_TEXT_FORMAT)
		return B_OK;

	if (encodingBuffer.Size() != 0 && size != outputSize) {
		if (outputSize > UINT32_MAX)
			return B_NOT_SUPPORTED;

		// we need to update the header as the decoded text size has changed
		status = destination->Seek(0, SEEK_SET);
		if (status == B_OK)
			status = output_headers(destination, (uint32)outputSize);
		if (status == B_OK)
			status = destination->Seek(0, SEEK_END);

		if (status < B_OK)
			return status;
	}

	// Read file attributes if outputting styled data
	// and source is a BNode object

	if (node == NULL)
		return B_OK;

	// Try to read styles - we only propagate an error if the actual on-disk
	// data is likely to be okay

	const char *kAttrName = "styles";
	attr_info info;
	if (node->GetAttrInfo(kAttrName, &info) != B_OK)
		return B_OK;

	if (info.type != B_RAW_TYPE || info.size < 160) {
		// styles seem to be broken, but since we got the text,
		// we don't propagate the error
		return B_OK;
	}

	uint8* flatRunArray = new (std::nothrow) uint8[info.size];
	if (flatRunArray == NULL)
		return B_NO_MEMORY;

	bytesRead = node->ReadAttr(kAttrName, B_RAW_TYPE, 0, flatRunArray, info.size);
	if (bytesRead != info.size)
		return B_OK;

	output_styles(destination, size, flatRunArray, info.size);

	delete[] flatRunArray;
	return B_OK;
}


//	#pragma mark -


STXTTranslator::STXTTranslator()
	: BaseTranslator(B_TRANSLATE("StyledEdit files"),
		B_TRANSLATE("StyledEdit file translator"),
		STXT_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"STXTTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_TEXT, B_STYLED_TEXT_FORMAT)
{
}


STXTTranslator::~STXTTranslator()
{
}


status_t
STXTTranslator::Identify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_TEXT;
	if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT)
		return B_NO_TRANSLATOR;

	const ssize_t kstxtsize = sizeof(TranslatorStyledTextStreamHeader);

	uint8 buffer[DATA_BUFFER_SIZE];
	status_t nread = 0;
	// Read in the header to determine
	// if the data is supported
	nread = inSource->Read(buffer, kstxtsize);
	if (nread < 0)
		return nread;

	// read in enough data to fill the stream header
	if (nread == kstxtsize) {
		TranslatorStyledTextStreamHeader header;
		memcpy(&header, buffer, kstxtsize);
		if (swap_data(B_UINT32_TYPE, &header, kstxtsize,
				B_SWAP_BENDIAN_TO_HOST) != B_OK)
			return B_ERROR;

		if (header.header.magic == B_STYLED_TEXT_FORMAT
			&& header.header.header_size == (int32)kstxtsize
			&& header.header.data_size == 0
			&& header.version == 100)
			return identify_stxt_header(header, inSource, outInfo, outType);
	}

	// if the data is not styled text, check if it is plain text
	const char* encoding;
	return identify_text(buffer, nread, inSource, outInfo, outType, encoding);
}


status_t
STXTTranslator::Translate(BPositionIO* source, const translator_info* info,
	BMessage* ioExtension, uint32 outType, BPositionIO* outDestination)
{
	if (!outType)
		outType = B_TRANSLATOR_TEXT;
	if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT)
		return B_NO_TRANSLATOR;

	const ssize_t headerSize = sizeof(TranslatorStyledTextStreamHeader);
	uint8 buffer[DATA_BUFFER_SIZE];
	status_t result;
	translator_info outInfo;
	// Read in the header to determine
	// if the data is supported
	ssize_t bytesRead = source->Read(buffer, headerSize);
	if (bytesRead < 0)
		return bytesRead;

	// read in enough data to fill the stream header
	if (bytesRead == headerSize) {
		TranslatorStyledTextStreamHeader header;
		memcpy(&header, buffer, headerSize);
		if (swap_data(B_UINT32_TYPE, &header, headerSize,
				B_SWAP_BENDIAN_TO_HOST) != B_OK)
			return B_ERROR;

		if (header.header.magic == B_STYLED_TEXT_FORMAT
			&& header.header.header_size == sizeof(TranslatorStyledTextStreamHeader)
			&& header.header.data_size == 0
			&& header.version == 100) {
			TranslatorStyledTextTextHeader textHeader;
			result = identify_stxt_header(header, source, &outInfo, outType,
				&textHeader);
			if (result != B_OK)
				return result;

			return translate_from_stxt(source, outDestination, outType, textHeader);
		}
	}

	// if the data is not styled text, check if it is ASCII text
	bool forceEncoding = false;
	const char* encoding = NULL;
	result = identify_text(buffer, bytesRead, source, &outInfo, outType, encoding);
	if (result != B_OK)
		return result;

	if (ioExtension != NULL) {
		const char* value;
		if (ioExtension->FindString("be:encoding", &value) == B_OK
			&& value[0]) {
			// override encoding
			encoding = value;
			forceEncoding = true;
		}
	}

	return translate_from_text(source, encoding, forceEncoding, outDestination, outType);
}


BView *
STXTTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new STXTView(BRect(0, 0, 225, 175),
		B_TRANSLATE("STXTTranslator Settings"),
		B_FOLLOW_ALL, B_WILL_DRAW, settings);
}

