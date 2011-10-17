/*
 * Copyright 2006-2010, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SPLIT_LAYOUT_H
#define	_SPLIT_LAYOUT_H


#include <AbstractLayout.h>
#include <Point.h>


namespace BPrivate {
namespace Layout {
	class Layouter;
	class LayoutInfo;
}
}

using BPrivate::Layout::Layouter;
using BPrivate::Layout::LayoutInfo;


class BSplitLayout : public BAbstractLayout {
public:
								BSplitLayout(enum orientation orientation,
									float spacing = 0.0f);
								BSplitLayout(BMessage* from);
	virtual						~BSplitLayout();

			void				SetInsets(float left, float top, float right,
									float bottom);
			void				GetInsets(float* left, float* top, float* right,
									float* bottom) const;

			float				Spacing() const;
			void				SetSpacing(float spacing);

			orientation			Orientation() const;
			void				SetOrientation(enum orientation orientation);

			float				SplitterSize() const;
			void				SetSplitterSize(float size);

	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);
	virtual	BLayoutItem*		AddView(BView* child, float weight);
	virtual	BLayoutItem*		AddView(int32 index, BView* child,
									float weight);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);
	virtual	bool				AddItem(BLayoutItem* item, float weight);
	virtual	bool				AddItem(int32 index, BLayoutItem* item,
									float weight);


			float				ItemWeight(int32 index) const;
			float				ItemWeight(BLayoutItem* item) const;
			void				SetItemWeight(int32 index, float weight,
									bool invalidateLayout);
			void				SetItemWeight(BLayoutItem* item, float weight);

			void				SetCollapsible(bool collapsible);
			void				SetCollapsible(int32 index, bool collapsible);
			void				SetCollapsible(int32 first, int32 last,
									bool collapsible);

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

	virtual	void				LayoutInvalidated(bool children);
	virtual	void				DoLayout();

	// interface for BSplitView
			BRect				SplitterItemFrame(int32 index) const;
			bool				IsAboveSplitter(const BPoint& point) const;

			bool				StartDraggingSplitter(BPoint point);
			bool				DragSplitter(BPoint point);
			bool				StopDraggingSplitter();
			int32				DraggedSplitter() const;

	// archiving methods
	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

	virtual status_t			ItemArchived(BMessage* into, BLayoutItem* item,
									int32 index) const;
	virtual	status_t			ItemUnarchived(const BMessage* from,
									BLayoutItem* item, int32 index);

protected:
	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);

private:
			class ItemLayoutInfo;
			class ValueRange;
			class SplitterItem;

			void				_InvalidateLayout(bool invalidateView,
									bool children = false);
			void				_InvalidateCachedHeightForWidth();

			SplitterItem*		_SplitterItemAt(const BPoint& point,
									int32* index = NULL) const;
			SplitterItem*		_SplitterItemAt(int32 index) const;

			void				_GetSplitterValueRange(int32 index,
									ValueRange& range);
			int32				_SplitterValue(int32 index) const;

			void				_LayoutItem(BLayoutItem* item, BRect frame,
									bool visible);
			void				_LayoutItem(BLayoutItem* item,
									ItemLayoutInfo* info);

			bool				_SetSplitterValue(int32 index, int32 value);

			ItemLayoutInfo*		_ItemLayoutInfo(BLayoutItem* item) const;


			void				_UpdateSplitterWeights();

			void				_ValidateMinMax();

			void				_InternalGetHeightForWidth(float width,
									bool realLayout, float* minHeight,
									float* maxHeight, float* preferredHeight);

			float				_SplitterSpace() const;

			BSize				_AddInsets(BSize size);
			void				_AddInsets(float* minHeight, float* maxHeight,
									float* preferredHeight);
			BSize				_SubtractInsets(BSize size);

private:
			orientation			fOrientation;
			float				fLeftInset;
			float				fRightInset;
			float				fTopInset;
			float				fBottomInset;
			float				fSplitterSize;
			float				fSpacing;

			BList				fSplitterItems;
			BList				fVisibleItems;

			BSize				fMin;
			BSize				fMax;
			BSize				fPreferred;

			Layouter*			fHorizontalLayouter;
			Layouter*			fVerticalLayouter;

			LayoutInfo*			fHorizontalLayoutInfo;
			LayoutInfo*			fVerticalLayoutInfo;

			BList				fHeightForWidthItems;
			// Incorporates the children's height for width constraints for a
			// concrete width. Cloned lazily from fVerticalLayout when needed.
			Layouter*			fHeightForWidthVerticalLayouter;
			LayoutInfo*			fHeightForWidthHorizontalLayoutInfo;
				// for computing height for width info

			bool				fLayoutValid;

			float				fCachedHeightForWidthWidth;
			float				fHeightForWidthVerticalLayouterWidth;
			float				fCachedMinHeightForWidth;
			float				fCachedMaxHeightForWidth;
			float				fCachedPreferredHeightForWidth;

			BPoint				fDraggingStartPoint;
			int32				fDraggingStartValue;
			int32				fDraggingCurrentValue;
			int32				fDraggingSplitterIndex;
};

#endif	// _SPLIT_LAYOUT_H
