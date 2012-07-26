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
									float beginLimit, float endLimit,
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
				ScrollArrow*	fBeginScrollArrow;
				ScrollArrow*	fEndScrollArrow;

				float			fScrollStep;
				float			fScrollValue;
				float			fScrollLimit;

				float			fBeginLimit;
				float			fEndLimit;

				int32			fOrientation;
};


#endif	// INLINE_SCROLL_VIEW_H
