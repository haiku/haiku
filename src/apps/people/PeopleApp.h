//--------------------------------------------------------------------
//	
//	PeopleApp.h
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef PEOPLEAPP_H
#define PEOPLEAPP_H

#define	B_PERSON_MIMETYPE	"application/x-person"
#define APP_SIG				"application/x-vnd.Be-PEPL"

#define P_NAME				"META:name"
#define P_NICKNAME			"META:nickname"
#define P_COMPANY			"META:company"
#define P_ADDRESS			"META:address"
#define P_CITY				"META:city"
#define P_STATE				"META:state"
#define P_ZIP				"META:zip"
#define P_COUNTRY			"META:country"
#define P_HPHONE			"META:hphone"
#define P_WPHONE			"META:wphone"
#define P_FAX				"META:fax"
#define P_EMAIL				"META:email"
#define P_URL				"META:url"
#define P_GROUP				"META:group"

enum	MESSAGES			{M_NEW = 128, M_SAVE, M_SAVE_AS, M_REVERT,
							 M_UNDO, M_SELECT, M_GROUP_MENU, M_DIRTY,
							 M_NAME, M_NICKNAME, M_COMPANY, M_ADDRESS,
							 M_CITY, M_STATE, M_ZIP, M_COUNTRY, M_HPHONE,
							 M_WPHONE, M_FAX, M_EMAIL, M_URL, M_GROUP};

enum	FIELDS				{F_NAME = 0, F_NICKNAME, F_COMPANY, F_ADDRESS,
							 F_CITY, F_STATE, F_ZIP, F_COUNTRY, F_HPHONE,
							 F_WPHONE, F_FAX, F_EMAIL, F_URL, F_GROUP, F_END};

class	TPeopleWindow;


//====================================================================

#include <Application.h>

class TPeopleApp : public BApplication {

private:

	bool			fHaveWindow;
	BRect			fPosition;

public:

	BFile			*fPrefs;

					TPeopleApp(void);
					~TPeopleApp(void);
	virtual void	AboutRequested(void);
	virtual void	ArgvReceived(int32, char**);
	virtual void	MessageReceived(BMessage*);
	virtual void	RefsReceived(BMessage*);
	virtual void	ReadyToRun(void);
	TPeopleWindow	*FindWindow(entry_ref);
	TPeopleWindow	*NewWindow(entry_ref* = NULL);
};

#endif /* PEOPLEAPP_H */
