/*
	SystemPalette.cpp
		One global function to generate the palette which is the default BeOS
		System palette and the variable to go with it	
*/

#include "SystemPalette.h"

rgb_color SystemPalette[256];

void GenerateSystemPalette(rgb_color *palette)
{
	int i,j,index=0;
	int indexvals1[]={ 255,229,204,179,154,129,105,80,55,30 },
		indexvals2[]={ 255,203,152,102,51,0 };
	rgb_color *currentcol;
	
	// Grays 0,0,0 -> 248,248,248 by 8's	
	for(i=0; i<=248; i+=8,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=i;
		currentcol->green=i;
		currentcol->blue=i;
		currentcol->alpha=255;
	}
	
	// Blues, following indexvals1
	for(i=0; i<10; i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=0;
		currentcol->green=0;
		currentcol->blue=indexvals1[i];
		currentcol->alpha=255;
	}

	// Reds, following indexvals1 - 1
	for(i=0; i<10; i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=indexvals1[i] - 1;
		currentcol->green=0;
		currentcol->blue=0;
		currentcol->alpha=255;
	}

	// Greens, following indexvals1 - 1
	for(i=0; i<10; i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=0;
		currentcol->green=indexvals1[i] - 1;
		currentcol->blue=0;
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=152;
	currentcol->blue=51;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=255;
	currentcol->blue=255;
	index++;

	for(j=1;j<5;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=255;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	for(i=0;i<4;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=152;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=102;
	currentcol->blue=51;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=102;
	currentcol->blue=0;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=255;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=203;
	index++;
	
	// Mostly array runs from here on out
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=203;
		currentcol->green=203;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=152;
		currentcol->green=255;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=102;
		currentcol->green=255;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=51;
		currentcol->green=255;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=102;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=152;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=102;
	index++;

	// knocks out 4 assignment loops at once :)
	for(j=1;j<5;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=152;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	currentcol+=sizeof(rgb_color);
	currentcol->red=230;
	currentcol->green=134;
	currentcol->blue=0;
	index++;

	for(i=1;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=51;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=51;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=0;
	index++;

	for(j=1;j<5;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=102;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=0;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=175;
	currentcol->blue=19;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=51;
	currentcol->blue=255;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=51;
	currentcol->blue=203;
	index++;

	for(j=1;j<5;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=51;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	for(i=3;i>=0;i--,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=203;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	for(i=2;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=0;
		currentcol->green=51;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	for(i=0;i<5;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=203;
		currentcol->green=0;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=227;
	currentcol->blue=70;
	index++;

	for(j=2;j<6;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=0;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=203;
	currentcol->blue=51;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=203;
	currentcol->blue=0;
	index++;

	for(i=5;i<=0;i--,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=255;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

}
