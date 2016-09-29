/*
 * Copyright 2016, Haiku, inc.
 * Distributed under terms of the MIT license.
 */


#include "TextEncoding.h"

#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>

#include <algorithm>


namespace BPrivate {


BTextEncoding::BTextEncoding(BString name)
	:
	fName(name),
	fUtf8Converter(NULL),
	fConverter(NULL)
{
}


BTextEncoding::BTextEncoding(const char* data, size_t length)
	:
	fUtf8Converter(NULL),
	fConverter(NULL)
{
	UErrorCode error = U_ZERO_ERROR;

	UCharsetDetector* detector = ucsdet_open(&error);
	ucsdet_setText(detector, data, length, &error);
	const UCharsetMatch* encoding = ucsdet_detect(detector, &error);

	fName = ucsdet_getName(encoding, &error);
	ucsdet_close(detector);
}


BTextEncoding::~BTextEncoding()
{
	if (fUtf8Converter != NULL)
		ucnv_close(fUtf8Converter);

	if (fConverter != NULL)
		ucnv_close(fConverter);
}


status_t
BTextEncoding::InitCheck()
{
	if (fName.IsEmpty())
		return B_NO_INIT;
	else
		return B_OK;
}


status_t
BTextEncoding::Decode(const char* input, size_t& inputLength, char* output,
	size_t& outputLength)
{
	const char* base = input;
	char* target = output;

	// Optimize the easy case.
	// Note: we don't check the input to be valid UTF-8 when doing that.
	if (fName == "UTF-8") {
		outputLength = std::min(inputLength, outputLength);
		inputLength = outputLength;
		memcpy(output, input, inputLength);
		return B_OK;
	}

	UErrorCode error = U_ZERO_ERROR;

	if (fUtf8Converter == NULL)
		fUtf8Converter = ucnv_open("UTF-8", &error);

	if (fConverter == NULL)
		fConverter = ucnv_open(fName.String(), &error);

	ucnv_convertEx(fUtf8Converter, fConverter, &target, output + outputLength,
		&base, input + inputLength, NULL, NULL, NULL, NULL, FALSE, TRUE,
		&error);

	// inputLength is set to the number of bytes consumed. We may not use all of
	// the input data (for example if it is cut in the middle of an utf-8 char).
	inputLength = base - input;
	outputLength = target - output;

	if (!U_SUCCESS(error))
		return B_ERROR;

	return B_OK;
}


status_t
BTextEncoding::Encode(const char* input, size_t& inputLength, char* output,
	size_t& outputLength)
{
	const char* base = input;
	char* target = output;

	// Optimize the easy case.
	// Note: we don't check the input to be valid UTF-8 when doing that.
	if (fName == "UTF-8") {
		outputLength = std::min(inputLength, outputLength);
		inputLength = outputLength;
		memcpy(output, input, inputLength);
		return B_OK;
	}

	UErrorCode error = U_ZERO_ERROR;

	if (fUtf8Converter == NULL)
		fUtf8Converter = ucnv_open("UTF-8", &error);

	if (fConverter == NULL)
		fConverter = ucnv_open(fName.String(), &error);

	ucnv_convertEx(fConverter, fUtf8Converter, &target, output + outputLength,
		&base, input + inputLength, NULL, NULL, NULL, NULL, FALSE, TRUE,
		&error);

	// inputLength is set to the number of bytes consumed. We may not use all of
	// the input data (for example if it is cut in the middle of an utf-8 char).
	inputLength = base - input;
	outputLength = target - output;

	if (!U_SUCCESS(error))
		return B_ERROR;

	return B_OK;
}


status_t
BTextEncoding::Flush(char* output, size_t& outputLength)
{
	char* target = output;

	if (fName == "UTF-8")
		return B_OK;

	if (fUtf8Converter == NULL || fConverter == NULL)
		return B_NO_INIT;

	UErrorCode error = U_ZERO_ERROR;

	ucnv_convertEx(fConverter, fUtf8Converter, &target, output + outputLength,
		NULL, NULL, NULL, NULL, NULL, NULL, FALSE, TRUE,
		&error);

	if (!U_SUCCESS(error))
		return B_ERROR;

	return B_OK;
}


BString
BTextEncoding::GetName()
{
	return fName;
}


};
