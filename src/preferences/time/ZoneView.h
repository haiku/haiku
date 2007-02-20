/*
	ZoneView.h
		Header file for the base class of the Time Zone tab in Time Pref App.
*/

#ifndef ZONE_VIEW_H
#define ZONE_VIEW_H


#include <ListItem.h>
#include <ListView.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <View.h>

#include "TZDisplay.h"

class TZoneItem: public BStringItem {
	public:
		TZoneItem(const char *text, const char *zone);
		virtual ~TZoneItem();
		
		const char *Zone() const;
		const char *Path() const;
		
	private:
		BPath *fZone;
};



class TZoneView: public BView {
	public:
		TZoneView(BRect frame);
		virtual ~TZoneView();
		
		virtual void AttachedToWindow();
		virtual void AllAttached();
		virtual void MessageReceived(BMessage *message);
	protected:
		void UpdateDateTime(BMessage *message);
		void ChangeRegion(BMessage *);
		void SetTimeZone();
		void SetTimeZone(const char *zone);
		void SetPreview();
		void SetCurrent(const char *text);
	private:
		virtual void InitView();
		void ReadTimeZoneLink();
		
		// set widest to font width of longest item
		void BuildRegionMenu(float *widest);

		// returns index of current zone
		int FillCityList(const char *area);

		BPopUpMenu *f_regionpopup;
		BListView *f_citylist;
		BButton *f_setzone;
		TTZDisplay *f_current;
		TTZDisplay *f_preview;
		
		BPath f_currentzone;
		int32 f_hour;
		int32 f_minute;
		int32 f_diff;
		bool f_first;
};

#endif //Zone_View_H
