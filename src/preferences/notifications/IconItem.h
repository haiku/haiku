/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICON_ITEM_H
#define _ICON_ITEM_H

#include <View.h>
#include <String.h>

class BBitmap;

class BIconItem {
public:
							BIconItem(BView* owner, const char* label, BBitmap* icon);
	virtual					~BIconItem();

			void			Draw();

			const char*		Label() const;

			void			SetFrame(BRect frame);
			BRect			Frame() const;

			void			Select();
			void			Deselect();
			bool			IsSelected() const;

private:
			BString			fLabel;
			BBitmap*		fIcon;
			bool			fSelected;
			BView*			fOwner;
			BRect			fFrame;
};

#endif // _ICON_ITEM_H
