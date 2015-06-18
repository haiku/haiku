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
};


#endif // SETTINGS_PARSER_H
