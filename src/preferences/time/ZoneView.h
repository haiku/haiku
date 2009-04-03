/*
 * Copyright 2004-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef ZONE_VIEW_H
#define ZONE_VIEW_H


#include <View.h>
#include <Path.h>


class BMessage;
class BPopUpMenu;
class BListView;
class BButton;
class TTZDisplay;


class TimeZoneView : public BView {
	public:
						TimeZoneView(BRect frame);
		virtual 		~TimeZoneView();

		virtual void 	AttachedToWindow();
		virtual void 	MessageReceived(BMessage *message);
		bool			CheckCanRevert();

	private:
		void 			UpdateDateTime(BMessage *message);
		void 			ChangeRegion(BMessage *);
		void 			SetTimeZone();
		void 			SetTimeZone(const char *zone);
		void 			SetPreview();
		void 			SetCurrent(const char *text);
		void 			InitView();
		void 			ReadTimeZoneLink();
		void 			BuildRegionMenu();
		void			_Revert();

		// returns index of current zone
		int32 			FillCityList(const char *area);

		status_t		_GetTimezonesPath(BPath& path);

	private:
		BPopUpMenu 		*fRegionPopUp;
		BListView 		*fCityList;
		BButton 		*fSetZone;
		TTZDisplay 		*fCurrent;
		TTZDisplay		*fPreview;

		int32 			fHour;
		int32 			fMinute;
		BPath 			fCurrentZone;
		BPath				fOldZone;
		bool 			fInitialized;
};

#endif //Zone_View_H

