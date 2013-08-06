/*
 * Copyright 2001-2013, Haiku, Inc.
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


#include <Region.h>

#include "DecorManager.h"
#include "RGBColor.h"
#include "TabDecorator.h"


class Desktop;


class BeDecorAddOn : public DecorAddOn {
public:
								BeDecorAddOn(image_id id, const char* name);

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect);
};


class BeDecorator: public TabDecorator {
public:
								BeDecorator(DesktopSettings& settings, BRect frame);
	virtual						~BeDecorator();

	virtual	void				Draw(BRect updateRect);
	virtual	void				Draw();

protected:
	virtual	void				_DrawFrame(BRect rect);
	virtual	void				_DrawTab(Decorator::Tab* tab, BRect rect);

	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawTitle(Decorator::Tab* tab, BRect rect);
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect rect);

private:
			void				_DrawBlendedRect(Decorator::Tab* tab,
									BRect rect, bool down);

			void				_GetComponentColors(Component component,
									ComponentColors _colors,
									Decorator::Tab* tab = NULL);

	inline	float				_DefaultTextOffset() const;
	inline	float				_SingleTabOffsetAndSize(float& tabSize);

			void				_CalculateTabsRegion();
};


#endif	// BE_DECORATOR_H
