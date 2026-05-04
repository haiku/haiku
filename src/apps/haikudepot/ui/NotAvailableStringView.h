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
class NotAvailableStringView : public BView {
public:
								NotAvailableStringView(const char* name, const char* text);
	virtual						~NotAvailableStringView();

	virtual	BSize				MaxSize();
	virtual BAlignment			LayoutAlignment();

			void				SetText(const char* text);
			const char*			Text() const;

	virtual	void				Draw(BRect bounds);

private:
			BString				fText;
};


#endif // NOT_AVAILABLE_STRING_VIEW_H
