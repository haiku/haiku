#ifndef RGBCOLOR_H_
#define RGBCOLOR_H_

#include <GraphicsDefs.h>

class RGBColor
{
public:
	RGBColor(uint8 r, uint8 g, uint8 b, uint8 a=255);
	RGBColor(rgb_color col);
	RGBColor(uint16 col);
	RGBColor(uint8 col);
	RGBColor(const RGBColor &col);
	RGBColor(void);
	void PrintToStream(void);
	uint8 GetColor8(void);
	uint16 GetColor16(void);
	rgb_color GetColor32(void);
	void SetColor(uint8 r, uint8 g, uint8 b, uint8 a=255);
	void SetColor(uint16 color16);
	void SetColor(uint8 color8);
	void SetColor(rgb_color color);
	void SetColor(const RGBColor &col);
	RGBColor MakeBlendColor(RGBColor color, float position);
	RGBColor & operator=(const RGBColor &col);
	RGBColor & operator=(rgb_color col);
protected:
	rgb_color color32;
	uint16 color16;
	uint8 color8;
};

#endif