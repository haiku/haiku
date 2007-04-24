//******************************************************************************
//
//	File:		main.cpp
//
//	Description:	show pixmap main test program.
//
//	Written by:	Benoit Schillings
//
//	Change History:
//
//	7/31/92		bgs	new today
//
//******************************************************************************

/*
	Copyright 1992-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <math.h>
#include <OS.h>

#include "tsb.h"

/*------------------------------------------------------------*/

TShowBit	*tsbb;

/*------------------------------------------------------------*/

uchar	palette[256];

/*------------------------------------------------------------*/

void	TShowBit::MouseDown(BPoint where)
{
	if ((Modifiers() & SHIFT_KEY) == 0) {
		change_selection(where.x, where.y);
	}
	redraw_mand();
}

/*------------------------------------------------------------*/

void	TShowBit::redraw_mand()
{
	float	px0;
	float	py0;
	float	scale0;

	if (Modifiers() & SHIFT_KEY) {
		px -= (scale / 2.0);
		py -= (scale / 2.0);
		scale *= 2.0;
	}
	else {
		px0 = px + (scale * (selection.left / (1.0*size_x)));
		py0 = py + (scale * (selection.top / (1.0*size_x)));
		scale0 = scale * ((selection.bottom-selection.top) / (1.0*size_x));

		px = px0; py = py0; scale = scale0;	
	}

	selection.Set(-1000, -1000, -1000, -1000);
	mand(px, py, scale, scale);
}

/*------------------------------------------------------------*/

void	TShowBit::set_palette(long code)
{
	rgb_color	c;
	long		i;

	if (code == 0) {
		for (i = 0; i < 256; i++)
			palette[i] = (i >> 1) & 0x1f;
	}
	if (code == 1) {
		for (i = 0; i < 256; i++) {
			c.red = i;
			c.green = 0;
			c.blue = 256-i;
			palette[i] = index_for_color(c);
		}
	}

	if (code == 2) {
		for (i = 0; i < 256; i++) {
			c.red = i;
			c.green = i/2;
			c.blue = 256-i;
			palette[i] = index_for_color(c);
		}
	}

	if (code == 3) {
		for (i = 0; i < 256; i++) {
			c.red = 256-i;
			c.green = i;
			c.blue = 0;
			palette[i] = index_for_color(c);
		}
	}
}

/*------------------------------------------------------------*/

void	TShowBit::TShowBit(BRect r, long flags) :
	BView(r, "", flags, WILL_DRAW)
{
	BRect	bitmap_r;
	long	ref;
	long	i;
	char	*bits;

	tsbb = this;
	set_palette(0);
	exit_now = 0;
	bitmap_r.Set(0, 0, size_x - 1, size_y - 1);
	selection.Set(-1000, -1000, -1000, -1000);

	the_bitmap = new BBitmap(bitmap_r, COLOR_8_BIT);
	bits = (char *)the_bitmap->Bits();
	memset(bits, 0x00, size_x*size_y);
	px = -1.5;
	py = -1.5;
	scale = 3.0;
}

/*------------------------------------------------------------*/

TShowBit::~TShowBit()
{
	delete	the_bitmap;
}

/*------------------------------------------------------------*/

void	TShowBit::Draw(BRect update_rect)
{
	DrawBitmap(the_bitmap, BPoint(0, 0));
}


/*------------------------------------------------------------*/

void	TShowBit::demo()
{
}

/*------------------------------------------------------------*/
extern "C" int iterate(float a, float b);
/*------------------------------------------------------------*/

float	vvx;
float	vvy;
float	ssx;
char	t1_done;
char	t2_done;

/*------------------------------------------------------------*/

void	__calc1()
{
	tsbb->manda(vvx, vvy, ssx, ssx);
}

/*------------------------------------------------------------*/

void	__calc2()
{
	tsbb->mandb(vvx, vvy, ssx, ssx);
}

/*------------------------------------------------------------*/

uchar	tmp[256];
uchar	pc[32][32];
uchar	tmp1[256];

/*------------------------------------------------------------*/

void	TShowBit::mand(float vx, float vy, float sx, float sy)
{
	vvx = vx; vvy = vy; ssx = sx;
	t1_done = 0; t2_done = 0;

	precompute(vx, vy, sx, sy);
	
	resume_thread(spawn_thread(__calc1, "calc1", 20, 0, 0));
	resume_thread(spawn_thread(__calc2, "calc2", 20, 0, 0));
	while(1) {
		snooze(100000);
		Draw(BRect(0,0,0,0));
		if (t1_done && t2_done)
			break;
	}
	Draw(BRect(0,0,0,0));
}

/*------------------------------------------------------------*/

void	TShowBit::precompute(float vx, float vy, float sx, float sy)
{
	long	x, y;
	long	i;
	long	nx, ny;
	float	cx, cy;
	char	v;
	char	*p;
	
	sx = sx / (32.0);
	sy = sy / (32.0);
	cy = vy;

	for (y = 0; y < 32; y++) {
		cy += sy;
		cx = vx;
		for (x = 0; x < 32; x++) {
			cx += sx;
			pc[x][y] = iterate(cx, cy);
		}
	}
}

/*------------------------------------------------------------*/

void	TShowBit::mandb(float vx, float vy, float sx, float sy)
{
	long	x, y;
	long	bx;
	long	i;
	long	nx, ny;
	float	cx, cy;
	char	v;
	char	*p;
	uchar	*bits = (char *)the_bitmap->Bits();
	uchar	*b0;
	long	y12;
	long	x12;
	
	sx = sx / (size_x * 1.0);
	sy = sy / (size_y * 1.0);
	cy = vy;

	cy += sy;
	sy *= 2.0;
	for (y = 1; y < size_y; y+=2) {
		y12 = y / 12;
		cy += sy;
		cx = vx;
		b0 = bits + (y * size_x);
		for (bx = 0; bx < size_x; bx += 12) {
			for (x = bx; x < (bx+12); x++) {
				cx += sx;
				v = iterate(cx, cy);
				*b0++ = palette[v];
			}
		}
	}
	t2_done = 1;
}

/*------------------------------------------------------------*/


void	TShowBit::manda(float vx, float vy, float sx, float sy)
{
	long	x, y;
	long	i;
	long	nx, ny;
	long	bx;
	float	cx, cy;
	char	v;
	char	*p;
	uchar	*bits = (char *)the_bitmap->Bits();
	uchar	*b0;
	long	y12;
	long	x12;
	
	sx = sx / (size_x * 1.0);
	sy = sy / (size_y * 1.0);
	cy = vy;

	sy *= 2.0;
	for (y = 0; y < size_y; y+=2) {
		y12 = y / 12;
		cy += sy;
		cx = vx;
		b0 = bits + (y * size_x);
		for (bx = 0; bx < size_x; bx += 12) {
			for (x = bx; x < (bx+12); x++) {
				cx += sx;
				v = iterate(cx, cy);
				*b0++ = palette[v];
			}
		}
	}
	t1_done = 1;
}


/*------------------------------------------------------------*/

long	TShowBit::limit_v(long v)
{
	if (v > (size_y - 1))
			v = (size_y - 1);

	if (v < 0)
			v = 0;
	return(v);
}

/*------------------------------------------------------------*/

long	TShowBit::limit_h(long v)
{
	if (v > (size_x - 1))
			v = size_x - 1;

	if (v < 0)
			v = 0;
	return(v);
}

/*------------------------------------------------------------*/

BRect	TShowBit::sort_rect(BRect *aRect)
{
	BRect	tmp_rect;
	long	tmp;

	tmp_rect = *aRect;
	if (tmp_rect.bottom < tmp_rect.top) {
		tmp = tmp_rect.top;
		tmp_rect.top = tmp_rect.bottom;
		tmp_rect.bottom = tmp;
	}

	if (tmp_rect.left > tmp_rect.right) {
		tmp = tmp_rect.right;
		tmp_rect.right = tmp_rect.left;
		tmp_rect.left = tmp;
	}

	tmp_rect.top = limit_v(tmp_rect.top);
	tmp_rect.left = limit_h(tmp_rect.left);
	tmp_rect.bottom = limit_v(tmp_rect.bottom);
	tmp_rect.right = limit_h(tmp_rect.right);

	return(tmp_rect);
}

/*------------------------------------------------------------*/

void	TShowBit::clip(long *h, long *v)
{
	if (*h > (size_x - 1))
			*h = (size_x - 1);
	if (*h < 0)
			*h = 0;
	if (*v > (size_y - 1))
			*v = size_y - 1;
	if (*v < 0)
			*v = 0;
}

/*------------------------------------------------------------*/

char	TShowBit::has_selection()
{
	if (((selection.bottom - selection.top) + (selection.right - selection.left)) < 5) 
		return 0;
	else
		return 1;
}

/*------------------------------------------------------------*/

void	TShowBit::change_selection(long h, long v)
{
	ulong	buttons;
	long	h0;
	long	v0;
	BRect	new_select;
	BRect	old_select;
	BRect	tmp_rect;
	long	max;
	long	width, height;
	
	clip(&h, &v);
	new_select.top = v;
	new_select.left = h;
	old_select = selection;

	SetDrawingMode(OP_INVERT);

	do {
		BPoint where;
		GetMouse(&where, &buttons);
		h0 = where.x;
		v0 = where.y;
		width = h0 - h;
		height = v0 - v;
		if (height > width)
			max = height;
		else
			max = height;

		h0 = h+max; v0 = v+max;

		clip(&h0, &v0);
		new_select.right = h0;
		new_select.bottom = v0;

		if ((old_select.top != new_select.top) || 
		    (old_select.bottom != new_select.bottom) ||
		    (old_select.right != new_select.right) ||
		    (old_select.left != new_select.left)) {
		
			tmp_rect = sort_rect(&new_select);
			StrokeRect(tmp_rect);

			tmp_rect = sort_rect(&old_select);
			StrokeRect(tmp_rect);

			old_select = new_select;
			Flush();
		}

		sleept(20);
	} while(buttons);

	selection = sort_rect(&new_select);
	if (!has_selection()) {
		StrokeRect(selection);
		selection.Set(-1000, -1000, -1000, -1000);
	} 
	SetDrawingMode(OP_COPY);
}
