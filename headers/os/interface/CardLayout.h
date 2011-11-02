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

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	virtual status_t			AllUnarchived(const BMessage* from);
	static	BArchivable*		Instantiate(BMessage* from);

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual	void				LayoutInvalidated(bool children = false);
	virtual	void				DoLayout();
	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);

private:

			void				_ValidateMinMax();

	// FBC padding
	virtual	void				_ReservedCardLayout1();
	virtual	void				_ReservedCardLayout2();
	virtual	void				_ReservedCardLayout3();
	virtual	void				_ReservedCardLayout4();
	virtual	void				_ReservedCardLayout5();
	virtual	void				_ReservedCardLayout6();
	virtual	void				_ReservedCardLayout7();
	virtual	void				_ReservedCardLayout8();
	virtual	void				_ReservedCardLayout9();
	virtual	void				_ReservedCardLayout10();

			BSize				fMin;
			BSize				fMax;
			BSize				fPreferred;
			BLayoutItem*		fVisibleItem;
			bool				fMinMaxValid;

			uint32				_reserved[5];
};

#endif	// _CARD_LAYOUT_H
