/*
 * Copyright 2016, Haiku, inc.
 * Distributed under terms of the MIT license.
 */


#ifndef TEXTENCODING_H
#define TEXTENCODING_H


#include <String.h>

#include <stddef.h>


class TextEncoding
{
	public:
		TextEncoding(const char* data, size_t length);

		BString GetName();

	private:
		BString fName;
};


#endif /* !TEXTENCODING_H */
