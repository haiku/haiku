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
//	File Name: PicturePlayer.cpp
//	Author: 	Marc Flerackers (mflerackers@androme.be)
//  			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Server class to interpret and play BPicture data. Based on
//  				Marc Flerackers' TPicture class
//------------------------------------------------------------------------------
#ifndef	_PICTUREPLAYER_H
#define	_PICTUREPLAYER_H

#include <GraphicsDefs.h>
#include <Point.h>
#include <Rect.h>
#include <DataIO.h>
#include "LayerData.h"
#include "PatternHandler.h" // for pattern_union

class DisplayDriver;

class PicturePlayer
{
public:
	PicturePlayer(DisplayDriver *driver,void *data, int32 size);
	virtual	~PicturePlayer();

	int16 GetOp();
	bool GetBool();
	int16 GetInt8();
	int16 GetInt16();
	int32 GetInt32();
	int64 GetInt64();
	float GetFloat();
	BPoint GetCoord();
	BRect GetRect();
	rgb_color	GetColor();
	//void GetString(char *);

	void *GetData(int32);
	void GetData(void *data, int32 size);

	void AddInt8(int8);
	void AddInt16(int16);
	void AddInt32(int32);
	void AddInt64(int64);
	void AddFloat(float);
	void AddCoord(BPoint);
	void AddRect(BRect);
	void AddColor(rgb_color);
	void AddString(char *);

	void AddData(void *data, int32 size);

	void BeginOp(int32);
	void EndOp();

	void EnterStateChange();
	void ExitStateChange();

	void EnterFontChange();
	void ExitFontChange();
	
	void SetClippingRegion(BRect *rects, int32 numrects);
	
	status_t Play(int32 tableEntries,void *userData, LayerData *d);
	status_t Rewind();

private:
	BMemoryIO fData;
	int32 fSize;
	DisplayDriver *fdriver;
	LayerData fldata;
	BPoint forigin;
	Pattern stipplepat;
	BRegion *clipreg;
};

#endif

