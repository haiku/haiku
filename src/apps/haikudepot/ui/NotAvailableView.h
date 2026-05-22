/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef NOT_AVAILABLE_STRING_VIEW_H
#define NOT_AVAILABLE_STRING_VIEW_H


#include <View.h>


/*!	This view abstracts a message to indicate that there is no item to view.
 *	This exists because the BStringView only renders at the bottom of the
 *	view frame according to the legacy Be documentation. This is unfortunately
 *	not convenient for this use-case. In addition, this view will centralize
 *	the configuration of colors etc... for this purpose.
 */
class NotAvailableView : public BView {
public:
								NotAvailableView(const char* name, const char* text, bool isImage);
	virtual						~NotAvailableView();

	virtual	BSize				MaxSize();
	virtual	BAlignment			LayoutAlignment();

			void				SetText(const char* text);
			const char*			Text() const;

			void				SetIsImage(bool value);
			bool				IsImage() const;

	virtual	void				Draw(BRect updateRect);

private:
			void				_DrawIsImage(BRect updateRect);
			void				_DrawStippledBorder(BRect box);
			void				_DrawText(BRect textRect, BRect updateRect);

private:
			bool				fIsImage;
			BString				fText;
};


#endif // NOT_AVAILABLE_STRING_VIEW_H
