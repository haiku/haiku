/*
	
	ZoneView.h
*/

#ifndef ZONE_VIEW_H
#define ZONE_VIEW_H

class ZoneView : public BView
{
public:
						ZoneView(BRect frame);
		void			ChangeRegion(BMessage *region);
		void			setTimeZone();
		void			newTimeZone();
		virtual void	Pulse();
		virtual void	Draw(BRect updateRect);
private:
		void			buildView();
		void			getCurrentTime();
		void			getCurrentTZ();
			
		
		BPopUpMenu		*fZonePopUp;
		BListView 		*fCityList;
		BButton 		*fSetTimeZone;
		char			fTimeStr[10];
		char			fZoneStr[B_FILE_NAME_LENGTH];  
};									   				

#endif
