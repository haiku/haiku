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
typedef void (*fnc_DrawPicture)(void *, BPoint, int32);
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
	// We don't check if the functions in the table are NULL, but we
	// check the tableEntries to see if the table is big enough.
	// If an application supplies the wrong size or an invalid pointer,
	// it's its own fault.

	// TODO: Call the exit_font_state_change and exit_state_change hooks.
	// there are no ops for them, so we need to keep track of the size of the data block,
	// and call them when we are at the end.
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
				if (tableEntries <= 1)
					break;
				((fnc_BPoint)callBackTable[1])(userData,
					*reinterpret_cast<const BPoint *>(data)); /* where */
				break;
			}

			case B_PIC_STROKE_LINE:
			{
				if (tableEntries <= 2)
					break;
				((fnc_BPointBPoint)callBackTable[2])(userData, 
					*reinterpret_cast<const BPoint *>(data), /* start */
					*reinterpret_cast<const BPoint *>(data + sizeof(BPoint))); /* end */
				break;
			}

			case B_PIC_STROKE_RECT:
			{
				if (tableEntries <= 3)
					break;
				((fnc_BRect)callBackTable[3])(userData,
					*reinterpret_cast<const BRect *>(data)); /* rect */
				break;
			}

			case B_PIC_FILL_RECT:
			{
				if (tableEntries <= 4)
					break;
				((fnc_BRect)callBackTable[4])(userData,
					*reinterpret_cast<const BRect *>(data)); /* rect */
				break;
			}

			case B_PIC_STROKE_ROUND_RECT:
			{
				if (tableEntries <= 5)
					break;
				((fnc_BRectBPoint)callBackTable[5])(userData,
					*reinterpret_cast<const BRect *>(data), /* rect */
					*reinterpret_cast<const BPoint *>(data + sizeof(BRect))); /* radii */
				break;
			}

			case B_PIC_FILL_ROUND_RECT:
			{
				if (tableEntries <= 6)
					break;
				((fnc_BRectBPoint)callBackTable[6])(userData,
					*reinterpret_cast<const BRect *>(data), /* rect */
					*reinterpret_cast<const BPoint *>(data + sizeof(BRect))); /* radii */
				break;
			}

			case B_PIC_STROKE_BEZIER:
			{
				if (tableEntries <= 7)
					break;
				((fnc_PBPoint)callBackTable[7])(userData,
					reinterpret_cast<const BPoint *>(data));
				break;
			}

			case B_PIC_FILL_BEZIER:
			{
				if (tableEntries <= 8)
					break;
				((fnc_PBPoint)callBackTable[8])(userData,
					reinterpret_cast<const BPoint *>(data));
				break;
			}

			case B_PIC_STROKE_ARC:
			{
				if (tableEntries <= 9)
					break;
				((fnc_BPointBPointff)callBackTable[9])(userData, 
					*reinterpret_cast<const BPoint *>(data), /* center */
					*reinterpret_cast<const BPoint *>(data + sizeof(BPoint)), /* radii */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint)), /* startTheta */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint) + sizeof(float))); /* arcTheta */
				break;
			}

			case B_PIC_FILL_ARC:
			{
				if (tableEntries <= 10)
					break;
				((fnc_BPointBPointff)callBackTable[10])(userData,
					*reinterpret_cast<const BPoint *>(data), /* center */
					*reinterpret_cast<const BPoint *>(data + sizeof(BPoint)), /* radii */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint)), /* startTheta */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint) + sizeof(float))); /* arcTheta */
				break;
			}

			case B_PIC_STROKE_ELLIPSE:
			{
				if (tableEntries <= 11)
					break;
				const BRect *rect = reinterpret_cast<const BRect *>(data);
				BPoint radii((rect->Width() + 1) / 2.0f, (rect->Height() + 1) / 2.0f);
				BPoint center = rect->LeftTop() + radii;
				((fnc_BPointBPoint)callBackTable[11])(userData, center, radii);
				break;
			}

			case B_PIC_FILL_ELLIPSE:
			{
				if (tableEntries <= 12)
					break;
				const BRect *rect = reinterpret_cast<const BRect *>(data);
				BPoint radii((rect->Width() + 1) / 2.0f, (rect->Height() + 1) / 2.0f);
				BPoint center = rect->LeftTop() + radii;
				((fnc_BPointBPoint)callBackTable[12])(userData, center, radii);
				break;
			}

			case B_PIC_STROKE_POLYGON:
			{
				if (tableEntries <= 13)
					break;
				int32 numPoints = *reinterpret_cast<const int32 *>(data);
				((fnc_iPBPointb)callBackTable[13])(userData, 
					numPoints,
					reinterpret_cast<const BPoint *>(data + sizeof(int32)), /* points */
					*reinterpret_cast<const uint8 *>(data + sizeof(int32) + numPoints * sizeof(BPoint))); /* is-closed */
				break;
			}

			case B_PIC_FILL_POLYGON:
			{
				if (tableEntries <= 14)
					break;
				((fnc_iPBPoint)callBackTable[14])(userData, 
					*reinterpret_cast<const int32 *>(data), /* numPoints */
					reinterpret_cast<const BPoint *>(data + sizeof(int32))); /* points */
				break;
			}

			case B_PIC_STROKE_SHAPE:
			case B_PIC_FILL_SHAPE:
			{
				const bool stroke = (op == B_PIC_STROKE_SHAPE);							
				if (tableEntries <= 16 || (stroke && tableEntries <= 15))
					break;

				int32 opCount = *reinterpret_cast<const int32 *>(data);
				int32 ptCount = *reinterpret_cast<const int32 *>(data + sizeof(int32));
				const uint32 *opList = reinterpret_cast<const uint32 *>(data + 2 * sizeof(int32));
				const BPoint *ptList = reinterpret_cast<const BPoint *>(data + 2 * sizeof(int32) + opCount * sizeof(uint32));

				// TODO: remove BShape data copying
				BShape shape; 
				shape.SetData(opCount, ptCount, opList, ptList);

				const int32 tableIndex = stroke ? 15 : 16;
				((fnc_BShape)callBackTable[tableIndex])(userData, &shape);
				break;
			}
			
			case B_PIC_DRAW_STRING:
			{
				if (tableEntries <= 17)
					break;
				((fnc_Pcff)callBackTable[17])(userData, 
					reinterpret_cast<const char *>(data + 2 * sizeof(float)), /* string */
					*reinterpret_cast<const float *>(data), /* escapement.space */
					*reinterpret_cast<const float *>(data + sizeof(float))); /* escapement.nonspace */
				break;
			}

			case B_PIC_DRAW_PIXELS:
			{
				if (tableEntries <= 18)
					break;
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
				if (tableEntries <= 19)
					break;
				((fnc_DrawPicture)callBackTable[19])(userData,
					*reinterpret_cast<const BPoint *>(data),
					*reinterpret_cast<const int32 *>(data + sizeof(BPoint)));
				break;
			}
			
			case B_PIC_SET_CLIPPING_RECTS:
			{
				if (tableEntries <= 20)
					break;
				break;
			}

			case B_PIC_CLEAR_CLIPPING_RECTS:
			{
				if (tableEntries <= 20)
					break;
				((fnc_PBRecti)callBackTable[20])(userData, NULL, 0);
				break;
			}

			case B_PIC_CLIP_TO_PICTURE:
			{
				if (tableEntries <= 21)
					break;
				break;
			}

			case B_PIC_PUSH_STATE:
			{
				if (tableEntries <= 22)
					break;
				((fnc)callBackTable[22])(userData);
				break;
			}

			case B_PIC_POP_STATE:
			{
				if (tableEntries <= 23)
					break;
				((fnc)callBackTable[23])(userData);
				break;
			}

			case B_PIC_ENTER_STATE_CHANGE:
			{
				if (tableEntries <= 24)
					break;
				((fnc)callBackTable[24])(userData);
				break;
			}

			case B_PIC_ENTER_FONT_STATE:
			{
				if (tableEntries <= 26)
					break;
				((fnc)callBackTable[26])(userData);
				break;
			}

			case B_PIC_SET_ORIGIN:
			{
				if (tableEntries <= 28)
					break;
				((fnc_BPoint)callBackTable[28])(userData,
					*reinterpret_cast<const BPoint *>(data)); /* origin */
				break;
			}

			case B_PIC_SET_PEN_LOCATION:
			{
				if (tableEntries <= 29)
					break;
				((fnc_BPoint)callBackTable[29])(userData,
					*reinterpret_cast<const BPoint *>(data)); /* location */
				break;
			}

			case B_PIC_SET_DRAWING_MODE:
			{
				if (tableEntries <= 30)
					break;
				((fnc_s)callBackTable[30])(userData,
					*reinterpret_cast<const int16 *>(data)); /* mode */
				break;
			}

			case B_PIC_SET_LINE_MODE:
			{
				if (tableEntries <= 31)
					break;
				((fnc_ssf)callBackTable[31])(userData,
					*reinterpret_cast<const int16 *>(data), /* cap-mode */
					*reinterpret_cast<const int16 *>(data + 1 * sizeof(int16)), /* join-mode */
					*reinterpret_cast<const float *>(data + 2 * sizeof(int16))); /* miter-limit */
				break;
			}

			case B_PIC_SET_PEN_SIZE:
			{
				if (tableEntries <= 32)
					break;
				((fnc_f)callBackTable[32])(userData,
					*reinterpret_cast<const float *>(data)); /* size */
				break;
			}

			case B_PIC_SET_FORE_COLOR:
			{			
				if (tableEntries <= 33)
					break;
				((fnc_Color)callBackTable[33])(userData,
					*reinterpret_cast<const rgb_color *>(data)); /* color */
				break;
			}

			case B_PIC_SET_BACK_COLOR:
			{		
				if (tableEntries <= 34)
					break;	
				((fnc_Color)callBackTable[34])(userData,
					*reinterpret_cast<const rgb_color *>(data)); /* color */
				break;
			}

			case B_PIC_SET_STIPLE_PATTERN:
			{
				if (tableEntries <= 35)
					break;
				((fnc_Pattern)callBackTable[35])(userData,
					*reinterpret_cast<const pattern *>(data)); /* pattern */
				break;
			}

			case B_PIC_SET_SCALE:
			{
				if (tableEntries <= 36)
					break;
				((fnc_f)callBackTable[36])(userData,
					*reinterpret_cast<const float *>(data)); /* scale */
				break;
			}

			case B_PIC_SET_FONT_FAMILY:
			{
				if (tableEntries <= 37)
					break;
				((fnc_Pc)callBackTable[37])(userData,
					reinterpret_cast<const char *>(data)); /* string */
				break;
			}

			case B_PIC_SET_FONT_STYLE:
			{
				if (tableEntries <= 38)
					break;
				((fnc_Pc)callBackTable[38])(userData,
					reinterpret_cast<const char *>(data)); /* string */
				break;
			}

			case B_PIC_SET_FONT_SPACING:
			{
				if (tableEntries <= 39)
					break;
				((fnc_i)callBackTable[39])(userData,
					*reinterpret_cast<const int32 *>(data)); /* spacing */
				break;
			}

			case B_PIC_SET_FONT_SIZE:
			{
				if (tableEntries <= 40)
					break;
				((fnc_f)callBackTable[40])(userData,
					*reinterpret_cast<const float *>(data)); /* size */
				break;
			}

			case B_PIC_SET_FONT_ROTATE:
			{
				if (tableEntries <= 41)
					break;
				((fnc_f)callBackTable[41])(userData,
					*reinterpret_cast<const float *>(data)); /* rotation */
				break;
			}

			case B_PIC_SET_FONT_ENCODING:
			{
				if (tableEntries <= 42)
					break;
				((fnc_i)callBackTable[42])(userData,
					*reinterpret_cast<const int32 *>(data)); /* encoding */
				break;
			}

			case B_PIC_SET_FONT_FLAGS:
			{
				if (tableEntries <= 43)
					break;
				((fnc_i)callBackTable[43])(userData,
					*reinterpret_cast<const int32 *>(data)); /* flags */
				break;
			}
			
			case B_PIC_SET_FONT_SHEAR:
			{
				if (tableEntries <= 44)
					break;
				((fnc_f)callBackTable[44])(userData,
					*reinterpret_cast<const float *>(data)); /* shear */
				break;
			}

			case B_PIC_SET_FONT_FACE:
			{
				if (tableEntries <= 46)
					break;
				((fnc_i)callBackTable[46])(userData,
					*reinterpret_cast<const int32 *>(data)); /* flags */
				break;
			}

			case B_PIC_SET_BLENDING_MODE:
			{
				if (tableEntries <= 47)
					break;
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
