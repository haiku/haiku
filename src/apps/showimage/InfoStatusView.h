/*
 * Copyright 2003-2026 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 */
#ifndef INFO_STATUS_VIEW_H
#define INFO_STATUS_VIEW_H


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


class InfoStatusView : public BView {
public:
								InfoStatusView();

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


#endif // INFO_STATUS_VIEW_H
