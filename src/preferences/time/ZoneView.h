/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
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


class TZoneView : public BView {
	public:
						TZoneView(BRect frame);
		virtual 		~TZoneView();
		
		virtual void 	AttachedToWindow();
		virtual void 	MessageReceived(BMessage *message);
	
		const char* 	TimeZone();

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

		// returns index of current zone
		int32 			FillCityList(const char *area);

	private:
		BPopUpMenu 		*fRegionPopUp;
		BListView 		*fCityList;
		BButton 		*fSetZone;
		TTZDisplay 		*fCurrent;
		TTZDisplay		*fPreview;
		
		int32 			fHour;
		int32 			fMinute;
		BPath 			fCurrentZone;
		bool 			fNotInitialized;
};

#endif //Zone_View_H

