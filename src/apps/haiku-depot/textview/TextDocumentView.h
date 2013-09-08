/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_DOCUMENT_VIEW_H
#define TEXT_DOCUMENT_VIEW_H

#include <String.h>
#include <View.h>

#include "TextDocument.h"
#include "TextDocumentLayout.h"


class TextDocumentView : public BView {
public:
								TextDocumentView(const char* name = NULL);
	virtual						~TextDocumentView();

	virtual void				Draw(BRect updateRect);

	virtual	void				AttachedToWindow();
	virtual void				FrameResized(float width, float height);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

			void				SetTextDocument(
									const TextDocumentRef& document);

			void				SetInsets(float inset);
			void				SetInsets(float horizontal, float vertical);
			void				SetInsets(float left, float top, float right,
									float bottom);
private:
			float				_TextLayoutWidth(float viewWidth) const;

			void				_UpdateScrollBars();

private:
			TextDocumentRef		fTextDocument;
			TextDocumentLayout	fTextDocumentLayout;

			float				fInsetLeft;
			float				fInsetTop;
			float				fInsetRight;
			float				fInsetBottom;
};

#endif // TEXT_DOCUMENT_VIEW_H
