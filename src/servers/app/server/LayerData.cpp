//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		LayerData.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@mymail.ro>
//	Description:	Data classes for working with BView states and draw parameters
//  
//------------------------------------------------------------------------------
#include "LayerData.h"
#include <Layer.h>

DrawData::DrawData(void)
{
	pensize=1.0;
	penlocation.Set(0,0);
	highcolor.SetColor(0, 0, 0, 255);
	lowcolor.SetColor(255, 255, 255, 255);

	patt=pat_solidhigh;
	draw_mode=B_OP_COPY;
	
	lineCap	=B_BUTT_CAP;
	lineJoin=B_BEVEL_JOIN;
	miterLimit=B_DEFAULT_MITER_LIMIT;

	alphaSrcMode=B_PIXEL_ALPHA;
	alphaFncMode=B_ALPHA_OVERLAY;

	scale=1.0;
	fontAliasing=true;

	if(fontserver)
		font=*(fontserver->GetSystemPlain());
	
	clippReg=NULL;

	edelta.space=0;
	edelta.nonspace=0;
}

DrawData::~DrawData(void)
{
	if (clippReg)
		delete clippReg;
}

DrawData::DrawData(const DrawData &data)
{
	*this=data;
}

DrawData &DrawData::operator=(const DrawData &from)
{
	pensize=from.pensize;
	highcolor=from.highcolor;
	lowcolor=from.lowcolor;

	patt=from.patt;
	draw_mode=from.draw_mode;
	penlocation=from.penlocation;
	
	lineCap=from.lineCap;
	lineJoin=from.lineJoin;
	miterLimit=from.miterLimit;
	
	alphaSrcMode=from.alphaSrcMode;
	alphaFncMode=from.alphaFncMode;

	scale=from.scale;
	fontAliasing=from.fontAliasing;
	font=from.font;
	
	if(from.clippReg)
	{
		if(clippReg)
			*clippReg=*(from.clippReg);
		else
			clippReg=new BRegion(*(from.clippReg));
	}
	
	edelta=from.edelta;
	
	return *this;
}

LayerData::LayerData(void)
{
	coordOrigin.Set(0.0, 0.0);
	viewcolor.SetColor(255,255,255,255);
	
	background=NULL;
	overlay=NULL;
	prevState=NULL;
}

LayerData::LayerData(const LayerData &data)
{
	*this=data;
}

LayerData::~LayerData(void)
{
	if (prevState)
	{
		delete prevState;
		prevState=NULL;
	}
}

LayerData &LayerData::operator=(const LayerData &from)
{
	// I'm not sure if the DrawData copy constructor will get called before this one is,
	// so I'll just make sure the copy happens. :P
	pensize=from.pensize;
	highcolor=from.highcolor;
	lowcolor=from.lowcolor;

	patt=from.patt;
	draw_mode=from.draw_mode;
	
	lineCap=from.lineCap;
	lineJoin=from.lineJoin;
	miterLimit=from.miterLimit;
	
	alphaSrcMode=from.alphaSrcMode;
	alphaFncMode=from.alphaFncMode;

	scale=from.scale;
	fontAliasing=from.fontAliasing;
	font=from.font;
	
	if(from.clippReg)
	{
		if(clippReg)
			*clippReg=*(from.clippReg);
		else
			clippReg=new BRegion(*(from.clippReg));
	}
	
	edelta=from.edelta;
	
	// Stuff specific to LayerData
	coordOrigin=from.coordOrigin;;
	viewcolor=from.viewcolor;
	
	background=from.background;
	overlay=from.overlay;
	
	prevState=from.prevState;
	
	return *this;
}
