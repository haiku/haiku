/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Copyright 2014, Augustin Cavalier (waddlesplash)
 * Distributed under the terms of the MIT License.
 */
#ifndef _JSON_H
#define _JSON_H


#include "JsonEventListener.h"

#include <Message.h>
#include <String.h>


namespace BPrivate {

class JsonParseContext;

class BJson {

public:
	static	status_t			Parse(const char* JSON, BMessage& message);
	static	status_t			Parse(const BString& JSON, BMessage& message);
	static	void				Parse(BDataIO* data,
									BJsonEventListener* listener);

private:
	static	bool				NextChar(JsonParseContext& jsonParseContext,
									char* c);
	static	bool				NextNonWhitespaceChar(
									JsonParseContext& jsonParseContext,
									char* c);

	static	bool				ParseAny(JsonParseContext& jsonParseContext);
	static	bool				ParseObjectNameValuePair(
									JsonParseContext& jsonParseContext);
	static	bool				ParseObject(JsonParseContext& jsonParseContext);
	static	bool				ParseArray(JsonParseContext& jsonParseContext);
	static	bool				ParseEscapeUnicodeSequence(
									JsonParseContext& jsonParseContext,
									BString& stringResult);
	static	bool				ParseStringEscapeSequence(
									JsonParseContext& jsonParseContext,
									BString& stringResult);
	static	bool				ParseString(JsonParseContext& jsonParseContext,
									json_event_type eventType);
	static	bool				ParseExpectedVerbatimStringAndRaiseEvent(
									JsonParseContext& jsonParseContext,
									const char* expectedString,
									size_t expectedStringLength,
									char leadingChar,
									json_event_type jsonEventType);
	static	bool				ParseExpectedVerbatimString(
									JsonParseContext& jsonParseContext,
									const char* expectedString,
									size_t expectedStringLength,
									char leadingChar);

	static bool					IsValidNumber(BString& number);
	static bool					ParseNumber(JsonParseContext& jsonParseContext);
};

} // namespace BPrivate

using BPrivate::BJson;

#endif	// _JSON_H
