/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_WRAPPER_VIEW_H
#define WIDGET_LAYOUT_TEST_WRAPPER_VIEW_H


#include "View.h"


class BView;


class WrapperView : public View {
public:
								WrapperView(BView* view);

			BView*				GetView() const;

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	void				AddedToContainer();
	virtual	void				RemovingFromContainer();

	virtual	void				FrameChanged(BRect oldFrame, BRect newFrame);

private:
			void				_UpdateViewFrame();
			BRect				_ViewFrame() const;
			BRect				_ViewFrameInContainer() const;

			BSize				_FromViewSize(BSize size) const;

private:
			BView*				fView;
			BRect				fInsets;
};


#endif	// WIDGET_LAYOUT_TEST_WRAPPER_VIEW_H
