/*
 * Copyright 2016, Haiku, inc.
 * Distributed under terms of the MIT license.
 */


#ifndef TEXTENCODING_H
#define TEXTENCODING_H


#include <String.h>

#include <stddef.h>


struct UConverter;


class TextEncoding
{
	public:
		TextEncoding(BString name);
		TextEncoding(const char* data, size_t length);

		~TextEncoding();

		status_t	InitCheck();
		BString		GetName();

		status_t Encode(const char* input, size_t& inputLength, char* output,
			size_t& outputLength);
		status_t Decode(const char* input, size_t& inputLength, char* output,
			size_t& outputLength);
		status_t Flush(char* output, size_t& outputLength);

	private:
		BString fName;

		UConverter* fUtf8Converter;
		UConverter* fConverter;
};


#endif /* TEXTENCODING_H */
