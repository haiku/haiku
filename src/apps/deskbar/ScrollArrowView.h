/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		John Scipione (jscipione@gmail.com)
 */
#ifndef SCROLL_ARROW_VIEW_H
#define SCROLL_ARROW_VIEW_H


#include <View.h>

class BLayout;
class BMenu;
class ScrollArrow;
class BPoint;


class TScrollArrowView : public BView {
public:
								TScrollArrowView(BRect frame, BMenu* menu);
	virtual						~TScrollArrowView();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

				void			AttachScrollers();
				void			DetachScrollers();
				bool			HasScrollers() const;

				void			SetSmallStep(float step);
				void			GetSteps(float* _smallStep, float* _largeStep) const;

				bool			CheckForScrolling(const BPoint& cursor);
				void			ScrollBy(const float& step);

protected:
				bool			_Scroll(const BPoint& cursor);

private:
				BMenu*			fMenu;
				ScrollArrow*	fUpperScrollArrow;
				ScrollArrow*	fLowerScrollArrow;

				float			fScrollStep;
				float			fValue;
				float			fLimit;
};


#endif	// SCROLL_ARROW_VIEW_H
