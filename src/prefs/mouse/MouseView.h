// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			MouseView.h
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk)
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Mouse Preferences
//  Created:		December 10, 2003
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef MOUSE_VIEW_H
#define MOUSE_VIEW_H


#include <View.h>
#include <Bitmap.h>
#include <PopUpMenu.h>


class MouseSettings;

class MouseView : public BView {
	public:
		MouseView(BRect frame, const MouseSettings &settings);
		virtual ~MouseView();

		virtual void AttachedToWindow();
		virtual void MouseDown(BPoint where);
		virtual void Draw(BRect frame);
		virtual void Pulse();
		virtual void GetPreferredSize(float *_width, float *_height);

		void SetMouseType(int32 type);
		void UpdateFromSettings();

	private:
		int32 ConvertFromVisualOrder(int32 button);

		typedef BView inherited;

		const MouseSettings &fSettings;
		BBitmap		*fMouseBitmap, *fMouseDownBitmap;
		BRect		fMouseDownBounds;

		int32		fType;
		uint32		fButtons;
		uint32		fOldButtons;
};

#endif	/* MOUSE_VIEW_H */
