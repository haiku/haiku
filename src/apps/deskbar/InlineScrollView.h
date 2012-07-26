/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		John Scipione (jscipione@gmail.com)
 */
#ifndef INLINE_SCROLL_VIEW_H
#define INLINE_SCROLL_VIEW_H


#include <View.h>

class BLayout;
class ScrollArrow;
class BPoint;


class TInlineScrollView : public BView {
public:
								TInlineScrollView(BRect frame, BView* target,
									enum orientation orientation = B_VERTICAL);
	virtual						~TInlineScrollView();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

				void			AttachScrollers();
				void			DetachScrollers();
				bool			HasScrollers() const;

				void			SetSmallStep(float step);
				void			GetSteps(float* _smallStep, float* _largeStep) const;
				void			ScrollBy(const float& step);

private:
				BView*			fTarget;
				ScrollArrow*	fUpperScrollArrow;
				ScrollArrow*	fLowerScrollArrow;

				float			fScrollStep;
				float			fValue;
				float			fLimit;
};


#endif	// INLINE_SCROLL_VIEW_H
