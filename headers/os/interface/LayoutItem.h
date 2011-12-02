/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_LAYOUT_ITEM_H
#define	_LAYOUT_ITEM_H


#include <Alignment.h>
#include <Archivable.h>
#include <Rect.h>
#include <Size.h>


class BLayout;
class BView;


class BLayoutItem : public BArchivable {
public:
								BLayoutItem();
								BLayoutItem(BMessage* from);
	virtual						~BLayoutItem();

			BLayout*			Layout() const;

	virtual	BSize				MinSize() = 0;
	virtual	BSize				MaxSize() = 0;
	virtual	BSize				PreferredSize() = 0;
	virtual	BAlignment			Alignment() = 0;

	virtual	void				SetExplicitMinSize(BSize size) = 0;
	virtual	void				SetExplicitMaxSize(BSize size) = 0;
	virtual	void				SetExplicitPreferredSize(BSize size) = 0;
	virtual	void				SetExplicitAlignment(BAlignment alignment) = 0;

	virtual	bool				IsVisible() = 0;
	virtual	void				SetVisible(bool visible) = 0;

	virtual	BRect				Frame() = 0;
	virtual	void				SetFrame(BRect frame) = 0;

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

	virtual	BView*				View();

	virtual	void				InvalidateLayout(bool children = false);
	virtual	void				Relayout(bool immediate = false);

			void*				LayoutData() const;
			void				SetLayoutData(void* data);

			void				AlignInFrame(BRect frame);

	virtual status_t			Archive(BMessage* into, bool deep = true) const;

	virtual status_t			Perform(perform_code d, void* arg);

protected:
	// archiving methods
	virtual status_t			AllArchived(BMessage* into) const;
	virtual	status_t			AllUnarchived(const BMessage* from);

	// hook methods
	virtual	void				LayoutInvalidated(bool children);

	virtual	void				AttachedToLayout();
	virtual	void				DetachedFromLayout(BLayout* layout);

	virtual void				AncestorVisibilityChanged(bool shown);

private:
			void				SetLayout(BLayout* layout);

	virtual	void				_ReservedLayoutItem1();
	virtual	void				_ReservedLayoutItem2();
	virtual	void				_ReservedLayoutItem3();
	virtual	void				_ReservedLayoutItem4();
	virtual	void				_ReservedLayoutItem5();
	virtual	void				_ReservedLayoutItem6();
	virtual	void				_ReservedLayoutItem7();
	virtual	void				_ReservedLayoutItem8();
	virtual	void				_ReservedLayoutItem9();
	virtual	void				_ReservedLayoutItem10();

	// forbidden methods
								BLayoutItem(const BLayoutItem&);
			void				operator =(const BLayoutItem&);

			friend class BLayout;

			BLayout*			fLayout;
			void*				fLayoutData;

			uint32				_reserved[5];
};


#endif	// _LAYOUT_ITEM_H
