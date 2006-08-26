/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SPACE_LAYOUT_ITEM_H
#define	_SPACE_LAYOUT_ITEM_H

#include <LayoutItem.h>


class BSpaceLayoutItem : public BLayoutItem {
public:
								BSpaceLayoutItem(BSize minSize, BSize maxSize,
									BSize preferredSize, BAlignment alignment);
	virtual						~BSpaceLayoutItem();

	static	BSpaceLayoutItem*	CreateGlue();
	static	BSpaceLayoutItem*	CreateHorizontalStrut(float width);
	static	BSpaceLayoutItem*	CreateVerticalStrut(float height);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			Alignment();

	virtual	void				SetExplicitMinSize(BSize size);
	virtual	void				SetExplicitMaxSize(BSize size);
	virtual	void				SetExplicitPreferredSize(BSize size);
	virtual	void				SetExplicitAlignment(BAlignment alignment);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

private:
			BRect				fFrame;
			BSize				fMinSize;
			BSize				fMaxSize;
			BSize				fPreferredSize;
			BAlignment			fAlignment;
			bool				fVisible;
};

#endif	//	_SPACE_LAYOUT_ITEM_H
