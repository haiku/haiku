/*
 * Copyright 2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sean Bailey <ziusudra@gmail.com>
 */
#ifndef _TIME_ZONE_LIST_VIEW_H
#define _TIME_ZONE_LIST_VIEW_H


#include <DateFormat.h>
#include <OutlineListView.h>
#include <TimeFormat.h>


class TimeZoneListView : public BOutlineListView {
public:
								TimeZoneListView(void);
	virtual						~TimeZoneListView();

protected:
	virtual	bool				GetToolTipAt(BPoint point, BToolTip** _tip);

private:
	BDateFormat					fDateFormat;
	BTimeFormat					fTimeFormat;
};


#endif	// _TIME_ZONE_LIST_VIEW_H
