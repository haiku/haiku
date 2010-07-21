/* 
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *		
 */

#ifndef _COLOR_FIELD_H
#define _COLOR_FIELD_H

#include <Control.h>

#if LIB_LAYOUT
#  include <layout.h>
#endif

#include "selected_color_mode.h"

enum {
	MSG_COLOR_FIELD		= 'ColF',
};

class BBitmap;

class ColorField :
				   #if LIB_LAYOUT
				   public MView,
				   #endif
				   public BControl {
 public:
								ColorField(BPoint offset_point,
										   selected_color_mode mode,
										   float fixed_value,
										   orientation orient = B_VERTICAL);
	virtual						~ColorField();

								// MView
	#if LIB_LAYOUT
	virtual	minimax				layoutprefs();
	virtual	BRect				layout(BRect frame);
	#endif

								// BControl
	virtual	status_t			Invoke(BMessage *msg = NULL);

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
										   const BMessage* message);

								// ColorField
			void				Update(int depth);

			void				SetModeAndValue(selected_color_mode mode, float fixed_value);
			void				SetFixedValue(float fixed_value);
			float				FixedValue() const
									{ return fFixedValue; }
			
			void				SetMarkerToColor( rgb_color color );
			void				PositionMarkerAt( BPoint where );

			float				Width() const;
			float				Height() const;
			bool				IsTracking() const
									{ return fMouseDown; }

 private:
	static	status_t				_UpdateThread(void* data);
			void				_DrawBorder();

	selected_color_mode			fMode;
	float						fFixedValue;
	orientation					fOrientation;

	BPoint						fMarkerPosition;
	BPoint						fLastMarkerPosition;

	bool						fMouseDown;

	BBitmap*					fBgBitmap[2];
	BView*						fBgView[2];

	thread_id					fUpdateThread;
	port_id						fUpdatePort;
};

#endif
