/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef ZONE_VIEW_H
#define ZONE_VIEW_H


#include <TimeZone.h>
#include <View.h>


class BMessage;
class BPopUpMenu;
class BOutlineListView;
class BButton;
class TTZDisplay;
class TimeZoneListItem;


class TimeZoneView : public BView {
public:
								TimeZoneView(BRect frame);
	virtual						~TimeZoneView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
			bool				CheckCanRevert();

private:
			void				_UpdateDateTime(BMessage* message);
			void				_SetTimeZone();
			void				_SetTimeZone(const char* zone);
			void				_SetPreview();
			void				_SetCurrent(const char* text);
			void				_InitView();
			void				_BuildRegionMenu();
			void				_Revert();

private:
			BOutlineListView*	fCityList;
			BButton*			fSetZone;
			TTZDisplay*			fCurrent;
			TTZDisplay*			fPreview;

			int32				fHour;
			int32				fMinute;
			TimeZoneListItem*	fCurrentZone;
			TimeZoneListItem*	fOldZone;
			bool				fInitialized;
};


#endif // ZONE_VIEW_H
