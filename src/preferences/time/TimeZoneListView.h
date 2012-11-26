/*
 * Copyright 2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sean Bailey <ziusudra@gmail.com>
 */
#ifndef _TIME_ZONE_LIST_VIEW_H
#define _TIME_ZONE_LIST_VIEW_H


#include <OutlineListView.h>


class BTextToolTip;


class TimeZoneListView : public BOutlineListView {
public:
								TimeZoneListView(void);
								~TimeZoneListView();

protected:
	virtual	bool				GetToolTipAt(BPoint point, BToolTip** _tip);

private:
			BTextToolTip*		fToolTip;
};


#endif	// _TIME_ZONE_LIST_VIEW_H
