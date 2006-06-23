/* 
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *		
 */

#ifndef _COLOR_SLIDER_H
#define _COLOR_SLIDER_H

#include <Control.h>

#if LIB_LAYOUT
#  include <layout.h>
#endif

#include "selected_color_mode.h"

#define	MSG_COLOR_SLIDER	'ColS'

class BBitmap;

class ColorSlider : 
					#if LIB_LAYOUT
					public MView,
					#endif
					public BControl {

public:
								ColorSlider(BPoint offset_point,
											selected_color_mode mode,
											float value1, float value2,
											orientation dir = B_VERTICAL);
	virtual						~ColorSlider();

	#if LIB_LAYOUT
								// MView
	virtual	minimax				layoutprefs();
	virtual	BRect				layout(BRect frame);
	#endif

								// BControl
	virtual	void				AttachedToWindow();

	virtual	status_t			Invoke(BMessage* message = NULL);

	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
										   const BMessage* dragMessage);

	virtual	void				SetValue(int32 value);

								// ColorSlider
			void				Update(int depth);

			bool				IsTracking() const
									{ return fMouseDown; }

			void				SetModeAndValues(selected_color_mode mode,
												 float value1, float value2);
			void				SetOtherValues(float value1, float value2);
			void				GetOtherValues(float* value1, float* value2) const;

			void				SetMarkerToColor( rgb_color color );

//	inline	void				_DrawColorLineY( float y, int r, int g, int b);
private:

	static	int32				_UpdateThread(void* cookie);
	static	inline void			_DrawColorLineY(BView* view, float y,
											  int r, int g, int b);
	static	inline void			_DrawColorLineX(BView* view, float x,
											  int r, int g, int b);
			void				_TrackMouse(BPoint where);

	selected_color_mode			fMode;
	float						fFixedValue1;
	float						fFixedValue2;
	
	bool						fMouseDown;
	
	BBitmap*					fBgBitmap;
	BView*						fBgView;
	
	thread_id					fUpdateThread;
	port_id						fUpdatePort;

	orientation					fOrientation;
};

#endif
