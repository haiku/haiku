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


class TimeZoneListView : public BOutlineListView {
public:
								TimeZoneListView(void);
								~TimeZoneListView();

protected:
	virtual	bool				GetToolTipAt(BPoint point, BToolTip** _tip);
};


#endif	// _TIME_ZONE_LIST_VIEW_H
