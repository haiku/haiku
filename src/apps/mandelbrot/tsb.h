/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef	_VIEW_H
#include <View.h>
#endif
#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif

#include <math.h>

#ifndef	TSB
#define	TSB

/*------------------------------------------------------------*/

#define	size_x	384
#define	size_y	384

/*------------------------------------------------------------*/

class	TShowBit : public BView {
public:
		BBitmap		*the_bitmap;
		bool		busy;
		bool		exit_now;
		BRect		selection;
		double		px;
		double		py;
		double		scale;
		long		iter;

						TShowBit(BRect r, uint32 resizeMask, uint32 flags);
virtual					~TShowBit();
virtual		void		Draw(BRect);
virtual		void		MouseDown(BPoint where);
virtual		void		Pulse();
			void		mand(double vx, double vy, double sx, double sy);
			long		limit_v(long v);
			long		limit_h(long h);
			BRect		sort_rect(BRect *aRect);
			void		clip(long *h, long *v);
			void		change_selection(long h, long v);
			char		has_selection();
			void		redraw_mand();
			void		mandb(double vx, double vy, double sx, double sy);
			void		manda(double vx, double vy, double sx, double sy);
			void		set_palette(long code);
			void		set_iter(long i);
			void		precompute(double vx, double vy, double sx, double sy);
};

/*------------------------------------------------------------*/

#endif
