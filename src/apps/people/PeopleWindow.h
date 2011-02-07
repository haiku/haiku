/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef PEOPLE_WINDOW_H
#define PEOPLE_WINDOW_H


#include <String.h>
#include <Window.h>


#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			321
#define WIND_HEIGHT			340


class TPeopleView;
class BFilePanel;
class BMenuItem;


class TPeopleWindow : public BWindow {
public:

								TPeopleWindow(BRect frame, const char* title,
									const char* nameAttribute,
									const char* categoryAttribute,
									const entry_ref* ref);
	virtual						~TPeopleWindow();

	virtual	void				MenusBeginning();
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
	virtual	void				Show();

			void				AddAttribute(const char* label,
									const char* attribute);

			void				SaveAs();

			bool				RefersPersonFile(const entry_ref& ref) const;

private:
			void				_GetDefaultFileName(char* name);
			void				_SetToRef(entry_ref* ref);
			void				_WatchChanges(bool doIt);

private:
			entry_ref*			fRef;

			BFilePanel*			fPanel;
			BMenuItem*			fCopy;
			BMenuItem*			fCut;
			BMenuItem*			fPaste;
			BMenuItem*			fRevert;
			BMenuItem*			fSave;
			BMenuItem*			fUndo;
			TPeopleView*		fView;

			BString				fNameAttribute;
};


#endif // PEOPLE_WINDOW_H
