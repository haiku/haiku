/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_TWO_DIMENSIONAL_LAYOUT_H
#define	_TWO_DIMENSIONAL_LAYOUT_H


#include <AbstractLayout.h>

class BLayoutContext;


class BTwoDimensionalLayout : public BAbstractLayout {
public:
								BTwoDimensionalLayout();
								BTwoDimensionalLayout(BMessage* from);
	virtual						~BTwoDimensionalLayout();

			void				SetInsets(float left, float top, float right,
									float bottom);
			void				SetInsets(float horizontal, float vertical);
			void				SetInsets(float insets);
			void				GetInsets(float* left, float* top, float* right,
									float* bottom) const;

			void				AlignLayoutWith(BTwoDimensionalLayout* other,
									enum orientation orientation);

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

	virtual	void				SetFrame(BRect frame);

	virtual	void				InvalidateLayout(bool children = false);

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	virtual status_t			AllArchived(BMessage* into) const;
	virtual	status_t			AllUnarchived(const BMessage* from);

protected:
			struct ColumnRowConstraints {
				float	weight;
				float	min;
				float	max;
			};

			struct Dimensions {
				int32	x;
				int32	y;
				int32	width;
				int32	height;
			};

	virtual	void				DoLayout();

			BSize				AddInsets(BSize size);
			void				AddInsets(float* minHeight, float* maxHeight,
									float* preferredHeight);
			BSize				SubtractInsets(BSize size);

	virtual	void				PrepareItems(enum orientation orientation);
	virtual	bool				HasMultiColumnItems();
	virtual	bool				HasMultiRowItems();

	virtual	int32				InternalCountColumns() = 0;
	virtual	int32				InternalCountRows() = 0;
	virtual	void				GetColumnRowConstraints(
									enum orientation orientation,
									int32 index,
									ColumnRowConstraints* constraints) = 0;
	virtual	void				GetItemDimensions(BLayoutItem* item,
									Dimensions* dimensions) = 0;

private:
			class CompoundLayouter;
			class LocalLayouter;
			class VerticalCompoundLayouter;

			friend class LocalLayouter;

			void				_ValidateMinMax();

protected:
			float				fLeftInset;
			float				fRightInset;
			float				fTopInset;
			float				fBottomInset;
			float				fHSpacing;
			float				fVSpacing;

private:
			LocalLayouter*		fLocalLayouter;
};

#endif // _TWO_DIMENSIONAL_LAYOUT_H
