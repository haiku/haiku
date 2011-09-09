/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_CARD_LAYOUT_H
#define	_CARD_LAYOUT_H

#include <AbstractLayout.h>


class BCardLayout : public BAbstractLayout {
public:
								BCardLayout();
								BCardLayout(BMessage* from);
	virtual						~BCardLayout();

			BLayoutItem*		VisibleItem() const;
			int32				VisibleIndex() const;
			void				SetVisibleItem(int32 index);
			void				SetVisibleItem(BLayoutItem* item);

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

	virtual	void				InvalidateLayout(bool children = false);

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	virtual status_t			AllUnarchived(const BMessage* from);
	static	BArchivable*		Instantiate(BMessage* from);

protected:
	virtual	void				DoLayout();
	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);

private:
			BSize				fMin;
			BSize				fMax;
			BSize				fPreferred;
			BLayoutItem*		fVisibleItem;
			bool				fMinMaxValid;

			void				_ValidateMinMax();
};

#endif	// _CARD_LAYOUT_H
