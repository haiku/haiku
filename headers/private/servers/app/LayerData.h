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
//	File Name:		LayerData.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@mymail.ro>
//	Description:	Data classes for working with BView states and draw parameters
//  
//------------------------------------------------------------------------------
#ifndef LAYERDATA_H_
#define LAYERDATA_H_

#include <Point.h>
#include <Font.h>
#include <Region.h>
#include <RGBColor.h>
#include <FontServer.h>
#include <ServerFont.h>
#include <PatternHandler.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>

class ServerBitmap;
class ServerFont;
class ServerPicture;
class Layer;

class DrawData
{
public:
								DrawData(void);
								DrawData(const DrawData &data);
	virtual						~DrawData(void);
			DrawData& operator=(const DrawData &from);
			// TODO: uncomment and implement DrawData::PrintToStream when needed.
//	virtual	void				PrintToStream() const;

			BPoint				penlocation;

			RGBColor			highcolor,
					 			lowcolor;

			float				pensize;
			Pattern				patt;
			drawing_mode		draw_mode;
	
			cap_mode			lineCap;
			join_mode			lineJoin;
			float				miterLimit;
	
			source_alpha		alphaSrcMode;
			alpha_function		alphaFncMode;
			float				scale;
			bool				fontAliasing;
			ServerFont			font;
	
			BRegion*			clipReg;
	
			escapement_delta	edelta;
};

class LayerData : public DrawData
{
public:
								LayerData(void);
								LayerData(const Layer *layer);
								LayerData(const LayerData &data);
	virtual						~LayerData(void);
			LayerData &operator=(const LayerData &from);

	virtual	void				PrintToStream() const;

			BPoint				coordOrigin;

			RGBColor			viewcolor;

			// We have both because we are not going to suffer from the limitation that R5
			// places on us. We can have both. :)
			ServerBitmap*		background;
			ServerBitmap*		overlay;

			// used for the state stack
			LayerData*			prevState;
};
#endif
