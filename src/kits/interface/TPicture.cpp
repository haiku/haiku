/*
 * Copyright 2001-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */

/**	PicturePlayer is used to create and play picture data. */

#include <stdio.h>

#include <TPicture.h>
#include <PictureProtocol.h>


typedef void (*fnc)(void*);
typedef void (*fnc_BPoint)(void*, BPoint);
typedef void (*fnc_BPointBPoint)(void*, BPoint, BPoint);
typedef void (*fnc_BRect)(void*, BRect);
typedef void (*fnc_BRectBPoint)(void*, BRect, BPoint);
typedef void (*fnc_PBPoint)(void*, BPoint*);
typedef void (*fnc_i)(void*, int32);
typedef void (*fnc_iPBPointb)(void*, int32, BPoint*, bool);
typedef void (*fnc_iPBPoint)(void*, int32, BPoint*);
typedef void (*fnc_Pc)(void*, char*);
typedef void (*fnc_Pcff)(void*, char*, float, float);
typedef void (*fnc_BPointBPointff)(void*, BPoint, BPoint, float, float);
typedef void (*fnc_s)(void*, int16);
typedef void (*fnc_ssf)(void*, int16, int16, float);
typedef void (*fnc_f)(void*, float);
typedef void (*fnc_Color)(void*, rgb_color);
typedef void (*fnc_Pattern)(void*, pattern);
typedef void (*fnc_ss)(void *, int16, int16);
typedef void (*fnc_PBRecti)(void*, BRect*, int32);
typedef void (*fnc_DrawPixels)(void *, BRect, BRect, int32, int32, int32,
							   int32, int32, void*);


PicturePlayer::PicturePlayer(void *data, int32 size, BList *pictures)
	:	fData(data, size),
		fPictures(pictures)
{
}


PicturePlayer::~PicturePlayer()
{
}


int16
PicturePlayer::GetOp()
{
	int16 data;

	fData.Read(&data, sizeof(int16));
	
	return data;
}


bool
PicturePlayer::GetBool()
{
	bool data;

	fData.Read(&data, sizeof(bool));
	
	return data;
}


int16
PicturePlayer::GetInt16()
{
	int16 data;

	fData.Read(&data, sizeof(int16));
	
	return data;
}


int32
PicturePlayer::GetInt32()
{
	int32 data;

	fData.Read(&data, sizeof(int32));
	
	return data;
}


float
PicturePlayer::GetFloat()
{
	float data;

	fData.Read(&data, sizeof(float));
	
	return data;
}


BPoint
PicturePlayer::GetCoord()
{
	BPoint data;

	fData.Read(&data, sizeof(BPoint));
	
	return data;
}


BRect
PicturePlayer::GetRect()
{
	BRect data;

	fData.Read(&data, sizeof(BRect));
	
	return data;
}


rgb_color
PicturePlayer::GetColor()
{
	rgb_color data;

	fData.Read(&data, sizeof(rgb_color));
	
	return data;
}


void
PicturePlayer::GetData(void *data, int32 size)
{
	fData.Read(data, size);
}


status_t
PicturePlayer::Play(void **callBackTable, int32 tableEntries, void *userData)
{
	// TODO: we should probably check if the functions in the table are not NULL
	//       before calling them.

	// lenght of the stream
	size_t length = fData.Seek(0, SEEK_END);
	fData.Seek(0, SEEK_SET);

	while (fData.Position() < length) {
		int16 op = GetOp();
		int32 size = GetInt32();
		off_t pos = fData.Position();

		switch (op) {
			case B_PIC_MOVE_PEN_BY:
			{
				BPoint where = GetCoord();
				((fnc_BPoint)callBackTable[1])(userData, where);
				break;
			}
			case B_PIC_STROKE_LINE:
			{
				BPoint start = GetCoord();
				BPoint end = GetCoord();
				((fnc_BPointBPoint)callBackTable[2])(userData, start, end);
				break;
			}
			case B_PIC_STROKE_RECT:
			{
				BRect rect = GetRect();
				((fnc_BRect)callBackTable[3])(userData, rect);
				break;
			}
			case B_PIC_FILL_RECT:
			{
				BRect rect = GetRect();
				((fnc_BRect)callBackTable[4])(userData, rect);
				break;
			}
			case B_PIC_STROKE_ROUND_RECT:
			{
				BRect rect = GetRect();
				BPoint radii = GetCoord();
				((fnc_BRectBPoint)callBackTable[5])(userData, rect, radii);
				break;
			}
			case B_PIC_FILL_ROUND_RECT:
			{
				BRect rect = GetRect();
				BPoint radii = GetCoord();
				((fnc_BRectBPoint)callBackTable[6])(userData, rect, radii);
				break;
			}
			case B_PIC_STROKE_BEZIER:
			{
				BPoint control[4];
				GetData(control, sizeof(control));
				((fnc_PBPoint)callBackTable[7])(userData, control);
				break;
			}
			case B_PIC_FILL_BEZIER:
			{
				BPoint control[4];
				GetData(control, sizeof(control));
				((fnc_PBPoint)callBackTable[8])(userData, control);
				break;
			}
case B_PIC_STROKE_ARC:
			{
				BPoint center = GetCoord();
				BPoint radii = GetCoord();
				float startTheta = GetFloat();
				float arcTheta = GetFloat();
				((fnc_BPointBPointff)callBackTable[9])(userData, center, radii,
					startTheta, arcTheta);
				break;
			}
			case B_PIC_FILL_ARC:
			{
				BPoint center = GetCoord();
				BPoint radii = GetCoord();
				float startTheta = GetFloat();
				float arcTheta = GetFloat();
				((fnc_BPointBPointff)callBackTable[10])(userData, center, radii,
					startTheta, arcTheta);
				break;
			}
			case B_PIC_STROKE_ELLIPSE:
			{
				BRect rect = GetRect();
				BPoint center;
				BPoint radii((rect.Width() + 1) / 2.0f, (rect.Height() + 1) / 2.0f);
				center = rect.LeftTop() + radii;
				((fnc_BPointBPoint)callBackTable[11])(userData, center, radii);
				break;
			}
			case B_PIC_FILL_ELLIPSE:
			{
				BRect rect = GetRect();
				BPoint center;
				BPoint radii((rect.Width() + 1) / 2.0f, (rect.Height() + 1) / 2.0f);
				center = rect.LeftTop() + radii;
				((fnc_BPointBPoint)callBackTable[12])(userData, center, radii);
				break;
			}
			case B_PIC_STROKE_POLYGON:
			{
				int32 numPoints = GetInt32();
				BPoint *points = new BPoint[numPoints];
				GetData(points, numPoints * sizeof(BPoint));
				bool isClosed = GetBool();
				((fnc_iPBPointb)callBackTable[13])(userData, numPoints, points, isClosed);
				delete points;
				break;
			}
			case B_PIC_FILL_POLYGON:
			{
				int32 numPoints = GetInt32();
				BPoint *points = new BPoint[numPoints];
				GetData(points, numPoints * sizeof(BPoint));
				((fnc_iPBPoint)callBackTable[14])(userData, numPoints, points);
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
				float deltax = GetFloat();
				float deltay = GetFloat();
				((fnc_Pcff)callBackTable[17])(userData, string, deltax, deltay);
				delete string;
				break;
			}
			case B_PIC_DRAW_PIXELS:
			{
				BRect src = GetRect();
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
				break;
			}
			case B_PIC_DRAW_PICTURE:
			{
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
			case B_PIC_CLEAR_CLIPPING_RECTS:
			{
				((fnc_PBRecti)callBackTable[20])(userData, NULL, 0);
				break;
			}
			case B_PIC_CLIP_TO_PICTURE:
			{
				break;
			}
			case B_PIC_PUSH_STATE:
			{
				((fnc)callBackTable[22])(userData);
				break;
			}
			case B_PIC_POP_STATE:
			{
				((fnc)callBackTable[23])(userData);
				break;
			}
			case B_PIC_ENTER_FONT_STATE:
			{
				((fnc)callBackTable[26])(userData);
				break;
			}
			case B_PIC_SET_ORIGIN:
			{
				BPoint pt = GetCoord();
				((fnc_BPoint)callBackTable[28])(userData, pt);
				break;
			}
			case B_PIC_SET_PEN_LOCATION:
			{
				BPoint pt = GetCoord();
				((fnc_BPoint)callBackTable[29])(userData, pt);
				break;
			}
			case B_PIC_SET_DRAWING_MODE:
			{
				int16 mode = GetInt16();
				((fnc_s)callBackTable[30])(userData, mode);
				break;
			}
			case B_PIC_SET_LINE_MODE:
			{
				int16 capMode = GetInt16();
				int16 joinMode = GetInt16();
				float miterLimit = GetFloat();
				((fnc_ssf)callBackTable[31])(userData, capMode, joinMode, miterLimit);
				break;
			}
			case B_PIC_SET_PEN_SIZE:
			{
				float size = GetFloat();
				((fnc_f)callBackTable[32])(userData, size);
				break;
			}
			case B_PIC_SET_FORE_COLOR:
			{			
				rgb_color color = GetColor();
				((fnc_Color)callBackTable[33])(userData, color);
				break;
			}
			case B_PIC_SET_BACK_COLOR:
			{			
				rgb_color color = GetColor();
				((fnc_Color)callBackTable[34])(userData, color);
				break;
			}
			case B_PIC_SET_STIPLE_PATTERN:
			{
				pattern p;
				GetData(&p, sizeof(p));
				((fnc_Pattern)callBackTable[35])(userData, p);
				break;
			}
			case B_PIC_SET_SCALE:
			{
				float scale = GetFloat();
				((fnc_f)callBackTable[36])(userData, scale);
				break;
			}
			case B_PIC_SET_FONT_FAMILY:
			{
				int32 len = GetInt32();
				char *string = new char[len + 1];
				GetData(string, len);
				string[len] = '\0';
				((fnc_Pc)callBackTable[37])(userData, string);
				delete string;
				break;
			}
			case B_PIC_SET_FONT_STYLE:
			{
				int32 len = GetInt32();
				char *string = new char[len + 1];
				GetData(string, len);
				string[len] = '\0';
				((fnc_Pc)callBackTable[38])(userData, string);
				delete string;
				break;
			}
			case B_PIC_SET_FONT_SPACING:
			{
				int32 spacing = GetInt32();
				((fnc_i)callBackTable[39])(userData, spacing);
				break;
			}
			case B_PIC_SET_FONT_SIZE:
			{
				float size = GetFloat();
				((fnc_f)callBackTable[40])(userData, size);
				break;
			}
			case B_PIC_SET_FONT_ROTATE:
			{
				float rotation = GetFloat();
				((fnc_f)callBackTable[41])(userData, rotation);
				break;
			}
			case B_PIC_SET_FONT_ENCODING:
			{
				int32 encoding = GetInt32();
				((fnc_i)callBackTable[42])(userData, encoding);
				break;
			}
			case B_PIC_SET_FONT_FLAGS:
			{
				int32 flags = GetInt32();
				((fnc_i)callBackTable[43])(userData, flags);
				break;
			}
			
			case B_PIC_SET_FONT_SHEAR:
			{
				float shear = GetFloat();
				((fnc_f)callBackTable[44])(userData, shear);
				break;
			}
			case B_PIC_SET_FONT_FACE:
			{
				int32 flags = GetInt32();
				((fnc_i)callBackTable[46])(userData, flags);
				break;
			}
			// TODO: Looks like R5 function table only exports 47 functions...
			// I added this here as a temporary workaround, because there seems to be
			// no room for this op, although it's obviously implemented in some way...
			case B_PIC_SET_BLENDING_MODE:
			{
				int16 alphaSrcMode = GetInt16();
				int16 alphaFncMode = GetInt16();
				((fnc_ss)callBackTable[47])(userData, alphaSrcMode, alphaFncMode);
				break;
			}
			default:
				break;
		}

		// If we didn't read enough bytes, skip them. This is not a error
		// since the instructions can change over time.
		if (fData.Position() - pos < size)
			fData.Seek(size - (fData.Position() - pos), SEEK_CUR);

		// TODO: what if too much was read, should we return B_ERROR?
	}

	return B_OK;
}
