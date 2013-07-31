/*
 * Copyright 2001-2013 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef DEFAULT_DECORATOR_H
#define DEFAULT_DECORATOR_H


#include <Region.h>

#include "TabDecorator.h"


class Desktop;
class ServerBitmap;

class DefaultDecorator: public TabDecorator {
public:
								DefaultDecorator(DesktopSettings& settings,
									BRect frame);
	virtual						~DefaultDecorator();

	virtual	void				Draw(BRect updateRect);
	virtual	void				Draw();

protected:
	virtual	void				_DrawFrame(BRect r);
	virtual	void				_DrawTab(Decorator::Tab* tab, BRect r);

	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect r);
	virtual	void				_DrawTitle(Decorator::Tab* tab, BRect r);
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect r);

	virtual	void				_SetFocus(Decorator::Tab* tab);

	virtual	void				_FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion);

	virtual	void				_SetLook(Decorator::Tab* tab,
									DesktopSettings& settings, window_look look,
									BRegion* updateRegion = NULL);
	virtual	void				_SetFlags(Decorator::Tab* tab, uint32 flags,
									BRegion* updateRegion = NULL);

	virtual	void				_GetButtonSizeAndOffset(const BRect& tabRect,
									float* offset, float* size,
									float* inset) const;

private:
 			void				_DrawButtonBitmap(ServerBitmap* bitmap,
 									bool direct, BRect rect);
			void				_DrawBlendedRect(DrawingEngine *engine,
									BRect rect, bool down,
									const ComponentColors& colors);
			void 				_InvalidateBitmaps();
			ServerBitmap*		_GetBitmapForButton(Decorator::Tab* tab,
									Component item, bool down, int32 width,
									int32 height);

			void				_GetComponentColors(Component component,
									ComponentColors _colors,
									Decorator::Tab* tab = NULL);

	inline	float				_DefaultTextOffset() const;
	inline	float				_SingleTabOffsetAndSize(float& tabSize);

			void				_CalculateTabsRegion();
};


#endif	// DEFAULT_DECORATOR_H
