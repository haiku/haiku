#ifndef LAYERDATA_H_
#define LAYERDATA_H_

#include <Point.h>
#include <Font.h>
#include <RGBColor.h>
#include <FontServer.h>
#include <ServerFont.h>
#include <PatternHandler.h>
#include "GraphicsDefs.h"
#include "InterfaceDefs.h"

class ServerBitmap;
class ServerFont;

class LayerData
{
public:
	LayerData(void)
		{
			penlocation.Set(0,0);
			pensize			= 1.0;
			highcolor.SetColor(0, 0, 0, 255);
			lowcolor.SetColor(255, 255, 255, 255);
			viewcolor.SetColor(255, 255, 255, 255);
			patt			= pat_solidhigh;
			draw_mode		= B_OP_COPY;
			coordOrigin.Set(0.0, 0.0);
			
			lineCap			= B_BUTT_CAP;
			lineJoin		= B_BEVEL_JOIN;
			miterLimit		= B_DEFAULT_MITER_LIMIT;

			alphaSrcMode	= B_PIXEL_ALPHA;
			alphaFncMode	= B_ALPHA_OVERLAY;
			scale			= 1.0;
			fontAliasing	= true;
			font			= fontserver->GetSystemPlain();
			//font			= NULL;
			
			clippReg		= NULL;
		
				// NOTE: read below!			
			//background		= NULL;
			//overlay			= NULL;
			image			= NULL;
			isOverlay		= false;

			edelta.space	= 0;
			edelta.nonspace	= 0;
		}
	~LayerData(void)
		{
			if(font) {
				delete	font;
				font	= NULL;
			}
			if (image){
				/* NOTE: I don't know yet how bitmap allocation/deallocation
					is managed by server. I tend to think it's a reference
					count type, so a 'delete' it's NOT good here.
					Maybe a image.Release()?
					
					TODO: tell 'image' we're finished with it! :-)
				*/
			}
		}

		// BView related
	BPoint			penlocation;
	float			pensize;
	RGBColor 		highcolor,
					lowcolor,
					viewcolor;
	Pattern			patt;
	drawing_mode	draw_mode;
	BPoint			coordOrigin;
	
	cap_mode		lineCap;
	join_mode		lineJoin;
	float			miterLimit;
	
	source_alpha	alphaSrcMode;
	alpha_function	alphaFncMode;
	float			scale;
	bool			fontAliasing;
	ServerFont		*font;
	
	BRegion			*clippReg;

		// server related
	// NOTE: only one image is needed. It's a bitmap OR an overlay!
	ServerBitmap	*image;
	bool			isOverlay;
	//ServerBitmap	*background;
	//ServerBitmap	*overlay;

	escapement_delta	edelta;
};
#endif
