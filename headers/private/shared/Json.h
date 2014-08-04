/*
 * Copyright 2014, Augustin Cavalier (waddlesplash)
 * Distributed under the terms of the MIT License.
 */
#ifndef JSON_H
#define JSON_H

#include <Message.h>
#include <String.h>

namespace BPrivate {

class BJson {
public:
			enum JsonObjectType {
				JSON_TYPE_MAP = '_JTM',
				JSON_TYPE_ARRAY = '_JTA'
			};

public:
	static	status_t			Parse(BMessage& message, const char* JSON);
	static	status_t			Parse(BMessage& message, BString& JSON);

private:
	static	BString				_Parser_ParseString(BString& JSON, int32& pos);
	static	double				_Parser_ParseNumber(BString& JSON, int32& pos);
	static	bool				_Parser_ParseConstant(BString& JSON, int32& pos,
													  const char* constant);
};

} // namespace BPrivate

#endif	// JSON_H
