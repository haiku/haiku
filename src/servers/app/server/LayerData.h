#ifndef LAYERDATA_H_
#define LAYERDATA_H_

#include <Point.h>
#include <Font.h>
#include "RGBColor.h"
#include "FontServer.h"
#include "ServerFont.h"

class ServerBitmap;

class LayerData
{
public:
	LayerData(void)
		{
			pensize=1.0;
			penlocation.Set(0,0);
			draw_mode=B_OP_COPY;
			blending_mode=B_ALPHA_OVERLAY;
			alpha_mode=B_CONSTANT_ALPHA;
			background=NULL;
			overlay=NULL;
			highcolor.SetColor(0,0,0,255);
			lowcolor.SetColor(255,255,255,255);
			viewcolor.SetColor(255,255,255,255);
			font=fontserver->GetSystemPlain();
			scale=1.0;
			edelta.space=0;
			edelta.nonspace=0;
		}
	~LayerData(void)
		{
			if(font)
			{
				delete font;
				font=NULL;
			}
		}

	float pensize;
	BPoint penlocation;
	drawing_mode draw_mode;
	source_alpha alpha_mode;
	alpha_function blending_mode;
	ServerBitmap *background;
	ServerBitmap *overlay;
	RGBColor highcolor, lowcolor, viewcolor;
	ServerFont *font;
	float scale;
	escapement_delta edelta;
};
#endif
