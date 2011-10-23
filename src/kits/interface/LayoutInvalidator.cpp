/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "LayoutInvalidator.h"

#include <Layout.h>
#include <LayoutItem.h>
#include <View.h>
#include <ViewPrivate.h>


/*
	Handles propagation of layout invalidations between the various classes.
		* keeps the propagation logic in one place
		* lets the other classes focus on invalidation, not propagation logic
		* along with NVI, makes it easier to ensure that we are not
			invalidating the same objects over and over.

	TODO: could be optimized further if we can be certain that encountering
	an already invalidated object means we are done.

	Note: we must be certain of this for BLayout objects, if we want to have
	any reasonable way to invalidate from a set of BLayoutItems without
	invalidating all of their parent objects n times.
*/


class LayoutInvalidator {
public:

	class Layout;
	class View;
	class LayoutItem;

	/* Since we use templates, we don't need these in the base class,
		but these methods are conceptually virtual.
	virtual bool IsFinished() = 0;
	virtual *this InvalidateChildren() = 0;
	virtual *this DoInvalidation() = 0; // actually does the invalidation
	virtual void Up() = 0; // propagates upwards
	*/

	template <class T>
	static void InvalidateLayout(T from, bool deep = false)
	{
		if (LayoutInvalidator::For(from).IsFinished())
			return;

		if (deep) {
			LayoutInvalidator::For(from)
				.DoInvalidation()
				.InvalidateChildren()
				.Up();
		} else {
			LayoutInvalidator::For(from)
				.DoInvalidation()
				.Up();
		}
	}

	static LayoutInvalidator::Layout For(BLayout* layout);
	static LayoutInvalidator::View For(BView* view);
	static LayoutInvalidator::LayoutItem For(BLayoutItem* item);

	class View {
	public:
		View(BView* view)
			:
			fView(view),
			fViewPrivate(view)
		{
		}

		bool IsFinished()
		{
			return fView != NULL;
			// TODO: test other conditions
		}
		
		View& InvalidateChildren()
		{
			// fViewPrivate.InvalidateChildren();
			// TODO: propagate downwards
			return *this;
		}

		View& DoInvalidation()
		{
			// fViewPrivate.InvalidateLayout();
			// BLayout::Private(fView->GetLayout()).InvalidateLayout();
			// We don't invalidate layout items, if a BView subclass requires
			// that behaviour, it can be implemented there.

			return *this;
		}
		
		View& View::Up()
		{
			BLayout* layout = fView->GetLayout();
			if (layout && layout->Layout()) {
				LayoutInvalidator::InvalidateLayout(layout->Layout());
			} else if (fViewPrivate.CountLayoutItems() > 0) {
				_UpThroughLayoutItems();
			} else {
				LayoutInvalidator::InvalidateLayout(fView->Parent());
			}

			return *this;
		}

	private:
		void _UpThroughLayoutItems()
		{
			int32 count = fViewPrivate.CountLayoutItems();
			for (int32 i = 0; i < count; i++) {
				BLayoutItem* item = fViewPrivate.LayoutItemAt(i);
				LayoutInvalidator::InvalidateLayout(item);
			}
		}

		BView*				fView;
		BView::Private		fViewPrivate;
	};


	class Layout {
	public:
		Layout(BLayout* layout)
			:
			fLayout(layout)
		{
		}

		bool IsFinished()
		{
			// return !fLayout->LayoutIsValid();
			return false;
		}

		Layout& InvalidateChildren()
		{
			/*
			for (int32 i = fLayout->CountItems() - 1; i >= 0 i--;)
				BLayoutItem::Private(fLayout->ItemAt(i)).InvalidateLayout(true);
			*/
			return *this;
		}
		
		Layout& DoInvalidation()
		{
			BLayout::Private(fLayout).InvalidateLayout();
			BView::Private(fLayout->Owner()).InvalidateLayout();

			return *this;
		}
		
		void Up()
		{
			if (fLayout->Layout())
				LayoutInvalidator::InvalidateLayout(fLayout->Layout());
			else if (fLayout->Owner())
				LayoutInvalidator::For(fLayout->Owner()).Up();
		}

	private:
		BLayout* fLayout;
	};

	class LayoutItem {
	public:
		LayoutItem(BLayoutItem* layoutItem)
			:
			fLayoutItem(layoutItem)
		{
		}

		bool IsFinished()
		{
			return !fLayoutItem->LayoutIsValid();
		}

		LayoutItem& InvalidateChildren()
		{
			/* what to do here?? */
			return *this;
		}
		
		LayoutItem& DoInvalidation()
		{
			BLayoutItem::Private(fLayoutItem).InvalidateLayout();
			BView::Private(fLayoutItem->View()).InvalidateLayout();
			return *this;
		}
		
		void Up()
		{
			if (fLayoutItem->Layout())
				BLayoutInvalidator::_InvalidateLayout(fLayoutItem->Layout());
			else if (fLayoutItem->View())
				BLayoutInvalidator::For(fLayoutItem->View()).Up();
		}

	private:
		BLayoutItem* fLayoutItem;
	};


};


inline LayoutInvalidator::Layout
LayoutInvalidator::For(BLayout* layout)
{
	return BLayoutInvalidator::Layout(layout);
}

inline LayoutInvalidator::View
LayoutInvalidator::For(BView* view)
{
	return BLayoutInvalidator::View(view);
}

inline LayoutInvalidator::LayoutItem
LayoutInvalidator::For(BLayoutItem* item)
{
	return LayoutInvalidator::LayoutItem(view);
}


void
BLayoutInvalidator::InvalidateLayout(BLayout* layout, bool deep)
{
	LayoutInvalidator::For(layout).InvalidateLayout(deep);
}


void
BLayoutInvalidator::InvalidateLayout(BLayoutItem* item, bool deep)
{
	LayoutInvalidator::For(item).InvalidateLayout(deep);
}


void
BLayoutInvalidator::InvalidateLayout(BView* view, bool deep)
{
	LayoutInvalidator::For(view).InvalidateLayout(deep);
}

