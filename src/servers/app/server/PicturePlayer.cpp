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
//	File Name:		PicturePlayer.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Server class to interpret and play BPicture data. Based on
//					Marc Flerackers' TPicture class
//------------------------------------------------------------------------------

#include "PicturePlayer.h"
#include "PictureProtocol.h"
#include "DisplayDriver.h"
#include <stdio.h>

PicturePlayer::PicturePlayer(DisplayDriver *d,void *data, int32 size)
 : fData(data, size)
{
	fdriver=d;
	pat=0xFFFFFFFFFFFFFFFFLL;
}

PicturePlayer::~PicturePlayer()
{
}

int16 PicturePlayer::GetOp()
{
	int16 data;

	fData.Read(&data, sizeof(int16));
	
	return data;
}

bool PicturePlayer::GetBool()
{
	bool data;

	fData.Read(&data, sizeof(bool));
	
	return data;
}

int16 PicturePlayer::GetInt16()
{
	int16 data;

	fData.Read(&data, sizeof(int16));
	
	return data;
}

int32 PicturePlayer::GetInt32()
{
	int32 data;

	fData.Read(&data, sizeof(int32));
	
	return data;
}

float PicturePlayer::GetFloat()
{
	float data;

	fData.Read(&data, sizeof(float));
	
	return data;
}

BPoint PicturePlayer::GetCoord()
{
	BPoint data;

	fData.Read(&data, sizeof(BPoint));
	
	return data;
}

BRect PicturePlayer::GetRect()
{
	BRect data;

	fData.Read(&data, sizeof(BRect));
	
	return data;
}

rgb_color PicturePlayer::GetColor()
{
	rgb_color data;

	fData.Read(&data, sizeof(rgb_color));
	
	return data;
}

void PicturePlayer::GetData(void *data, int32 size)
{
	fData.Read(data, size);
}

status_t PicturePlayer::Play(int32 tableEntries,void *userData, LayerData *d)
{
	if(!userData || !d)
		return B_ERROR;
	
	int16 op;
	int32 size=fData.Seek(0,SEEK_END);
	fData.Seek(0,SEEK_SET);
	off_t pos;
	fldata=*d;
	
	while (fData.Position() < size)
	{
		op = GetOp();
		size = GetInt32();
		pos = fData.Position();

		switch (op)
		{
			case B_PIC_MOVE_PEN_BY:
			{
				BPoint where = GetCoord();
				fldata.penlocation.x+=where.x;
				fldata.penlocation.y+=where.y;
				break;
			}
			case B_PIC_STROKE_LINE:
			{
				BPoint start = GetCoord();
				BPoint end = GetCoord();
				fdriver->StrokeLine(start,end,&fldata,pat);
				break;
			}
			case B_PIC_STROKE_RECT:
			{
				BRect rect = GetRect();
				fdriver->StrokeRect(rect,&fldata,pat);
				break;
			}
			case B_PIC_FILL_RECT:
			{
				BRect rect = GetRect();
				fdriver->FillRect(rect,&fldata,pat);
				break;
			}
			case B_PIC_STROKE_ROUND_RECT:
			{
				BRect rect = GetRect();
				BPoint radii = GetCoord();
				fdriver->StrokeRoundRect(rect,radii.x,radii.y,&fldata,pat);
				break;
			}
			case B_PIC_FILL_ROUND_RECT:
			{
				BRect rect = GetRect();
				BPoint radii = GetCoord();
				fdriver->FillRoundRect(rect,radii.x,radii.y,&fldata,pat);
				break;
			}
			case B_PIC_STROKE_BEZIER:
			{
				BPoint control[4];
				GetData(control, sizeof(control));
				fdriver->StrokeBezier(control,&fldata,pat);
				break;
			}
			case B_PIC_FILL_BEZIER:
			{
				BPoint control[4];
				GetData(control, sizeof(control));
				fdriver->FillBezier(control,&fldata,pat);
				break;
			}
			case B_PIC_STROKE_POLYGON:
			{
				int32 numPoints = GetInt32();
				BPoint *points = new BPoint[numPoints];
				GetData(points, numPoints * sizeof(BPoint));
				bool isClosed = GetBool();
				fdriver->StrokePolygon(points,numPoints,BRect(0,0,0,0),&fldata,pat,isClosed);
				delete points;
				break;
			}
			case B_PIC_FILL_POLYGON:
			{
				int32 numPoints = GetInt32();
				BPoint *points = new BPoint[numPoints];
				GetData(points, numPoints * sizeof(BPoint));
				fdriver->FillPolygon(points,numPoints,BRect(0,0,0,0),&fldata,pat);
				delete points;
				break;
			}
			case B_PIC_STROKE_SHAPE:
			case B_PIC_FILL_SHAPE:
				break;
			case B_PIC_DRAW_STRING:
			{
				int32 len = GetInt32();
				char *string = new char[len + 1];
				GetData(string, len);
				string[len] = '\0';
//				float deltax = GetFloat();
//				float deltay = GetFloat();
				
				// TODO: The deltas given are escapements. Find out how they translate into
				// escapement_delta units.
				
				fdriver->DrawString(string,len,fldata.penlocation,&fldata,NULL);
				delete string;
				break;
			}
			case B_PIC_DRAW_PIXELS:
			{
				// Equivalent of DrawBitmap(). 
				
				// TODO: Implement
				
/*				BRect src = GetRect();
				BRect dest = GetRect();
				int32 width = GetInt32();
				int32 height = GetInt32();
				int32 bytesPerRow = GetInt32();
				int32 pixelFormat = GetInt32();
				int32 flags = GetInt32();
				char *data = new char[size - (fData.Position() - pos)];
				GetData(data, size - (fData.Position() - pos));
				((fnc_DrawPixels)callBackTable[18])(userData, src, dest,
					width, height, bytesPerRow, pixelFormat, flags, data);
				delete data;
*/				break;
			}
			case B_PIC_DRAW_PICTURE:
			{
				break;
			}
			case B_PIC_STROKE_ARC:
			{
				BPoint center = GetCoord();
				BPoint radii = GetCoord();
				float startTheta = GetFloat();
				float arcTheta = GetFloat();
				fdriver->StrokeArc(BRect(center.x-radii.x,center.y-radii.y,center.x+radii.x,
						center.y+radii.y),startTheta, arcTheta, &fldata,pat);
				break;
			}
			case B_PIC_FILL_ARC:
			{
				BPoint center = GetCoord();
				BPoint radii = GetCoord();
				float startTheta = GetFloat();
				float arcTheta = GetFloat();
				fdriver->FillArc(BRect(center.x-radii.x,center.y-radii.y,center.x+radii.x,
						center.y+radii.y),startTheta, arcTheta, &fldata,pat);
				break;
			}
			case B_PIC_STROKE_ELLIPSE:
			{
				BRect rect = GetRect();
				BPoint center;
				BPoint radii((rect.Width() + 1) / 2.0f, (rect.Height() + 1) / 2.0f);
				center = rect.LeftTop() + radii;
				fdriver->StrokeEllipse(rect,&fldata,pat);
				break;
			}
			case B_PIC_FILL_ELLIPSE:
			{
				BRect rect = GetRect();
				BPoint center;
				BPoint radii((rect.Width() + 1) / 2.0f, (rect.Height() + 1) / 2.0f);
				center = rect.LeftTop() + radii;
				fdriver->FillEllipse(rect,&fldata,pat);
				break;
			}
			case B_PIC_ENTER_STATE_CHANGE:
			{
				break;
			}
			case B_PIC_SET_CLIPPING_RECTS:
			{
				break;
			}
			case B_PIC_CLIP_TO_PICTURE:
			{
				//TODO: Implement
				break;
			}
			case B_PIC_PUSH_STATE:
			{
				//TODO: Implement
				break;
			}
			case B_PIC_POP_STATE:
			{
				//TODO: Implement
				break;
			}
			case B_PIC_CLEAR_CLIPPING_RECTS:
			{
				//TODO: Implement
				break;
			}
			case B_PIC_SET_ORIGIN:
			{
				BPoint pt = GetCoord();
				forigin=pt;
				break;
			}
			case B_PIC_SET_PEN_LOCATION:
			{
				BPoint pt = GetCoord();
				fldata.penlocation=pt;
				break;
			}
			case B_PIC_SET_DRAWING_MODE:
			{
				int16 mode = GetInt16();
				fldata.draw_mode=(drawing_mode)mode;
				break;
			}
			case B_PIC_SET_LINE_MODE:
			{
				//TODO: Implement

/*				int16 capMode = GetInt16();
				int16 joinMode = GetInt16();
				float miterLimit = GetFloat();
				((fnc_ssf)callBackTable[31])(userData, capMode, joinMode, miterLimit);
*/				break;
			}
			case B_PIC_SET_PEN_SIZE:
			{
				float size = GetFloat();
				fldata.pensize=size;
				break;
			}
			case B_PIC_SET_SCALE:
			{
				float scale = GetFloat();
				fldata.scale=scale;
				break;
			}
			case B_PIC_SET_FORE_COLOR:
			{			
				rgb_color color = GetColor();
				fldata.highcolor=color;
				break;
			}
			case B_PIC_SET_BACK_COLOR:
			{			
				rgb_color color = GetColor();
				fldata.lowcolor=color;
				break;
			}
			case B_PIC_SET_STIPLE_PATTERN:
			{
				pattern p;
				GetData(&p, sizeof(p));
				pat=*((uint64*)p.data);
				break;
			}
			case B_PIC_ENTER_FONT_STATE:
			{
				//TODO: Implement
				break;
			}
			case B_PIC_SET_BLENDING_MODE:
			{
				int16 alphaSrcMode = GetInt16();
				int16 alphaFncMode = GetInt16();
				fldata.alphaSrcMode = (source_alpha)alphaSrcMode;
				fldata.alphaFncMode = (alpha_function)alphaFncMode;
				break;
			}
			case B_PIC_SET_FONT_FAMILY:
			{
				//TODO: Implement
/*				int32 len = GetInt32();
				char *string = new char[len + 1];
				GetData(string, len);
				string[len] = '\0';
				((fnc_Pc)callBackTable[37])(userData, string);
				delete string;
*/				break;
			}
			case B_PIC_SET_FONT_STYLE:
			{
				//TODO: Implement
/*				int32 len = GetInt32();
				char *string = new char[len + 1];
				GetData(string, len);
				string[len] = '\0';
				((fnc_Pc)callBackTable[38])(userData, string);
				delete string;
*/				break;
			}
			case B_PIC_SET_FONT_SPACING:
			{
				//TODO: Implement
//				int32 spacing = GetInt32();
				break;
			}
			case B_PIC_SET_FONT_ENCODING:
			{
				//TODO: Implement
//				int32 encoding = GetInt32();
				break;
			}
			case B_PIC_SET_FONT_FLAGS:
			{
				//TODO: Implement
//				int32 flags = GetInt32();
				break;
			}
			case B_PIC_SET_FONT_SIZE:
			{
				//TODO: Implement
//				float size = GetFloat();
				break;
			}
			case B_PIC_SET_FONT_ROTATE:
			{
				//TODO: Implement
//				float rotation = GetFloat();
				break;
			}
			case B_PIC_SET_FONT_SHEAR:
			{
				//TODO: Implement
//				float shear = GetFloat();
				break;
			}
			case B_PIC_SET_FONT_FACE:
			{
				//TODO: Implement
//				int32 flags = GetInt32();
				break;
			}
			default:
				break;
		}

		// If we didn't read enough bytes, skip them. This is not a error
		// since the instructions can change over time.
		if (fData.Position() - pos < size)
			fData.Seek(size - (fData.Position() - pos), SEEK_CUR);
	}

	return B_OK;
}


