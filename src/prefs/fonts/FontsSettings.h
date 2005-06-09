/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef FONTS_SETTINGS_H
#define FONTS_SETTINGS_H

class FontsSettings
{
public:
			FontsSettings(void);
			~FontsSettings(void);
		
	BPoint	WindowCorner(void) const { return fCorner; }
	void	SetWindowCorner(BPoint);
	
	void	SetDefaults(void);
	
private:
	BPoint fCorner;
};

#endif
