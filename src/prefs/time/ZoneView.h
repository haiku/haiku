/*
	
	TZoneView.h
*/

#ifndef ZONE_VIEW_H
#define ZONE_VIEW_H

#include <Path.h>
#include <String.h>
#include <ListItem.h>

#include "TZDisplay.h"

class TZoneItem: public BStringItem {
	public:
		TZoneItem(const char *text, const char *zonedata);
		virtual ~TZoneItem();
		
		const char *GetZone() const;
	private:
		BPath *f_zonedata;
};

class TZoneView : public BView {
	public:
		TZoneView(BRect frame);
		~TZoneView();
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		virtual void Draw(BRect);
		
		void ChangeRegion(BMessage *region);
		void SetTimeZone();
		void NewTimeZone();
	protected:
		virtual void UpdateDateTime(BMessage *message);
	private:
		void InitView();
		void GetCurrentTZ();
		void GetCurrentRegion();

		int FillZoneList(const char *area);
		void CreateZoneMenu(float *widest);
		
	private:
		BPopUpMenu *f_zonepopup;
		BListView *f_citylist;
		BButton *f_settimezone;
		TTZDisplay *f_currentzone;
		TTZDisplay *f_timeinzone;
	private:
		BPath f_ZoneStr;
		BPath f_RegionStr;
		
		int f_hour;
		int f_minutes;
		char f_TimeStr[10];
};

#endif
