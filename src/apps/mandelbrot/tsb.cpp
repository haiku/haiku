//******************************************************************************
//
//	File:		tsb.cpp
//
//******************************************************************************

/*
	Copyright 1993-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Debug.h>
#include <Window.h>

#include <string.h>
#include <math.h>
#include <OS.h>
#include <Screen.h>

#include "tsb.h"

#ifndef _INTERFACE_DEFS_H
#include <InterfaceDefs.h>
#endif

/*------------------------------------------------------------*/

TShowBit	*tsbb;

/*------------------------------------------------------------*/
long	niter = 256; 
/*------------------------------------------------------------*/

uchar	palette[256];

/*------------------------------------------------------------*/

void	TShowBit::MouseDown(BPoint where)
{
	if (!this->Window()->IsActive()) {
		this->Window()->Activate(TRUE);
		this->Window()->UpdateIfNeeded();
	}

	if (busy)
		return;

	if ((modifiers() & B_SHIFT_KEY) == 0) {
		change_selection(where.x, where.y);
		if ((selection.bottom - selection.top) < 4)
			return;
	}
	redraw_mand();
}

/*------------------------------------------------------------*/

void	TShowBit::redraw_mand()
{
	double	px0;
	double	py0;
	double	scale0;

	if (modifiers() & B_SHIFT_KEY) {
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

void	TShowBit::set_iter(long it)
{
	if (it != iter) {
		iter = it;
		niter = it;
		selection.Set(-1000, -1000, -1000, -1000);
		mand(px, py, scale, scale);
	}
}

/*------------------------------------------------------------*/

void	TShowBit::set_palette(long code)
{
	rgb_color	c = {0, 0, 0, 255};
	long		i;
	
	BScreen screen( Window() );

	if (code == 0) {
		for (i = 0; i < 256; i++)
			palette[i] = (i >> 1) & 0x1f;
	}
	if (code == 1) {
		for (i = 0; i < 256; i++) {
			c.red = i * 4;
			c.green = i * 7;
			c.blue = 256-(i - i * 5);
			palette[i] = screen.IndexForColor(c);
		}
	}

	if (code == 2) {
		for (i = 0; i < 256; i++) {
			c.red = (i * 7);
			c.green = i/2;
			c.blue = 256-(i * 3);
			palette[i] = screen.IndexForColor(c);
		}
	}

	if (code == 3) {
		for (i = 0; i < 256; i++) {
			c.red = 256-(i * 6);
			c.green = (i * 7);
			c.blue = 0;
			palette[i] = screen.IndexForColor(c);
		}
	}
	mand(px, py, scale, scale);
}

/*------------------------------------------------------------*/

TShowBit::TShowBit(BRect r, uint32 resizeMask, uint32 flags) :
	BView(r, "", resizeMask, flags | B_WILL_DRAW | B_PULSE_NEEDED)
{
	BRect	bitmap_r;
	char	*bits;

	busy = FALSE;
	exit_now = FALSE;
	tsbb = this;
	bitmap_r.Set(0, 0, size_x - 1, size_y - 1);
	selection.Set(-1000, -1000, -1000, -1000);
	iter = 256;

	the_bitmap = new BBitmap(bitmap_r, B_COLOR_8_BIT);
	bits = (char *)the_bitmap->Bits();
	memset(bits, 0x00, size_x*size_y);
	px = -2.5;
	py = -2.0;
	scale = 4.0;
	set_palette(2);
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


int iterate_double(double a, double b)
{
	double	x;
	double	y;
	double	xsq;
	double	ysq;
	double	ctwo = 2.0, cfour = 4.0;
	int		i = 0, iter = niter;

	x = 0.0;
	y = 0.0;

	while (i < iter) {
		xsq = x * x;
		ysq = y * y;
		y = (ctwo * x * y) + b;
		i++;
		x = a + (xsq - ysq);
	
		if ((xsq + ysq) > cfour)
			return(i);
	}
	return(i);
}

/*------------------------------------------------------------*/
//extern "C" int iterate(float a, float b);
/*------------------------------------------------------------*/

int iterate_float(float a, float b)
{
	float	x;
	float	y;
	float	xsq;
	float	ysq;
// These are variables, because the metaware compiler would reload the
// constants from memory each time through the loop rather than leave them
// in registers.  Lovely.
	float	ctwo = 2.0, cfour = 4.0;
	long	i;
	int		iter = niter;

	x = 0.0;
	y = 0.0;
	i = 0;

	while (i < iter) {
		xsq = x * x;
		ysq = y * y;
		y = (ctwo * x * y) + b;
		i++;
		x = a + (xsq - ysq);
	
		if ((xsq + ysq) > cfour)
			return(i);
	}
	return(i);
}

/*------------------------------------------------------------*/

double	vvx;
double	vvy;
double	ssx;
char	t1_done;
char	t2_done;

/*------------------------------------------------------------*/

long	__calc1(void *arg)
{
	tsbb->manda(vvx, vvy, ssx, ssx);
	return B_NO_ERROR;
}

/*------------------------------------------------------------*/

long	__calc2(void *arg)
{
	tsbb->mandb(vvx, vvy, ssx, ssx);
	return B_NO_ERROR;
}

/*------------------------------------------------------------*/

uchar	tmp[256];
uchar	pc[32][32];
uchar	tmp1[256];

/*------------------------------------------------------------*/

void	TShowBit::mand(double vx, double vy, double sx, double sy)
{
	vvx = vx; vvy = vy; ssx = sx;
	t1_done = 0; t2_done = 0;

	precompute(vx, vy, sx, sy);
	
	resume_thread(spawn_thread(__calc1, "calc1", B_NORMAL_PRIORITY, NULL));
	resume_thread(spawn_thread(__calc2, "calc2", B_NORMAL_PRIORITY, NULL));
	busy = TRUE;
}

/*------------------------------------------------------------*/

void	TShowBit::Pulse()
{
//	PRINT(("pulsing (%d)\n", busy));
	if (busy) {
		Draw(BRect(0,0,0,0));
		if (t1_done && t2_done) {
			busy = FALSE;
			exit_now = FALSE;
		}
	}
}

/*------------------------------------------------------------*/

void	TShowBit::precompute(double vx, double vy, double sx, double sy)
{
	long	x, y;
	double	cx, cy;
	double	scale = sx;	

	sx = sx / (32.0);
	sy = sy / (32.0);
	cy = vy;

	for (y = 0; y < 32; y++) {
		cy += sy;
		cx = vx;
		if (scale < 0.000025 || niter != 256) {
			for (x = 0; x < 32; x++) {
				cx += sx;
				pc[x][y] = iterate_double(cx, cy);
			}
		}
		else
		for (x = 0; x < 32; x++) {
			cx += sx;
			pc[x][y] = iterate_float(cx, cy);
		}
	}
}

/*------------------------------------------------------------*/

void	TShowBit::mandb(double vx, double vy, double sx, double sy)
{
	long	x, y;
	long	bx;
	double	cx, cy;
	int		v;
	uchar	*bits = (uchar *)the_bitmap->Bits();
	uchar	*b0;
	long	y12;
	long	x12;
	double	scale = sx;	
	
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
			x12 = (bx+6) / 12;
			v = pc[x12][y12];
			
			if (exit_now)
				goto done;

			if (v == pc[x12+1][y12] &&
			    v == pc[x12][y12+1] &&
				v == pc[x12-1][y12] &&
				v == pc[x12][y12-1] &&
				v == pc[x12-2][y12]) {
				for (x = bx; x < (bx+12); x++) {
					cx += sx;
					*b0++ = palette[v];
				}
			}
			else {
				if (scale < 0.000025 || niter != 256) {
					for (x = bx; x < (bx+12); x++) {
						cx += sx;
						v = iterate_double(cx, cy);
						*b0++ = palette[v];
					}
				}
				else
				for (x = bx; x < (bx+12); x++) {
					cx += sx;
					v = iterate_float(cx, cy);
					*b0++ = palette[v];
				}
			}
		}
	}
done:
	t2_done = 1;
}

/*------------------------------------------------------------*/

void	TShowBit::manda(double vx, double vy, double sx, double sy)
{
	long	x, y;
	long	bx;
	double	cx, cy;
	int		v;
	uchar	*bits = (uchar *)the_bitmap->Bits();
	uchar	*b0;
	long	y12;
	long	x12;
	double	scale = sx;	
	
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
			x12 = (bx+6) / 12;
			v = pc[x12][y12];

			if (exit_now)
				goto done;
			
			if (v == pc[x12+1][y12] &&
			    v == pc[x12][y12+1] &&
				v == pc[x12-1][y12] &&
				v == pc[x12][y12-1] &&
				v == pc[x12-2][y12]) {
				for (x = bx; x < (bx+12); x++) {
					cx += sx;
					*b0++ = palette[v];
				}
			}
			else {
				if (scale < 0.000025 || niter != 256) {
					for (x = bx; x < (bx+12); x++) {
						cx += sx;
						v = iterate_double(cx, cy);
						*b0++ = palette[v];
					}
				}
				else
				for (x = bx; x < (bx+12); x++) {
					cx += sx;
					v = iterate_float(cx, cy);
					*b0++ = palette[v];
				}
			}
		}
	}
done:
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
		tmp = (long)tmp_rect.top;
		tmp_rect.top = tmp_rect.bottom;
		tmp_rect.bottom = tmp;
	}

	if (tmp_rect.left > tmp_rect.right) {
		tmp = (long) tmp_rect.right;
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

	SetDrawingMode(B_OP_INVERT);

	do {
		BPoint where;
		GetMouse(&where, &buttons);
		h0 = (long) where.x;
		v0 = (long) where.y;
		width = h0 - h;
		height = v0 - v;
		max= ((v0>v) ^ (height < width)) ? height : width;

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

		snooze(20000);
	} while(buttons);

	selection = sort_rect(&new_select);
	if (!has_selection()) {
		StrokeRect(selection);
		selection.Set(-1000, -1000, -1000, -1000);
	} 
	SetDrawingMode(B_OP_COPY);
}
