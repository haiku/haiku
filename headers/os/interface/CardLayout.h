/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_CARD_LAYOUT_H
#define	_CARD_LAYOUT_H

#include <Layout.h>


class BCardLayout : public BLayout {
public:
								BCardLayout();
	virtual						~BCardLayout();

			BLayoutItem*		VisibleItem() const;
			int32				VisibleIndex() const;
			void				SetVisibleItem(int32 index);
			void				SetVisibleItem(BLayoutItem* item);

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
	virtual	void				ItemAdded(BLayoutItem* item);
	virtual	void				ItemRemoved(BLayoutItem* item);

private:
			BSize				fMin;
			BSize				fMax;
			BSize				fPreferred;
			BLayoutItem*		fVisibleItem;
			bool				fMinMaxValid;

			void				_ValidateMinMax();
};

#endif	// _CARD_LAYOUT_H
