/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef RGB_COLOR_H
#define RGB_COLOR_H


#include <GraphicsDefs.h>


class RGBColor {
 public:
								RGBColor(uint8 r,
										 uint8 g,
										 uint8 b,
										 uint8 a = 255);
								RGBColor(int r,
										 int g,
										 int b,
										 int a = 255);
								RGBColor(const rgb_color& color);
								RGBColor(uint16 color);
								RGBColor(uint8 color);
								RGBColor(const RGBColor& color);
								RGBColor();

			uint8				GetColor8() const;
			uint16				GetColor15() const;
			uint16				GetColor16() const;
			rgb_color			GetColor32() const;

			void				SetColor(uint8 r,
										 uint8 g,
										 uint8 b,
										 uint8 a = 255);
			void				SetColor(int r,
										 int g,
										 int b,
										 int a = 255);
			void				SetColor(uint16 color16);
			void				SetColor(uint8 color8);
			void				SetColor(const rgb_color& color);
			void				SetColor(const RGBColor& color);

			const RGBColor&		operator=(const RGBColor& color);
			const RGBColor&		operator=(const rgb_color& color);

			bool				operator==(const rgb_color& color) const;
			bool				operator==(const RGBColor& color) const;
			bool				operator!=(const rgb_color& color) const;
			bool				operator!=(const RGBColor& color) const;

			// conversion to rgb_color
								operator rgb_color() const
									{ return fColor32; }
	
			bool				IsTransparentMagic() const;
	
			void				PrintToStream() const;

	protected:
			rgb_color			fColor32;

	// caching
	mutable	uint16				fColor16;
	mutable	uint16				fColor15;
	mutable	uint8				fColor8;
	mutable	bool				fUpdate8;
	mutable	bool				fUpdate15;
	mutable	bool				fUpdate16;
};

#endif	// RGB_COLOR_H
