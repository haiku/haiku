/*
	
	TZoneView.h
*/

#ifndef ZONE_VIEW_H
#define ZONE_VIEW_H

#include <Path.h>
#include <String.h>
#include <ListItem.h>

enum OB_TIME_TARGET
{
	OB_CURRENT_TIME,
	OB_SELECTION_TIME
};

class TZoneItem: public BStringItem
{
	public:
		TZoneItem(const char *text, const char *zonedata);
		virtual ~TZoneItem();
		
		const char *GetZone() const;
	private:
		BPath *f_zonedata;
};

class TZoneView : public BView
{
	public:
		TZoneView(BRect frame);
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		
		void ChangeRegion(BMessage *region);
		void SetTimeZone();
		void NewTimeZone();
	private:
		void InitView();
		void GetCurrentTZ();
		void GetCurrentRegion();

		void FillZoneList(const char *area);
		void CreateZoneMenu(float *widest);
		
		void Update();
		
		void UpdateTimeDisplay(OB_TIME_TARGET target);
		void UpdateDateTime(BMessage *message);
	private:
		BPopUpMenu		*f_zonepopup;
		BListView 		*f_citylist;
		BButton 		*f_settimezone;
		BStringView		*f_seltext;
		BStringView		*f_seltime;
		BStringView		*f_curtext;
		BStringView		*f_curtime;
	private:
		BString 		f_CurrentTime;
		BPath			f_ZoneStr;
		BPath			f_RegionStr;
		
		int 			f_hour;
		int				f_minutes;
		char			f_TimeStr[10];
};									   				

#endif
