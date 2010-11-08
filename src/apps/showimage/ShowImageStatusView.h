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


class ShowImageStatusView : public BView {
public:
								ShowImageStatusView(BRect rect,
									const char* name, uint32 resizingMode,
									uint32 flags);

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				Update(const entry_ref& ref,
									const BString& text);

private:
			BString				fText;
			entry_ref			fRef;
};


#endif	// SHOW_IMAGE_STATUS_VIEW_H
