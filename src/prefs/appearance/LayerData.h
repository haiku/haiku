#ifndef LAYERDATA_H_
#define LAYERDATA_H_

#include "SPoint.h"
class ServerBitmap;

class LayerData
{
public:
LayerData(void)
	{
		pensize=1.0;
		penlocation.Set(0,0);
		drawing_mode=0;
		bitmap_background=NULL;
		bitmap_overlay=NULL;
		highcolor.SetColor(0,0,0,255);
		lowcolor.SetColor(255,255,255,255);
		//SFont font;
		//bool antialias_text;
		scale=1.0;
	};

float pensize;
SPoint penlocation;
int32 drawing_mode;
ServerBitmap *bitmap_background;
ServerBitmap *bitmap_overlay;
RGBColor highcolor, lowcolor;
//SFont font;
//bool antialias_text;
float scale;
};


#endif