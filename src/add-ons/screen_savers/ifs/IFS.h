/*
 * Copyright (c) 1997 by Massimino Pascal <Pascal.Massimon@ens.fr>
 * Copyright 2006-2014, Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Massimino Pascal, Pascal.Massimon@ens.fr
 *		John Scipione, jscipione@gmail.com
 */
#ifndef IFS_H
#define IFS_H


#include <Screen.h>
#include <Point.h>
#include <Rect.h>
#include <Region.h>
#include <View.h>


#define FIX 12
#define UNIT   ( 1<<FIX )
#define MAX_SIMILITUDE  6

// settings for a PC 120Mhz...
#define MAX_DEPTH_2  10
#define MAX_DEPTH_3  6
#define MAX_DEPTH_4  4
#define MAX_DEPTH_5  3


struct buffer_info {
	void*			bits;
	uint32			bytesPerRow;
	uint32			bits_per_pixel;
	color_space		format;
	clipping_rect	bounds;
};

struct Point {
	int32		x;
	int32		y;
};

typedef struct Similitude {
	float		c_x;
	float		c_y;
	float		r;
	float		r2;
	float		A;
	float		A2;
	int32		Ct;
	int32		St;
	int32		Ct2;
	int32		St2;
	int32		Cx;
	int32		Cy;
	int32		R;
	int32		R2;
} SIMILITUDE;

typedef struct Fractal {
	int			SimilitudeCount;
	SIMILITUDE	Components[5 * MAX_SIMILITUDE];
	int			Depth;
	int			Col;
	int			Count;
	int			Speed;
	int			Width;
	int			Height;
	int			Lx;
	int			Ly;
	float		r_mean;
	float		dr_mean;
	float		dr2_mean;
	int			CurrentPoint;
	int			MaxPoint;
	Point*		buffer1;
	Point*		buffer2;
	BBitmap*	bitmap;
	BBitmap*	markBitmap;
} FRACTAL;


class IFS {
public:
								IFS(BRect bounds);
	virtual						~IFS();

			void				Draw(BView* view, const buffer_info* info,
									 int32 frames);

			void				SetAdditive(bool additive);
			void				SetSpeed(int32 speed);

private:
			void				_DrawFractal(BView* view,
									const buffer_info* info);
			void				_Trace(FRACTAL* F, int32 xo, int32 yo);
			void				_RandomSimilitudes(FRACTAL* f, SIMILITUDE* cur,
									int i) const;
			void				_FreeBuffers(FRACTAL* f);
			void				_FreeIFS(FRACTAL* f);

			FRACTAL*			fRoot;
			FRACTAL*			fCurrentFractal;
			Point*				fPointBuffer;
			int32				fCurrentPoint;

			bool				fAdditive;
			uint8				fCurrentMarkValue;
};


#endif	// IFS_H
