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
#ifndef PERSON_WINDOW_H
#define PERSON_WINDOW_H


#include <String.h>
#include <Window.h>


#define	TITLE_BAR_HEIGHT	 32
#define	WIND_WIDTH			420
#define WIND_HEIGHT			600


class PersonView;
class BFilePanel;
class BMenuItem;


class PersonWindow : public BWindow {
public:

								PersonWindow(BRect frame, const char* title,
									const char* nameAttribute,
									const char* categoryAttribute,
									const entry_ref* ref);
	virtual						~PersonWindow();

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
			PersonView*			fView;

			BString				fNameAttribute;
};


#endif // PERSON_WINDOW_H
