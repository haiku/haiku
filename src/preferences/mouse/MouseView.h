/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 */
#ifndef MOUSE_VIEW_H
#define MOUSE_VIEW_H


#include <Bitmap.h>
#include <PopUpMenu.h>
#include <View.h>


class MouseSettings;

class MouseView : public BView {
public:
								MouseView(const MouseSettings &settings);
		virtual					~MouseView();

		virtual	void			AttachedToWindow();
		virtual	void			MouseDown(BPoint where);
		virtual	void			MouseUp(BPoint where);
		virtual	void			Draw(BRect frame);
		virtual	void			GetPreferredSize(float *_width, float *_height);

				void			SetMouseType(int32 type);
				void			MouseMapUpdated();
				void			UpdateFromSettings();

private:
		int32 _ConvertFromVisualOrder(int32 button);

				typedef			BView inherited;

		const	MouseSettings	&fSettings;
				BBitmap			*fMouseBitmap, *fMouseDownBitmap;
				BRect			fMouseDownBounds;

				int32			fType;
				uint32			fButtons;
				uint32			fOldButtons;
};

#endif	/* MOUSE_VIEW_H */
