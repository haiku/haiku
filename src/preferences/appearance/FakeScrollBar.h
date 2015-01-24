/*
 *  Copyright 2010-2015 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef FAKE_SCROLL_BAR_H
#define FAKE_SCROLL_BAR_H


#include <Control.h>


class FakeScrollBar : public BControl {
public:
							FakeScrollBar(bool drawArrows, bool doubleArrows,
								uint32 knobStyle, BMessage* message);
							~FakeScrollBar(void);

	virtual	void			MouseDown(BPoint point);
	virtual	void			MouseMoved(BPoint point, uint32 transit,
								const BMessage *message);
	virtual	void			MouseUp(BPoint point);

	virtual	void			Draw(BRect updateRect);

	virtual	void			SetValue(int32 value);

			void			SetDoubleArrows(bool doubleArrows);
			void			SetKnobStyle(uint32 knobStyle);

			void			SetFromScrollBarInfo(const scroll_bar_info &info);

private:
			void			_DrawArrowButton(int32 direction, BRect r,
								const BRect& updateRect);

			bool			fDrawArrows;
			bool			fDoubleArrows;
			uint32			fKnobStyle;
};

#endif	// FAKE_SCROLL_BAR_H
