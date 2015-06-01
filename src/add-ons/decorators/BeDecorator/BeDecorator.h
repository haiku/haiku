/*
 * Copyright 2001-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef BE_DECORATOR_H
#define BE_DECORATOR_H


#include "DecorManager.h"
#include "SATDecorator.h"


class Desktop;
class ServerBitmap;


class BeDecorAddOn : public DecorAddOn {
public:
								BeDecorAddOn(image_id id, const char* name);

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect);
};


class BeDecorator: public SATDecorator {
public:
								BeDecorator(DesktopSettings& settings, BRect frame);
	virtual						~BeDecorator();

	virtual	void				GetComponentColors(Component component,
									uint8 highlight, ComponentColors _colors,
									Decorator::Tab* tab = NULL);

protected:
	virtual	void				_DrawFrame(BRect rect);

	virtual	void				_DrawTab(Decorator::Tab* tab, BRect rect);
	virtual	void				_DrawTitle(Decorator::Tab* tab, BRect rect);
	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawMinimize(Decorator::Tab* tab, bool direct,
									BRect rect);

	virtual	void				_GetButtonSizeAndOffset(const BRect& tabRect,
									float* offset, float* size,
									float* inset) const;

private:
			void				_DrawBevelRect(DrawingEngine* engine,
									const BRect rect, bool down,
									rgb_color light, rgb_color shadow);
			void				_DrawBlendedRect(DrawingEngine* engine,
									const BRect rect, bool down,
									rgb_color colorA, rgb_color colorB,
									rgb_color colorC, rgb_color colorD);
			void				_DrawButtonBitmap(ServerBitmap* bitmap,
									bool direct, BRect rect);
			ServerBitmap*		_GetBitmapForButton(Decorator::Tab* tab,
									Component item, bool down, int32 width,
									int32 height);
			void				_GetComponentColors(Component component,
									ComponentColors _colors,
									Decorator::Tab* tab = NULL);
};


#endif	// BE_DECORATOR_H
