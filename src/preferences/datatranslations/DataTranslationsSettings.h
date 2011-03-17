/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 */
#ifndef DATA_TRANSLATIONS_SETTINGS_H
#define DATA_TRANSLATIONS_SETTINGS_H


#include <Point.h>


class DataTranslationsSettings {
public:
							DataTranslationsSettings();
							~DataTranslationsSettings();

			BPoint			WindowCorner() const { return fCorner; }
			void			SetWindowCorner(BPoint corner);

	static DataTranslationsSettings*	Instance();

private:
			BPoint			fCorner;
};


#endif	// DATA_TRANSLATIONS_SETTINGS_H
