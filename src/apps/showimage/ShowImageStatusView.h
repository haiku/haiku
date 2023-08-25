/*
 * Copyright 2003-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 */
#ifndef SHOW_IMAGE_STATUS_VIEW_H
#define SHOW_IMAGE_STATUS_VIEW_H


#include <Entry.h>
#include <String.h>
#include <View.h>


enum {
	kFrameSizeCell,
	kZoomCell,
	kPagesCell,
	kImageTypeCell,
	kStatusCellCount
};


class ShowImageStatusView : public BView {
public:
								ShowImageStatusView();

	virtual	void				AttachedToWindow();
	virtual void				GetPreferredSize(float* _width, float* _height);
	virtual	void				ResizeToPreferred();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				Update(const entry_ref& ref,
									const BString& text, const BString& pages,
									const BString& imageType, float zoom);
			void				SetZoom(float zoom);
 private:
			void				_SetFrameText(const BString& text);
			void				_SetZoomText(float zoom);
			void				_SetPagesText(const BString& pages);
			void				_SetImageTypeText(const BString& imageType);
			void				_ValidatePreferredSize();
			BSize				fPreferredSize;
			BString				fCellText[kStatusCellCount];
			float				fCellWidth[kStatusCellCount];
			entry_ref			fRef;
};


#endif	// SHOW_IMAGE_STATUS_VIEW_H
