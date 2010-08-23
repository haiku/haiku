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
class BTextToolTip;
class TTZDisplay;
class TimeZoneListItem;


class TimeZoneView : public BView {
public:
								TimeZoneView(BRect frame);
	virtual						~TimeZoneView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
			bool				CheckCanRevert();

protected:
	virtual	bool				GetToolTipAt(BPoint point, BToolTip** _tip);

private:
			void				_UpdateDateTime(BMessage* message);

			void				_SetSystemTimeZone();

			void				_UpdatePreview();
			void				_UpdateCurrent();
			BString				_FormatTime(TimeZoneListItem* zoneItem);

			void				_InitView();
			void				_BuildZoneMenu();

			void				_Revert();

private:
			BOutlineListView*	fZoneList;
			BButton*			fSetZone;
			TTZDisplay*			fCurrent;
			TTZDisplay*			fPreview;

			BTextToolTip*		fToolTip;

			int32				fLastUpdateMinute;

			TimeZoneListItem*	fCurrentZoneItem;
			TimeZoneListItem*	fOldZoneItem;
			bool				fInitialized;
};


#endif // ZONE_VIEW_H
