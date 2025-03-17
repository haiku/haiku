/*
 * Copyright 2025, Haiku, Inc.
 * Authors:
 *     Pawan Yerramilli <me@pawanyerramilli.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "Point.h"
#include <Cursor.h>
#include <SupportDefs.h>
#include <View.h>


enum Handle {
	DRAG_TOP_LEFT,
	DRAG_TOP_RIGHT,
	DRAG_BOTTOM_LEFT,
	DRAG_BOTTOM_RIGHT,
	DRAG_NO_HANDLE
};

class SelectAreaView : public BView {
public:
							SelectAreaView(BBitmap* screenshot);

			void			AttachedToWindow();
			void			MessageReceived(BMessage* message);
			void			Draw(BRect updateRect);
			void			KeyDown(const char* bytes, int32 numBytes);
			void			MouseDown(BPoint point);
			void			MouseMoved(BPoint point, uint32 transit, const BMessage* message);
			void			MouseUp(BPoint point);

private:
			BRect			_CurrentFrame();
			BRect			_GetHandle(Handle handle);
			Handle			_FindSelectedHandle(BPoint point);
			void			_SaveSelection();

			BBitmap*		fScreenShot;
			BCursor			fCursor;
			bool			fIsCurrentlyDragging;
			BPoint			fStartCorner;
			BPoint			fEndCorner;
			bool			fResizable;
			Handle			fCurrentHandle;
			BPoint			fMoveDelta;
};
