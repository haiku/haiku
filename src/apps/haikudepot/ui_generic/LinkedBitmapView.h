/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LINKED_BITMAP_VIEW_H
#define LINKED_BITMAP_VIEW_H


#include <Invoker.h>

#include "BitmapView.h"


class LinkedBitmapView : public BitmapView, public BInvoker {
public:
								LinkedBitmapView(const char* name,
									BMessage* message);

	virtual void				AllAttached();

	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				MouseDown(BPoint where);

			void				SetEnabled(bool enabled);

private:
			void				_UpdateViewCursor();

private:
			bool				fEnabled;
			bool				fMouseInside;
};


#endif // LINKED_BITMAP_VIEW_H
