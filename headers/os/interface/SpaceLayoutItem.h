/*
 * Copyright 2006, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SPACE_LAYOUT_ITEM_H
#define	_SPACE_LAYOUT_ITEM_H

#include <LayoutItem.h>


class BSpaceLayoutItem : public BLayoutItem {
public:
								BSpaceLayoutItem(BSize minSize, BSize maxSize,
									BSize preferredSize, BAlignment alignment);
								BSpaceLayoutItem(BMessage* archive);
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

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
	// FBC padding
	virtual	void				_ReservedSpaceLayoutItem1();
	virtual	void				_ReservedSpaceLayoutItem2();
	virtual	void				_ReservedSpaceLayoutItem3();
	virtual	void				_ReservedSpaceLayoutItem4();
	virtual	void				_ReservedSpaceLayoutItem5();
	virtual	void				_ReservedSpaceLayoutItem6();
	virtual	void				_ReservedSpaceLayoutItem7();
	virtual	void				_ReservedSpaceLayoutItem8();
	virtual	void				_ReservedSpaceLayoutItem9();
	virtual	void				_ReservedSpaceLayoutItem10();

	// forbidden methods
								BSpaceLayoutItem(const BSpaceLayoutItem&);
			void				operator =(const BSpaceLayoutItem&);

			BRect				fFrame;
			BSize				fMinSize;
			BSize				fMaxSize;
			BSize				fPreferredSize;
			BAlignment			fAlignment;
			bool				fVisible;

			uint32				_reserved[2];
};

#endif	//	_SPACE_LAYOUT_ITEM_H
