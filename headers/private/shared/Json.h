/*
 * Copyright 2014, Augustin Cavalier (waddlesplash)
 * Distributed under the terms of the MIT License.
 */
#ifndef _JSON_H
#define _JSON_H

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
	static	status_t			Parse(const char* JSON, BMessage& message);
	static	status_t			Parse(const BString& JSON, BMessage& message);

private:
	static	void				_Parse(const BString& JSON, BMessage& message);
	static	BString				_ParseString(const BString& JSON, int32& pos);
	static	double				_ParseNumber(const BString& JSON, int32& pos);
	static	bool				_ParseConstant(const BString& JSON, int32& pos,
									 const char* constant);
};

} // namespace BPrivate

using BPrivate::BJson;

#endif	// _JSON_H
