/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef MOUSE_VIEW_H
#define MOUSE_VIEW_H


#include <Bitmap.h>
#include <Picture.h>
#include <PopUpMenu.h>
#include <View.h>


class MouseSettings;

class MouseView : public BView {
public:
								MouseView(const MouseSettings& settings);
		virtual					~MouseView();

				void			SetMouseType(int32 type);
				void			MouseMapUpdated();
				void			UpdateFromSettings();

		virtual	void			GetPreferredSize(float* _width, float* _height);
		virtual	void			AttachedToWindow();
		virtual	void			MouseUp(BPoint where);
		virtual	void			MouseDown(BPoint where);
		virtual	void			Draw(BRect frame);
		bool					IsMouseConnected()
								{ return fConnected; }

private:
				BRect			_ButtonsRect() const;
				BRect			_ButtonRect(const int32* offsets,
									int index) const;
				int32			_ConvertFromVisualOrder(int32 button);
				void			_CreateButtonsPicture();

private:
	typedef BView inherited;

		const	MouseSettings&	fSettings;

				BPicture		fButtonsPicture;
				int32			fDigitBaseline;
				int32			fDigitHeight;
				float			fScaling;

				int32			fType;
				uint32			fButtons;
				uint32			fOldButtons;
				bool			fConnected;
};


#endif	/* MOUSE_VIEW_H */