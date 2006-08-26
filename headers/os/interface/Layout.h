/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_LAYOUT_H
#define	_LAYOUT_H

#include <Alignment.h>
#include <List.h>
#include <Size.h>

class BLayoutItem;
class BView;


class BLayout {
public:
								BLayout();
	virtual						~BLayout();

			BView*				View() const;

	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);

	virtual	bool				RemoveView(BView* child);
	virtual	bool				RemoveItem(BLayoutItem* item);
	virtual	BLayoutItem*		RemoveItem(int32 index);

			BLayoutItem*		ItemAt(int32 index) const;
			int32				CountItems() const;
			int32				IndexOfItem(BLayoutItem* item) const;
			int32				IndexOfView(BView* child) const;

	virtual	BSize				MinSize() = 0;
	virtual	BSize				MaxSize() = 0;
	virtual	BSize				PreferredSize() = 0;
	virtual	BAlignment			Alignment() = 0;

	virtual	bool				HasHeightForWidth() = 0;
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred) = 0;

	virtual	void				InvalidateLayout();

	virtual	void				LayoutView() = 0;

protected:
// TODO: Since memory allocations can fail, we should return a bool and
// undo the addition, if false.
	virtual	void				ItemAdded(BLayoutItem* item);
	virtual	void				ItemRemoved(BLayoutItem* item);


private:
			friend class BView;

			void				SetView(BView* view);

			BView*				fView;
			BList				fItems;
};

#endif	//	_LAYOUT_H
