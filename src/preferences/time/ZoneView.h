/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Hamish Morrison <hamish@lavabit.com>
 */
#ifndef ZONE_VIEW_H
#define ZONE_VIEW_H


#include <LayoutBuilder.h>
#include <TimeZone.h>


class BButton;
class BMessage;
class BOutlineListView;
class BPopUpMenu;
class BTextToolTip;
class BTimeZone;
class TimeZoneListItem;
class TTZDisplay;


class TimeZoneView : public BGroupView {
public:
								TimeZoneView(const char* name);
	virtual						~TimeZoneView();

	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				MessageReceived(BMessage* message);
			bool				CheckCanRevert();

protected:
	virtual	bool				GetToolTipAt(BPoint point, BToolTip** _tip);

private:
			void				_UpdateDateTime(BMessage* message);

			void				_SetSystemTimeZone();

			void				_UpdatePreview();
			void				_UpdateCurrent();
			BString				_FormatTime(const BTimeZone& timeZone);

			void				_InitView();
			void				_BuildZoneMenu();

			void				_Revert();

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
