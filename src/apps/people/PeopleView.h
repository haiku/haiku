//--------------------------------------------------------------------
//	
//	PeopleView.h
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef PEOPLEVIEW_H
#define PEOPLEVIEW_H

#include "PeopleApp.h"

#define TEXT_HEIGHT			 16
#define NAME_H				 10
#define NAME_V				 10
#define NAME_WIDTH			300
#define NAME_TEXT			"Name"
#define NICKNAME_H			 10
#define NICKNAME_V			(NAME_V + 25)
#define NICKNAME_WIDTH		300
#define NICKNAME_TEXT		"Nickname"
#define COMPANY_H			 10
#define COMPANY_V			(NICKNAME_V + 25)
#define COMPANY_WIDTH		300
#define COMPANY_TEXT		"Company"
#define ADDRESS_H			 10
#define ADDRESS_V			(COMPANY_V + 25)
#define ADDRESS_WIDTH		300
#define ADDRESS_TEXT		"Address"
#define CITY_H				 10
#define CITY_V				(ADDRESS_V + 25)
#define CITY_WIDTH			300
#define CITY_TEXT			"City"
#define STATE_H				 10
#define STATE_V				(CITY_V + 25)
#define STATE_WIDTH			175
#define STATE_TEXT			"State"
#define ZIP_H				(STATE_H + STATE_WIDTH)
#define ZIP_V				(CITY_V + 25)
#define ZIP_WIDTH			125
#define ZIP_TEXT			"Zip"
#define COUNTRY_H			 10
#define COUNTRY_V			(ZIP_V + 25)
#define COUNTRY_WIDTH		300
#define COUNTRY_TEXT		"Country"
#define HPHONE_H			 10
#define HPHONE_V			(COUNTRY_V + 25)
#define HPHONE_WIDTH		300
#define HPHONE_TEXT			"Home Phone"
#define WPHONE_H			 10
#define WPHONE_V			(HPHONE_V + 25)
#define WPHONE_WIDTH		300
#define WPHONE_TEXT			"Work Phone"
#define FAX_H				 10
#define FAX_V				(WPHONE_V + 25)
#define FAX_WIDTH			300
#define FAX_TEXT			"Fax"
#define EMAIL_H				 10
#define EMAIL_V				(FAX_V + 25)
#define EMAIL_WIDTH			300
#define EMAIL_TEXT			"E-mail"
#define URL_H				 10
#define URL_V				(EMAIL_V + 25)
#define URL_WIDTH			300
#define URL_TEXT			"URL"
#define GROUP_H				 10
#define GROUP_V				(URL_V + 25)
#define GROUP_WIDTH			300
#define GROUP_TEXT			"Group"

class TTextControl;


//====================================================================

class TPeopleView : public BView {

private:

	BFile			*fFile;
	BPopUpMenu		*fGroups;
	TTextControl	*fField[F_END];

public:

					TPeopleView(BRect, char*, entry_ref*);
					~TPeopleView(void); 
	virtual	void	AttachedToWindow(void);
	virtual void	MessageReceived(BMessage*);
	void			BuildGroupMenu(void);
	bool			CheckSave(void);
	const char*		GetField(int32);
	void			NewFile(entry_ref*);
	void			Save(void);
	void			SetField(int32, char*, bool);
	bool			TextSelected(void);
};

#endif /* PEOPLEVIEW_H */
