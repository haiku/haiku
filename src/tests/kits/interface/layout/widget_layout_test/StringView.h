/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_STRING_VIEW_H
#define WIDGET_LAYOUT_TEST_STRING_VIEW_H


#include <String.h>

#include "View.h"


class StringView : public View {
public:
								StringView(const char* string);

			void				SetString(const char* string);
			void				SetAlignment(alignment align);
			void				SetTextColor(rgb_color color);

	virtual	BSize				MinSize();

			void				SetExplicitMinSize(BSize size);

	virtual	void				AddedToContainer();

	virtual	void				Draw(BView* container, BRect updateRect);

private:
			void				_UpdateStringMetrics();

private:
			BString				fString;
			alignment			fAlignment;
			rgb_color			fTextColor;
			float				fStringAscent;
			float				fStringDescent;
			float				fStringWidth;
			BSize				fExplicitMinSize;
};


#endif	// WIDGET_LAYOUT_TEST_STRING_VIEW_H
