#include "ColorUtils.h"
#include "SystemPalette.h"
#include <stdio.h>
#include <stdlib.h>

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

	return (uint8)cindex;
}

uint16 FindClosestColor16(rgb_color color)
{
	printf("FindClosestColor16() unimplemented\n");
	return 0;
}