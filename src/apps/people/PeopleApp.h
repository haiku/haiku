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


#include <Application.h>


#define	B_PERSON_MIMETYPE	"application/x-person"
#define APP_SIG				"application/x-vnd.Be-PEPL"

struct people_field {
	const char*	attribute;
	int32		width;
	const char*	name;
};
extern people_field gFields[];

enum messages{
	M_NEW = 128, M_SAVE, M_SAVE_AS, M_REVERT,
	M_UNDO, M_SELECT, M_GROUP_MENU, M_DIRTY,
	M_NAME, M_NICKNAME, M_COMPANY, M_ADDRESS,
	M_CITY, M_STATE, M_ZIP, M_COUNTRY, M_HPHONE,
	M_WPHONE, M_FAX, M_EMAIL, M_URL, M_GROUP
};

enum fields {
	F_NAME = 0, F_NICKNAME, F_COMPANY, F_ADDRESS,
	F_CITY, F_STATE, F_ZIP, F_COUNTRY, F_HPHONE,
	F_WPHONE, F_FAX, F_EMAIL, F_URL, F_GROUP, F_END
};

class TPeopleWindow;

//====================================================================

class TPeopleApp : public BApplication {
	public:
		TPeopleApp(void);
		virtual ~TPeopleApp(void);

		virtual void	AboutRequested(void);
		virtual void	ArgvReceived(int32, char**);
		virtual void	MessageReceived(BMessage*);
		virtual void	RefsReceived(BMessage*);
		virtual void	ReadyToRun(void);
		TPeopleWindow	*FindWindow(entry_ref);
		TPeopleWindow	*NewWindow(entry_ref* = NULL);

		BFile			*fPrefs;

	private:
		bool			fHaveWindow;
		BRect			fPosition;
};

#endif /* PEOPLEAPP_H */
