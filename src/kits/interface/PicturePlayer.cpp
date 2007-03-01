/*
 * Copyright 2001-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Marcus Overhagen (marcus@overhagen.de)
 */

/**	PicturePlayer is used to create and play picture data. */

#include <stdio.h>

#include <PicturePlayer.h>
#include <PictureProtocol.h>

#include <Shape.h>

typedef void (*fnc)(void*);
typedef void (*fnc_BPoint)(void*, BPoint);
typedef void (*fnc_BPointBPoint)(void*, BPoint, BPoint);
typedef void (*fnc_BRect)(void*, BRect);
typedef void (*fnc_BRectBPoint)(void*, BRect, BPoint);
typedef void (*fnc_PBPoint)(void*, const BPoint*);
typedef void (*fnc_i)(void*, int32);
typedef void (*fnc_iPBPointb)(void*, int32, const BPoint*, bool);
typedef void (*fnc_iPBPoint)(void*, int32, const BPoint*);
typedef void (*fnc_Pc)(void*, const char*);
typedef void (*fnc_Pcff)(void*, const char*, float, float);
typedef void (*fnc_BPointBPointff)(void*, BPoint, BPoint, float, float);
typedef void (*fnc_s)(void*, int16);
typedef void (*fnc_ssf)(void*, int16, int16, float);
typedef void (*fnc_f)(void*, float);
typedef void (*fnc_Color)(void*, rgb_color);
typedef void (*fnc_Pattern)(void*, pattern);
typedef void (*fnc_ss)(void *, int16, int16);
typedef void (*fnc_PBRecti)(void*, const BRect*, int32);
typedef void (*fnc_DrawPixels)(void *, BRect, BRect, int32, int32, int32,
							   int32, int32, const void *);
typedef void (*fnc_BShape)(void*, BShape*);


PicturePlayer::PicturePlayer(const void *data, size_t size, BList *pictures)
	:	fData(data),
		fSize(size),
		fPictures(pictures)
{
}


PicturePlayer::~PicturePlayer()
{
}


status_t
PicturePlayer::Play(void **callBackTable, int32 tableEntries, void *userData)
{
	// TODO: we should probably check if the functions in the table are not NULL
	//       before calling them.

	const char *data = reinterpret_cast<const char *>(fData);
	size_t pos = 0;

	while ((pos + 6) <= fSize) {
		int16 op = *reinterpret_cast<const int16 *>(data);
		int32 size = *reinterpret_cast<const int32 *>(data + 2);
		pos += 6;
		data += 6;

		if (pos + size > fSize)
			debugger("PicturePlayer::Play: buffer overrun\n");

		switch (op) {
			case B_PIC_MOVE_PEN_BY:
			{
				((fnc_BPoint)callBackTable[1])(userData,
					*reinterpret_cast<const BPoint *>(data)); /* where */
				break;
			}

			case B_PIC_STROKE_LINE:
			{
				((fnc_BPointBPoint)callBackTable[2])(userData, 
					*reinterpret_cast<const BPoint *>(data), /* start */
					*reinterpret_cast<const BPoint *>(data + sizeof(BPoint))); /* end */
				break;
			}

			case B_PIC_STROKE_RECT:
			{
				((fnc_BRect)callBackTable[3])(userData,
					*reinterpret_cast<const BRect *>(data)); /* rect */
				break;
			}

			case B_PIC_FILL_RECT:
			{
				((fnc_BRect)callBackTable[4])(userData,
					*reinterpret_cast<const BRect *>(data)); /* rect */
				break;
			}

			case B_PIC_STROKE_ROUND_RECT:
			{
				((fnc_BRectBPoint)callBackTable[5])(userData,
					*reinterpret_cast<const BRect *>(data), /* rect */
					*reinterpret_cast<const BPoint *>(data + sizeof(BRect))); /* radii */
				break;
			}

			case B_PIC_FILL_ROUND_RECT:
			{
				((fnc_BRectBPoint)callBackTable[6])(userData,
					*reinterpret_cast<const BRect *>(data), /* rect */
					*reinterpret_cast<const BPoint *>(data + sizeof(BRect))); /* radii */
				break;
			}

			case B_PIC_STROKE_BEZIER:
			{
				((fnc_PBPoint)callBackTable[7])(userData,
					reinterpret_cast<const BPoint *>(data));
				break;
			}

			case B_PIC_FILL_BEZIER:
			{
				((fnc_PBPoint)callBackTable[8])(userData,
					reinterpret_cast<const BPoint *>(data));
				break;
			}

			case B_PIC_STROKE_ARC:
			{
				((fnc_BPointBPointff)callBackTable[9])(userData, 
					*reinterpret_cast<const BPoint *>(data), /* center */
					*reinterpret_cast<const BPoint *>(data + sizeof(BPoint)), /* radii */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint)), /* startTheta */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint) + sizeof(float))); /* arcTheta */
				break;
			}

			case B_PIC_FILL_ARC:
			{
				((fnc_BPointBPointff)callBackTable[10])(userData,
					*reinterpret_cast<const BPoint *>(data), /* center */
					*reinterpret_cast<const BPoint *>(data + sizeof(BPoint)), /* radii */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint)), /* startTheta */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint) + sizeof(float))); /* arcTheta */
				break;
			}

			case B_PIC_STROKE_ELLIPSE:
			{
				const BRect *rect = reinterpret_cast<const BRect *>(data);
				BPoint radii((rect->Width() + 1) / 2.0f, (rect->Height() + 1) / 2.0f);
				BPoint center = rect->LeftTop() + radii;
				((fnc_BPointBPoint)callBackTable[11])(userData, center, radii);
				break;
			}

			case B_PIC_FILL_ELLIPSE:
			{
				const BRect *rect = reinterpret_cast<const BRect *>(data);
				BPoint radii((rect->Width() + 1) / 2.0f, (rect->Height() + 1) / 2.0f);
				BPoint center = rect->LeftTop() + radii;
				((fnc_BPointBPoint)callBackTable[12])(userData, center, radii);
				break;
			}

			case B_PIC_STROKE_POLYGON:
			{
				int32 numPoints = *reinterpret_cast<const int32 *>(data);
				((fnc_iPBPointb)callBackTable[13])(userData, 
					numPoints,
					reinterpret_cast<const BPoint *>(data + sizeof(int32)), /* points */
					*reinterpret_cast<const uint8 *>(data + sizeof(int32) + numPoints * sizeof(BPoint))); /* is-closed */
				break;
			}

			case B_PIC_FILL_POLYGON:
			{
				((fnc_iPBPoint)callBackTable[14])(userData, 
					*reinterpret_cast<const int32 *>(data), /* numPoints */
					reinterpret_cast<const BPoint *>(data + sizeof(int32))); /* points */
				break;
			}

			case B_PIC_STROKE_SHAPE:
			case B_PIC_FILL_SHAPE:
			{
				int32 opCount = *reinterpret_cast<const int32 *>(data);
				int32 ptCount = *reinterpret_cast<const int32 *>(data + sizeof(int32));
				const uint32 *opList = reinterpret_cast<const uint32 *>(data + 2 * sizeof(int32));
				const BPoint *ptList = reinterpret_cast<const BPoint *>(data + 2 * sizeof(int32) + opCount * sizeof(uint32));

				// TODO: remove BShape data copying
				BShape shape; 
				shape.SetData(opCount, ptCount, opList, ptList);

				const int32 tableIndex = (op == B_PIC_STROKE_SHAPE) ? 15 : 16;
				((fnc_BShape)callBackTable[tableIndex])(userData, &shape);
				break;
			}
			
			case B_PIC_DRAW_STRING:
			{
				((fnc_Pcff)callBackTable[17])(userData, 
					reinterpret_cast<const char *>(data + 2 * sizeof(float)), /* string */
					*reinterpret_cast<const float *>(data), /* escapement.space */
					*reinterpret_cast<const float *>(data + sizeof(float))); /* escapement.nonspace */
				break;
			}

			case B_PIC_DRAW_PIXELS:
			{
				((fnc_DrawPixels)callBackTable[18])(userData,
					*reinterpret_cast<const BRect *>(data), /* src */
					*reinterpret_cast<const BRect *>(data + 1 * sizeof(BRect)), /* dst */
					*reinterpret_cast<const int32 *>(data + 2 * sizeof(BRect)), /* width */
					*reinterpret_cast<const int32 *>(data + 2 * sizeof(BRect) + 1 * sizeof(int32)), /* height */
					*reinterpret_cast<const int32 *>(data + 2 * sizeof(BRect) + 2 * sizeof(int32)), /* bytesPerRow */
					*reinterpret_cast<const int32 *>(data + 2 * sizeof(BRect) + 3 * sizeof(int32)), /* pixelFormat */
					*reinterpret_cast<const int32 *>(data + 2 * sizeof(BRect) + 4 * sizeof(int32)), /* flags */
					reinterpret_cast<const void *>(data + 2 * sizeof(BRect) + 5 * sizeof(int32))); /* data */
				break;
			}

			case B_PIC_DRAW_PICTURE:
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

			case B_PIC_ENTER_STATE_CHANGE:
			{
				((fnc)callBackTable[24])(userData);
				break;
			}

			case B_PIC_ENTER_FONT_STATE:
			{
				((fnc)callBackTable[26])(userData);
				break;
			}

			case B_PIC_SET_ORIGIN:
			{
				((fnc_BPoint)callBackTable[28])(userData,
					*reinterpret_cast<const BPoint *>(data)); /* origin */
				break;
			}

			case B_PIC_SET_PEN_LOCATION:
			{
				((fnc_BPoint)callBackTable[29])(userData,
					*reinterpret_cast<const BPoint *>(data)); /* location */
				break;
			}

			case B_PIC_SET_DRAWING_MODE:
			{
				((fnc_s)callBackTable[30])(userData,
					*reinterpret_cast<const int16 *>(data)); /* mode */
				break;
			}

			case B_PIC_SET_LINE_MODE:
			{
				((fnc_ssf)callBackTable[31])(userData,
					*reinterpret_cast<const int16 *>(data), /* cap-mode */
					*reinterpret_cast<const int16 *>(data + 1 * sizeof(int16)), /* join-mode */
					*reinterpret_cast<const float *>(data + 2 * sizeof(int16))); /* miter-limit */
				break;
			}

			case B_PIC_SET_PEN_SIZE:
			{
				((fnc_f)callBackTable[32])(userData,
					*reinterpret_cast<const float *>(data)); /* size */
				break;
			}

			case B_PIC_SET_FORE_COLOR:
			{			
				((fnc_Color)callBackTable[33])(userData,
					*reinterpret_cast<const rgb_color *>(data)); /* color */
				break;
			}

			case B_PIC_SET_BACK_COLOR:
			{			
				((fnc_Color)callBackTable[34])(userData,
					*reinterpret_cast<const rgb_color *>(data)); /* color */
				break;
			}

			case B_PIC_SET_STIPLE_PATTERN:
			{
				((fnc_Pattern)callBackTable[35])(userData,
					*reinterpret_cast<const pattern *>(data)); /* pattern */
				break;
			}

			case B_PIC_SET_SCALE:
			{
				((fnc_f)callBackTable[36])(userData,
					*reinterpret_cast<const float *>(data)); /* scale */
				break;
			}

			case B_PIC_SET_FONT_FAMILY:
			{
				debugger("B_PIC_SET_FONT_FAMILY"); // TODO: is this unused?
				((fnc_Pc)callBackTable[37])(userData,
					reinterpret_cast<const char *>(data)); /* string */
				break;
			}

			case B_PIC_SET_FONT_STYLE:
			{
				debugger("B_PIC_SET_FONT_STYLE"); // TODO: is this unused?
				((fnc_Pc)callBackTable[38])(userData,
					reinterpret_cast<const char *>(data)); /* string */
				break;
			}

			case B_PIC_SET_FONT_SPACING:
			{
				((fnc_i)callBackTable[39])(userData,
					*reinterpret_cast<const int32 *>(data)); /* spacing */
				break;
			}

			case B_PIC_SET_FONT_SIZE:
			{
				((fnc_f)callBackTable[40])(userData,
					*reinterpret_cast<const float *>(data)); /* size */
				break;
			}

			case B_PIC_SET_FONT_ROTATE:
			{
				((fnc_f)callBackTable[41])(userData,
					*reinterpret_cast<const float *>(data)); /* rotation */
				break;
			}

			case B_PIC_SET_FONT_ENCODING:
			{
				((fnc_i)callBackTable[42])(userData,
					*reinterpret_cast<const int32 *>(data)); /* encoding */
				break;
			}

			case B_PIC_SET_FONT_FLAGS:
			{
				((fnc_i)callBackTable[43])(userData,
					*reinterpret_cast<const int32 *>(data)); /* flags */
				break;
			}
			
			case B_PIC_SET_FONT_SHEAR:
			{
				((fnc_f)callBackTable[44])(userData,
					*reinterpret_cast<const float *>(data)); /* shear */
				break;
			}

			case B_PIC_SET_FONT_FACE:
			{
				((fnc_i)callBackTable[46])(userData,
					*reinterpret_cast<const int32 *>(data)); /* flags */
				break;
			}

			// TODO: Looks like R5 function table only exports 47 functions...
			// I added this here as a temporary workaround, because there seems to be
			// no room for this op, although it's obviously implemented in some way...
			case B_PIC_SET_BLENDING_MODE:
			{
				((fnc_ss)callBackTable[47])(userData,
					*reinterpret_cast<const int16 *>(data), /* alphaSrcMode */
					*reinterpret_cast<const int16 *>(data + sizeof(int16))); /* alphaFncMode */
				break;
			}

			default:
				break;
		}

		pos += size;
		data += size;

		// TODO: what if too much was read, should we return B_ERROR?
	}

	return B_OK;
}
