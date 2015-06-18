/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_PARSER_H
#define SETTINGS_PARSER_H


#include <Message.h>


class SettingsParser {
public:
								SettingsParser();

			status_t			ParseFile(const char* path, BMessage& settings);

#ifdef TEST_HAIKU
			status_t			Parse(const char* text, BMessage& settings);
#endif
};


#endif // SETTINGS_PARSER_H
