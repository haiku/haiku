#include "ColorUtils.h"
#include <stdio.h>
#include <stdlib.h>

//#define DEBUG_COLOR_UTILS

void SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a=255)
{
	col->red=r;
	col->green=g;
	col->blue=b;
	col->alpha=a;
}

uint8 FindClosestColor(rgb_color *palette, rgb_color color)
{
	// We will be passed a 256-color palette and a 24-bit color
	uint16 cindex=0,cdelta=765,delta=765;
	rgb_color *c;
	
	for(uint16 i=0;i<256;i++)
	{
		c=&(palette[i]);
		delta=abs(c->red-color.red)+abs(c->green-color.green)+
			abs(c->blue-color.blue);

		if(delta==0)
		{
			cindex=i;
			break;
		}

		if(delta<cdelta)
		{
			cindex=i;
			cdelta=delta;
		}
	}
#ifdef DEBUG_COLOR_UTILS
printf("FindClosestColor( {%u,%u,%u,%u} ) : {%u,%u,%u,%u}\n",
	color.red,color.green,color.blue,color.alpha,
	palette[cindex].red,palette[cindex].green,palette[cindex].blue,palette[cindex].alpha);
#endif

	return (uint8)cindex;
}

uint16 FindClosestColor16(rgb_color color)
{
	printf("FindClosestColor16 unimplemented\n");
	return 0;
}

rgb_color MakeBlendColor(rgb_color col, rgb_color col2, float position)
{
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
	return newcol;
}
