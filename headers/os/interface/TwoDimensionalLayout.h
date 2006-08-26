/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_TWO_DIMENSIONAL_LAYOUT_H
#define	_TWO_DIMENSIONAL_LAYOUT_H

#include <Layout.h>

class BLayoutContext;


class BTwoDimensionalLayout : public BLayout {
public:
								BTwoDimensionalLayout();
	virtual						~BTwoDimensionalLayout();

			void				SetInsets(float left, float top, float right,
									float bottom);
			void				GetInsets(float* left, float* top, float* right,
									float* bottom);

			void				AlignLayoutWith(BTwoDimensionalLayout* other,
									enum orientation orientation);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			Alignment();

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

	virtual	void				InvalidateLayout();

	virtual	void				LayoutView();

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
	virtual	void	 			GetItemDimensions(BLayoutItem* item,
									Dimensions* dimensions) = 0;

private:
			class CompoundLayouter;
			class LocalLayouter;
			class VerticalCompoundLayouter;

			friend class LocalLayouter;

			void				_ValidateMinMax();
			BLayoutContext*		_CurrentLayoutContext();

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

#endif	//	_TWO_DIMENSIONAL_LAYOUT_H
