/*
 * Copyright 2005-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Stephan Aßmus <superstippi@gmx.de>
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef PEOPLE_APP_H
#define PEOPLE_APP_H


#include <Application.h>
#include <ObjectList.h>
#include <String.h>


#define	B_PERSON_MIMETYPE	"application/x-person"
#define APP_SIG				"application/x-vnd.Be-PEPL"


class BFile;

enum {
	M_NEW					= 'newp',
	M_SAVE_AS				= 'svas',
	M_WINDOW_QUITS			= 'wndq',
	M_CONFIGURE_ATTRIBUTES	= 'catr'
};

class PersonWindow;

class TPeopleApp : public BApplication {
public:
								TPeopleApp();
		virtual 				~TPeopleApp();

		virtual void			ArgvReceived(int32, char**);
		virtual void			MessageReceived(BMessage*);
		virtual void			RefsReceived(BMessage*);
		virtual void			ReadyToRun();

private:
				PersonWindow*	_FindWindow(const entry_ref&) const;
				PersonWindow*	_NewWindow(entry_ref* = NULL);
				void			_AddAttributes(PersonWindow* window) const;
				void			_SavePreferences(BMessage* message) const;

private:
				BFile*			fPrefs;
				uint32			fWindowCount;
				BRect			fPosition;

				struct Attribute {
					BString		attribute;
					int32		width;
					BString		name;
				};

				BObjectList<Attribute> fAttributes;
};

#endif // PEOPLE_APP_H

