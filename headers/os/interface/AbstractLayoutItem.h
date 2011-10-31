/*
 * Copyright 2006, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_ABSTRACT_LAYOUT_ITEM_H
#define	_ABSTRACT_LAYOUT_ITEM_H

#include <Alignment.h>
#include <LayoutItem.h>
#include <Size.h>


class BAbstractLayoutItem : public BLayoutItem {
public:
								BAbstractLayoutItem();
								BAbstractLayoutItem(BMessage* from);
	virtual						~BAbstractLayoutItem();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			Alignment();

	virtual	void				SetExplicitMinSize(BSize size);
	virtual	void				SetExplicitMaxSize(BSize size);
	virtual	void				SetExplicitPreferredSize(BSize size);
	virtual	void				SetExplicitAlignment(BAlignment alignment);

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual status_t			Archive(BMessage* into, bool deep = true) const;

	virtual	status_t			Perform(perform_code d, void* arg);

private:
			BSize				fMinSize;
			BSize				fMaxSize;
			BSize				fPreferredSize;
			BAlignment			fAlignment;
};

#endif	//	_ABSTRACT_LAYOUT_ITEM_H
