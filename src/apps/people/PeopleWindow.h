//--------------------------------------------------------------------
//	
//	PeopleWindow.h
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef PEOPLEWINDOW_H
#define PEOPLEWINDOW_H

#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			321
#define WIND_HEIGHT			340

class TPeopleView;
class BFilePanel;
class BMenuItem;

#include <Window.h>
//====================================================================

class TPeopleWindow : public BWindow {

private:

	BFilePanel		*fPanel;
	BMenuItem		*fCopy;
	BMenuItem		*fCut;
	BMenuItem		*fPaste;
	BMenuItem		*fRevert;
	BMenuItem		*fSave;
	BMenuItem		*fUndo;
	TPeopleView		*fView;

public:

	entry_ref		*fRef;

					TPeopleWindow(BRect, char*, entry_ref*);
					~TPeopleWindow(void);
	virtual void	MenusBeginning(void);
	virtual void	MessageReceived(BMessage*);
	virtual bool	QuitRequested(void);
	void			DefaultName(char*);
	void			SetField(int32, char*);
	void			SaveAs(void);
};

#endif /* PEOPLEWINDOW_H */
