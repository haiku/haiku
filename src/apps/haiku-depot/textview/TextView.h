/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

#include <String.h>
#include <View.h>

#include "TextLayout.h"


class TextView : public BView {
public:
								TextView(const char* name = NULL);
	virtual						~TextView();

	virtual void				Draw(BRect updateRect);

	virtual void				AttachedToWindow();
	virtual void				FrameResized(float width, float height);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

			void				SetText(const BString& text);

private:
			TextLayout			fTextLayout;
};

#endif // TEXT_VIEW_H
