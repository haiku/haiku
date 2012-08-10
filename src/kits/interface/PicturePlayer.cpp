/*
 * Copyright 2001-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Marcus Overhagen (marcus@overhagen.de)
 */

/**	PicturePlayer is used to play picture data. */

#include <stdio.h>
#include <string.h>

#include <PicturePlayer.h>
#include <PictureProtocol.h>

#include <Shape.h>

using BPrivate::PicturePlayer;

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
typedef void (*fnc_PBRecti)(void*, const BRect*, uint32);
typedef void (*fnc_DrawPixels)(void *, BRect, BRect, int32, int32, int32,
							   int32, int32, const void *);
typedef void (*fnc_DrawPicture)(void *, BPoint, int32);
typedef void (*fnc_BShape)(void*, BShape*);


static void
nop()
{
}


#if DEBUG > 1
static const char *
PictureOpToString(int op)
{
	#define RETURN_STRING(x) case x: return #x

	switch(op) {
		RETURN_STRING(B_PIC_MOVE_PEN_BY);
		RETURN_STRING(B_PIC_STROKE_LINE);
		RETURN_STRING(B_PIC_STROKE_RECT);
		RETURN_STRING(B_PIC_FILL_RECT);
		RETURN_STRING(B_PIC_STROKE_ROUND_RECT);
		RETURN_STRING(B_PIC_FILL_ROUND_RECT);
		RETURN_STRING(B_PIC_STROKE_BEZIER);
		RETURN_STRING(B_PIC_FILL_BEZIER);
		RETURN_STRING(B_PIC_STROKE_POLYGON);
		RETURN_STRING(B_PIC_FILL_POLYGON);
		RETURN_STRING(B_PIC_STROKE_SHAPE);
		RETURN_STRING(B_PIC_FILL_SHAPE);
		RETURN_STRING(B_PIC_DRAW_STRING);
		RETURN_STRING(B_PIC_DRAW_PIXELS);
		RETURN_STRING(B_PIC_DRAW_PICTURE);
		RETURN_STRING(B_PIC_STROKE_ARC);
		RETURN_STRING(B_PIC_FILL_ARC);
		RETURN_STRING(B_PIC_STROKE_ELLIPSE);
		RETURN_STRING(B_PIC_FILL_ELLIPSE);

		RETURN_STRING(B_PIC_ENTER_STATE_CHANGE);
		RETURN_STRING(B_PIC_SET_CLIPPING_RECTS);
		RETURN_STRING(B_PIC_CLIP_TO_PICTURE);
		RETURN_STRING(B_PIC_PUSH_STATE);
		RETURN_STRING(B_PIC_POP_STATE);
		RETURN_STRING(B_PIC_CLEAR_CLIPPING_RECTS);

		RETURN_STRING(B_PIC_SET_ORIGIN);
		RETURN_STRING(B_PIC_SET_PEN_LOCATION);
		RETURN_STRING(B_PIC_SET_DRAWING_MODE);
		RETURN_STRING(B_PIC_SET_LINE_MODE);
		RETURN_STRING(B_PIC_SET_PEN_SIZE);
		RETURN_STRING(B_PIC_SET_SCALE);
		RETURN_STRING(B_PIC_SET_FORE_COLOR);
		RETURN_STRING(B_PIC_SET_BACK_COLOR);
		RETURN_STRING(B_PIC_SET_STIPLE_PATTERN);
		RETURN_STRING(B_PIC_ENTER_FONT_STATE);
		RETURN_STRING(B_PIC_SET_BLENDING_MODE);
		RETURN_STRING(B_PIC_SET_FONT_FAMILY);
		RETURN_STRING(B_PIC_SET_FONT_STYLE);
		RETURN_STRING(B_PIC_SET_FONT_SPACING);
		RETURN_STRING(B_PIC_SET_FONT_ENCODING);
		RETURN_STRING(B_PIC_SET_FONT_FLAGS);
		RETURN_STRING(B_PIC_SET_FONT_SIZE);
		RETURN_STRING(B_PIC_SET_FONT_ROTATE);
		RETURN_STRING(B_PIC_SET_FONT_SHEAR);
		RETURN_STRING(B_PIC_SET_FONT_BPP);
		RETURN_STRING(B_PIC_SET_FONT_FACE);
		default: return "Unknown op";
	}
	#undef RETURN_STRING
}
#endif


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
#if DEBUG
	FILE *file = fopen("/var/log/PicturePlayer.log", "a");
	fprintf(file, "Start rendering BPicture...\n");
	bigtime_t startTime = system_time();
	int32 numOps = 0;
#endif
	// If the caller supplied a function table smaller than needed,
	// we use our dummy table, and copy the supported ops from the supplied one.
	void **functionTable = callBackTable;
	void *dummyTable[kOpsTableSize] = {
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop,
		(void *)nop, (void *)nop, (void *)nop, (void *)nop
	};

	if ((uint32)tableEntries < kOpsTableSize) {
#if DEBUG
		fprintf(file, "PicturePlayer: A smaller than needed function table was supplied.\n");
#endif
		functionTable = dummyTable;
		memcpy(functionTable, callBackTable, tableEntries * sizeof(void *));
	}

	const char *data = reinterpret_cast<const char *>(fData);
	size_t pos = 0;

	int32 fontStateBlockSize = -1;
	int32 stateBlockSize = -1;

	while ((pos + 6) <= fSize) {
		int16 op = *reinterpret_cast<const int16 *>(data);
		size_t size = *reinterpret_cast<const size_t *>(data + sizeof(int16));
		pos += sizeof(int16) + sizeof(size_t);
		data += sizeof(int16) + sizeof(size_t);

		if (pos + size > fSize)
			debugger("PicturePlayer::Play: buffer overrun\n");

#if DEBUG > 1
		bigtime_t startOpTime = system_time();
		fprintf(file, "Op %s ", PictureOpToString(op));
#endif
		switch (op) {
			case B_PIC_MOVE_PEN_BY:
			{
				((fnc_BPoint)functionTable[1])(userData,
					*reinterpret_cast<const BPoint *>(data)); /* where */
				break;
			}

			case B_PIC_STROKE_LINE:
			{
				((fnc_BPointBPoint)functionTable[2])(userData,
					*reinterpret_cast<const BPoint *>(data), /* start */
					*reinterpret_cast<const BPoint *>(data + sizeof(BPoint))); /* end */
				break;
			}

			case B_PIC_STROKE_RECT:
			{
				((fnc_BRect)functionTable[3])(userData,
					*reinterpret_cast<const BRect *>(data)); /* rect */
				break;
			}

			case B_PIC_FILL_RECT:
			{
				((fnc_BRect)functionTable[4])(userData,
					*reinterpret_cast<const BRect *>(data)); /* rect */
				break;
			}

			case B_PIC_STROKE_ROUND_RECT:
			{
				((fnc_BRectBPoint)functionTable[5])(userData,
					*reinterpret_cast<const BRect *>(data), /* rect */
					*reinterpret_cast<const BPoint *>(data + sizeof(BRect))); /* radii */
				break;
			}

			case B_PIC_FILL_ROUND_RECT:
			{
				((fnc_BRectBPoint)functionTable[6])(userData,
					*reinterpret_cast<const BRect *>(data), /* rect */
					*reinterpret_cast<const BPoint *>(data + sizeof(BRect))); /* radii */
				break;
			}

			case B_PIC_STROKE_BEZIER:
			{
				((fnc_PBPoint)functionTable[7])(userData,
					reinterpret_cast<const BPoint *>(data));
				break;
			}

			case B_PIC_FILL_BEZIER:
			{
				((fnc_PBPoint)functionTable[8])(userData,
					reinterpret_cast<const BPoint *>(data));
				break;
			}

			case B_PIC_STROKE_ARC:
			{
				((fnc_BPointBPointff)functionTable[9])(userData,
					*reinterpret_cast<const BPoint *>(data), /* center */
					*reinterpret_cast<const BPoint *>(data + sizeof(BPoint)), /* radii */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint)), /* startTheta */
					*reinterpret_cast<const float *>(data + 2 * sizeof(BPoint) + sizeof(float))); /* arcTheta */
				break;
			}

			case B_PIC_FILL_ARC:
			{
				((fnc_BPointBPointff)functionTable[10])(userData,
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
				((fnc_BPointBPoint)functionTable[11])(userData, center, radii);
				break;
			}

			case B_PIC_FILL_ELLIPSE:
			{
				const BRect *rect = reinterpret_cast<const BRect *>(data);
				BPoint radii((rect->Width() + 1) / 2.0f, (rect->Height() + 1) / 2.0f);
				BPoint center = rect->LeftTop() + radii;
				((fnc_BPointBPoint)functionTable[12])(userData, center, radii);
				break;
			}

			case B_PIC_STROKE_POLYGON:
			{
				int32 numPoints = *reinterpret_cast<const int32 *>(data);
				((fnc_iPBPointb)functionTable[13])(userData,
					numPoints,
					reinterpret_cast<const BPoint *>(data + sizeof(int32)), /* points */
					*reinterpret_cast<const uint8 *>(data + sizeof(int32) + numPoints * sizeof(BPoint))); /* is-closed */
				break;
			}

			case B_PIC_FILL_POLYGON:
			{
				((fnc_iPBPoint)functionTable[14])(userData,
					*reinterpret_cast<const int32 *>(data), /* numPoints */
					reinterpret_cast<const BPoint *>(data + sizeof(int32))); /* points */
				break;
			}

			case B_PIC_STROKE_SHAPE:
			case B_PIC_FILL_SHAPE:
			{
				const bool stroke = (op == B_PIC_STROKE_SHAPE);
				int32 opCount = *reinterpret_cast<const int32 *>(data);
				int32 ptCount = *reinterpret_cast<const int32 *>(data + sizeof(int32));
				const uint32 *opList = reinterpret_cast<const uint32 *>(data + 2 * sizeof(int32));
				const BPoint *ptList = reinterpret_cast<const BPoint *>(data + 2 * sizeof(int32) + opCount * sizeof(uint32));

				// TODO: remove BShape data copying
				BShape shape;
				shape.SetData(opCount, ptCount, opList, ptList);

				const int32 tableIndex = stroke ? 15 : 16;
				((fnc_BShape)functionTable[tableIndex])(userData, &shape);
				break;
			}

			case B_PIC_DRAW_STRING:
			{
				((fnc_Pcff)functionTable[17])(userData,
					reinterpret_cast<const char *>(data + 2 * sizeof(float)), /* string */
					*reinterpret_cast<const float *>(data), /* escapement.space */
					*reinterpret_cast<const float *>(data + sizeof(float))); /* escapement.nonspace */
				break;
			}

			case B_PIC_DRAW_PIXELS:
			{
				((fnc_DrawPixels)functionTable[18])(userData,
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
				((fnc_DrawPicture)functionTable[19])(userData,
					*reinterpret_cast<const BPoint *>(data),
					*reinterpret_cast<const int32 *>(data + sizeof(BPoint)));
				break;
			}

			case B_PIC_SET_CLIPPING_RECTS:
			{
				// TODO: Not sure if it's compatible with R5's BPicture version
				const uint32 numRects = *reinterpret_cast<const uint32 *>(data);
				const BRect *rects = reinterpret_cast<const BRect *>(data + sizeof(uint32));
				((fnc_PBRecti)functionTable[20])(userData, rects, numRects);

				break;
			}

			case B_PIC_CLEAR_CLIPPING_RECTS:
			{
				((fnc_PBRecti)functionTable[20])(userData, NULL, 0);
				break;
			}

			case B_PIC_CLIP_TO_PICTURE:
			{
				// TODO: Implement
				break;
			}

			case B_PIC_PUSH_STATE:
			{
				((fnc)functionTable[22])(userData);
				break;
			}

			case B_PIC_POP_STATE:
			{
				((fnc)functionTable[23])(userData);
				break;
			}

			case B_PIC_ENTER_STATE_CHANGE:
			{
				((fnc)functionTable[24])(userData);
				stateBlockSize = size;
				break;
			}

			case B_PIC_ENTER_FONT_STATE:
			{
				((fnc)functionTable[26])(userData);
				fontStateBlockSize = size;
				break;
			}

			case B_PIC_SET_ORIGIN:
			{
				((fnc_BPoint)functionTable[28])(userData,
					*reinterpret_cast<const BPoint *>(data)); /* origin */
				break;
			}

			case B_PIC_SET_PEN_LOCATION:
			{
				((fnc_BPoint)functionTable[29])(userData,
					*reinterpret_cast<const BPoint *>(data)); /* location */
				break;
			}

			case B_PIC_SET_DRAWING_MODE:
			{
				((fnc_s)functionTable[30])(userData,
					*reinterpret_cast<const int16 *>(data)); /* mode */
				break;
			}

			case B_PIC_SET_LINE_MODE:
			{
				((fnc_ssf)functionTable[31])(userData,
					*reinterpret_cast<const int16 *>(data), /* cap-mode */
					*reinterpret_cast<const int16 *>(data + 1 * sizeof(int16)), /* join-mode */
					*reinterpret_cast<const float *>(data + 2 * sizeof(int16))); /* miter-limit */
				break;
			}

			case B_PIC_SET_PEN_SIZE:
			{
				((fnc_f)functionTable[32])(userData,
					*reinterpret_cast<const float *>(data)); /* size */
				break;
			}

			case B_PIC_SET_FORE_COLOR:
			{
				((fnc_Color)functionTable[33])(userData,
					*reinterpret_cast<const rgb_color *>(data)); /* color */
				break;
			}

			case B_PIC_SET_BACK_COLOR:
			{
				((fnc_Color)functionTable[34])(userData,
					*reinterpret_cast<const rgb_color *>(data)); /* color */
				break;
			}

			case B_PIC_SET_STIPLE_PATTERN:
			{
				((fnc_Pattern)functionTable[35])(userData,
					*reinterpret_cast<const pattern *>(data)); /* pattern */
				break;
			}

			case B_PIC_SET_SCALE:
			{
				((fnc_f)functionTable[36])(userData,
					*reinterpret_cast<const float *>(data)); /* scale */
				break;
			}

			case B_PIC_SET_FONT_FAMILY:
			{
				((fnc_Pc)functionTable[37])(userData,
					reinterpret_cast<const char *>(data)); /* string */
				break;
			}

			case B_PIC_SET_FONT_STYLE:
			{
				((fnc_Pc)functionTable[38])(userData,
					reinterpret_cast<const char *>(data)); /* string */
				break;
			}

			case B_PIC_SET_FONT_SPACING:
			{
				((fnc_i)functionTable[39])(userData,
					*reinterpret_cast<const int32 *>(data)); /* spacing */
				break;
			}

			case B_PIC_SET_FONT_SIZE:
			{
				((fnc_f)functionTable[40])(userData,
					*reinterpret_cast<const float *>(data)); /* size */
				break;
			}

			case B_PIC_SET_FONT_ROTATE:
			{
				((fnc_f)functionTable[41])(userData,
					*reinterpret_cast<const float *>(data)); /* rotation */
				break;
			}

			case B_PIC_SET_FONT_ENCODING:
			{
				((fnc_i)functionTable[42])(userData,
					*reinterpret_cast<const int32 *>(data)); /* encoding */
				break;
			}

			case B_PIC_SET_FONT_FLAGS:
			{
				((fnc_i)functionTable[43])(userData,
					*reinterpret_cast<const int32 *>(data)); /* flags */
				break;
			}

			case B_PIC_SET_FONT_SHEAR:
			{
				((fnc_f)functionTable[44])(userData,
					*reinterpret_cast<const float *>(data)); /* shear */
				break;
			}

			case B_PIC_SET_FONT_FACE:
			{
				((fnc_i)functionTable[46])(userData,
					*reinterpret_cast<const int32 *>(data)); /* flags */
				break;
			}

			case B_PIC_SET_BLENDING_MODE:
			{
				((fnc_ss)functionTable[47])(userData,
					*reinterpret_cast<const int16 *>(data), /* alphaSrcMode */
					*reinterpret_cast<const int16 *>(data + sizeof(int16))); /* alphaFncMode */
				break;
			}

			default:
				break;
		}

		// Skip the already handled block unless it's one of these two,
		// since they can contain other nested ops.
		if (op != B_PIC_ENTER_STATE_CHANGE && op != B_PIC_ENTER_FONT_STATE) {
			pos += size;
			data += size;
			if (stateBlockSize > 0)
				stateBlockSize -= size + 6;
			if (fontStateBlockSize > 0)
				fontStateBlockSize -= size + 6;
		}

		// call the exit_state_change hook if needed
		if (stateBlockSize == 0) {
			((fnc)functionTable[25])(userData);
			stateBlockSize = -1;
		}

		// call the exit_font_state hook if needed
		if (fontStateBlockSize == 0) {
			((fnc)functionTable[27])(userData);
			fontStateBlockSize = -1;
		}
#if DEBUG
		numOps++;
#if DEBUG > 1
		fprintf(file, "executed in %lld usecs\n", system_time() - startOpTime);
#endif
#endif
		// TODO: what if too much was read, should we return B_ERROR?
	}

#if DEBUG
	fprintf(file, "Done! %ld ops, rendering completed in %lld usecs.\n", numOps, system_time() - startTime);
	fclose(file);
#endif
	return B_OK;
}
