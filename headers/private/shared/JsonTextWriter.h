/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef _JSON_STRING_STREAM_WRITER_H
#define _JSON_STRING_STREAM_WRITER_H


#include "JsonWriter.h"

#include <DataIO.h>
#include <String.h>


namespace BPrivate {

class BJsonTextWriterStackedEventListener;

class BJsonTextWriter : public BJsonWriter {
friend class BJsonTextWriterStackedEventListener;
public:
								BJsonTextWriter(BDataIO* dataIO);
		virtual					~BJsonTextWriter();

			bool				Handle(const BJsonEvent& event);
			void				Complete();

private:
			void				SetStackedListener(
									BJsonTextWriterStackedEventListener*
									stackedListener);

			status_t			StreamNumberNode(const BJsonEvent& event);

			status_t			StreamStringVerbatim(const char* string);
			status_t			StreamStringVerbatim(const char* string,
									off_t offset, size_t length);

			status_t			StreamStringEncoded(const char* string);
			status_t			StreamStringEncoded(const char* string,
									off_t offset, size_t length);
			status_t			StreamStringUnicodeCharacter(uint32 c);

			status_t			StreamQuotedEncodedString(const char* string);
			status_t			StreamQuotedEncodedString(const char* string,
									off_t offset, size_t length);

			status_t			StreamChar(char c);

			BDataIO*			fDataIO;
			BJsonTextWriterStackedEventListener*
								fStackedListener;

			char				fUnicodeAssemblyBuffer[7];

};

} // namespace BPrivate

using BPrivate::BJsonTextWriter;

#endif	// _JSON_STRING_STREAM_WRITER_H
