/*
 * Copyright 2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sean Bailey <ziusudra@gmail.com>
*/


#include "TimeZoneListView.h"

#include <new>

#include <Catalog.h>
#include <Locale.h>
#include <String.h>
#include <TimeZone.h>
#include <ToolTip.h>

#include "TimeZoneListItem.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Time"


TimeZoneListView::TimeZoneListView(void)
	:
	BOutlineListView("cityList", B_SINGLE_SELECTION_LIST)
{
}


TimeZoneListView::~TimeZoneListView()
{
}


bool
TimeZoneListView::GetToolTipAt(BPoint point, BToolTip** _tip)
{
	TimeZoneListItem* item = static_cast<TimeZoneListItem*>(
		this->ItemAt(this->IndexOf(point)));
	if (item == NULL || !item->HasTimeZone())
		return false;

	BString nowInTimeZone;
	time_t now = time(NULL);
	BLocale::Default()->FormatTime(&nowInTimeZone, now, B_SHORT_TIME_FORMAT,
		&item->TimeZone());

	BString dateInTimeZone;
	BLocale::Default()->FormatDate(&dateInTimeZone, now, B_SHORT_DATE_FORMAT,
		&item->TimeZone());

	BString toolTip = item->Text();
	toolTip << '\n' << item->TimeZone().ShortName() << " / "
			<< item->TimeZone().ShortDaylightSavingName()
			<< B_TRANSLATE("\nNow: ") << nowInTimeZone
			<< " (" << dateInTimeZone << ')';

	SetToolTip(toolTip.String());

	*_tip = ToolTip();

	return true;
}
