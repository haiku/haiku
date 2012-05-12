/*
 * Copyright 2006-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SWATCH_VIEW_H
#define SWATCH_VIEW_H

#include <View.h>


class SwatchView : public BView {
public:
								SwatchView(const char* name, BMessage* message,
									BHandler* target, rgb_color color,
									float width = 24.0, float height = 24.0);
	virtual						~SwatchView();

	// BView
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	// SwatchView
			void				SetColor(rgb_color color);
	inline	rgb_color			Color() const
									{ return fColor; }

			void				SetClickedMessage(BMessage* message);
			void				SetDroppedMessage(BMessage* message);

private:
			void				_Invoke(const BMessage* message);
			void				_StrokeRect(BRect frame, rgb_color leftTop,
									rgb_color rightBottom);
			void				_DragColor();

private:
			rgb_color			fColor;
			BPoint				fTrackingStart;
			bool				fActive;
			bool				fDropInvokes;

			BMessage*			fClickMessage;
			BMessage*			fDroppedMessage;
			BHandler*			fTarget;

			float				fWidth;
			float				fHeight;
};

#endif // SWATCH_VIEW_H
