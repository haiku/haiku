#ifndef LAYERDATA_H_
#define LAYERDATA_H_

#include <Point.h>
#include <View.h>
class ServerBitmap;

class LayerData
{
public:
LayerData(void)
	{
		pensize=1.0;
		penlocation.Set(0,0);
		draw_mode=B_OP_COPY;
		background=NULL;
		overlay=NULL;
		highcolor.SetColor(0,0,0,255);
		lowcolor.SetColor(255,255,255,255);
		//SFont font;
		//bool antialias_text;
		scale=1.0;
	};

	float pensize;
	BPoint penlocation;
	drawing_mode draw_mode;
	source_alpha alpha_mode;
	alpha_function blending_mode;
	ServerBitmap *background;
	ServerBitmap *overlay;
	RGBColor highcolor, lowcolor, viewcolor;
//	ServerFont *font;
	float scale;
	escapement_delta edelta;
};


#endif
