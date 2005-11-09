/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONTS_SETTINGS_H
#define FONTS_SETTINGS_H


#include <Point.h>


class FontsSettings {
	public:
		FontsSettings();
		~FontsSettings();

		BPoint	WindowCorner() const { return fCorner; }
		void	SetWindowCorner(BPoint);

		void	SetDefaults();

	private:
		BPoint	fCorner;
};

#endif	/* FONTS_SETTINGS_H */
