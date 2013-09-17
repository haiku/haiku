/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BITMAP_BUTTON_H
#define BITMAP_BUTTON_H


#include <Invoker.h>

#include "BitmapView.h"


class BitmapButton : public BitmapView, public BInvoker {
public:
								BitmapButton(const char* name,
									BMessage* message = NULL);
	virtual						~BitmapButton();

	virtual	void				AttachedToWindow();

	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

protected:
	virtual void				DrawBackground(BRect& bounds,
									BRect updateRect);

private:
			void				_SetMouseInside(bool inside);
			void				_SetMouseDown(bool down);

private:
			bool				fMouseInside;
			bool				fMouseDown;
};

#endif // BITMAP_VIEW_H
