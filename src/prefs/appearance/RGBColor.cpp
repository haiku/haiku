/*
	RGBColor.cpp
		Color encapsulation class for app_server.
*/

#include <stdio.h>
#include "RGBColor.h"

RGBColor::RGBColor(uint8 r, uint8 g, uint8 b, uint8 a=255)
{
	SetColor(r,g,b,a);
}

RGBColor::RGBColor(rgb_color col)
{
	SetColor(col);
}

RGBColor::RGBColor(uint16 col)
{
	SetColor(col);
}

RGBColor::RGBColor(uint8 col)
{
	SetColor(col);
}

RGBColor::RGBColor(const RGBColor &col)
{
	color32=col.color32;
	color16=col.color16;
	color8=col.color8;
}

RGBColor::RGBColor(void)
{
	SetColor(0,0,0,0);
}

uint8 RGBColor::GetColor8(void)
{
	return color8;
}

uint16 RGBColor::GetColor16(void)
{
	return color16;
}

rgb_color RGBColor::GetColor32(void)
{
	return color32;
}

void RGBColor::SetColor(uint8 r, uint8 g, uint8 b, uint8 a=255)
{
	color32.red=r;
	color32.green=g;
	color32.blue=b;
	color32.alpha=a;
}

void RGBColor::SetColor(uint16 col16)
{
	// Pared-down version from what is used in the app_server to
	// eliminate some dependencies
	color16=col16;
}

void RGBColor::SetColor(uint8 col8)
{
	// Pared-down version from what is used in the app_server to
	// eliminate some dependencies
	color8=col8;
}

void RGBColor::SetColor(rgb_color color)
{
	// Pared-down version from what is used in the app_server to
	// eliminate some dependencies
	color32=color;
}

void RGBColor::SetColor(const RGBColor &col)
{
	color32=col.color32;
	color16=col.color16;
	color8=col.color8;
}


RGBColor & RGBColor::operator=(const RGBColor &col)
{
	color32=col.color32;
	color16=col.color16;
	color8=col.color8;
	return *this;
}

RGBColor & RGBColor::operator=(rgb_color col)
{
	color32=col;
	return *this;
}

RGBColor RGBColor::MakeBlendColor(RGBColor color, float position)
{
	rgb_color col=color32;
	rgb_color col2=color.color32;
	
	rgb_color newcol={0,0,0,0};
	float mod=0;
	int16 delta;
	if(position<0 || position>1)
		return newcol;

	delta=int16(col2.red)-int16(col.red);
	mod=col.red + (position * delta);
	newcol.red=uint8(mod);
	if(mod>255 )
		newcol.red=255;
	if(mod<0 )
		newcol.red=0;

	delta=int16(col2.green)-int16(col.green);
	mod=col.green + (position * delta);
	newcol.green=uint8(mod);
	if(mod>255 )
		newcol.green=255;
	if(mod<0 )
		newcol.green=0;

	delta=int16(col2.blue)-int16(col.blue);
	mod=col.blue + (position * delta);
	newcol.blue=uint8(mod);
	if(mod>255 )
		newcol.blue=255;
	if(mod<0 )
		newcol.blue=0;

	delta=int8(col2.alpha)-int8(col.alpha);
	mod=col.alpha + (position * delta);
	newcol.alpha=uint8(mod);
	if(mod>255 )
		newcol.alpha=255;
	if(mod<0 )
		newcol.alpha=0;
#ifdef DEBUG_COLOR_UTILS
printf("MakeBlendColor( {%u,%u,%u,%u}, {%u,%u,%u,%u}, %f) : {%u,%u,%u,%u}\n",
	col.red,col.green,col.blue,col.alpha,
	col2.red,col2.green,col2.blue,col2.alpha,
	position,
	newcol.red,newcol.green,newcol.blue,newcol.alpha);
#endif

	return RGBColor(newcol);
}

void RGBColor::PrintToStream(void)
{
	printf("RGBColor (%u,%u,%u,%u)\n", color32.red,color32.green,color32.blue,color32.alpha);
}
