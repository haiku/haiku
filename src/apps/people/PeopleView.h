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

#define NAME_H				 10
#define NAME_V				 10
#define STATE_WIDTH			175

class TTextControl;


class TPeopleView : public BView {
	public:
		TPeopleView(BRect rect, char* title, entry_ref* ref);
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

	private:
		BFile			*fFile;
		BPopUpMenu		*fGroups;
		TTextControl	*fField[F_END];

};

#endif /* PEOPLEVIEW_H */
