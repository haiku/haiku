/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		John Scipione (jscipione@gmail.com)
 */
#ifndef SCROLL_MENU_H
#define SCROLL_MENU_H


#include <View.h>

class BLayout;
class BMenu;
class BMenuScroller;


class BScrollMenu : public BView {
public:
								BScrollMenu(BMenu* menu);
	virtual						~BScrollMenu();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float newWidth, float newHeight);

				void			AttachScrollers();
				void			DetachScrollers();
				bool			HasScrollers() const;

				void			SetSmallStep(float step);
				void			GetSteps(float* _smallStep, float* _largeStep) const;
				bool			CheckForScrolling(const BPoint& cursor);
				bool			TryScrollBy(const float& step);

protected:
				bool			_Scroll(const BPoint& cursor);
				void			_ScrollBy(const float& step);

private:
				BMenu*			fMenu;
				BMenuScroller*	fUpperScroller;
				BMenuScroller*	fLowerScroller;

				float			fScrollStep;
				float			fValue;
				float			fLimit;
};


#endif	// SCROLL_MENU_H
