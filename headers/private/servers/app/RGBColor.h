//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		RGBColor.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Color encapsulation class for the app_server
//  
//------------------------------------------------------------------------------
#ifndef RGBCOLOR_H_
#define RGBCOLOR_H_

#include <GraphicsDefs.h>

/*!
	\class RGBColor RGBColor.h
	\brief A color class to encapsulate color space ugliness in the app_server
	
	RGBColors can be used to perform tasks much more difficult using rgb_colors. This
	class is limited to the app_server because of the access to the system palette for
	looking up the 32-bit value to each color index.
*/

class RGBColor
{
public:
	RGBColor(uint8 r, uint8 g, uint8 b, uint8 a=255);
	RGBColor(int r, int g, int b, int a=255);
	RGBColor(const rgb_color &col);
	RGBColor(uint16 col);
	RGBColor(uint8 col);
	RGBColor(const RGBColor &col);
	RGBColor(void);
	
	void PrintToStream(void) const;
	
	uint8 GetColor8(void);
	uint16 GetColor15(void);
	uint16 GetColor16(void);
	rgb_color GetColor32(void) const;
	
	void SetColor(uint8 r, uint8 g, uint8 b, uint8 a=255);
	void SetColor(int r, int g, int b, int a=255);
	void SetColor(uint16 color16);
	void SetColor(uint8 color8);
	void SetColor(const rgb_color &color);
	void SetColor(const RGBColor &col);
	
	RGBColor MakeBlendColor(const RGBColor &color, const float &position);
	
	const RGBColor & operator=(const RGBColor &col);
	const RGBColor & operator=(const rgb_color &col);
	bool operator==(const rgb_color &col);
	bool operator==(const RGBColor &col);
protected:
	rgb_color color32;
	uint16 color16;
	uint16 color15;
	uint8 color8;
	bool update8,update15,update16;
};

#endif
