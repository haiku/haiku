// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        KeymapApplication.h
//  Author:      Sandor Vroemisse, Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 12, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef KEYMAP_APPLICATION_H
#define KEYMAP_APPLICATION_H

#include <storage/Entry.h>

#define APP_SIGNATURE		"application/x-vnd.haiku-Keymap"

class KeymapWindow;

class KeymapApplication : public BApplication {
public:
	KeymapApplication();
	void	MessageReceived(BMessage *message);
	
	bool	UseKeymap( BEntry *keymap );

protected:
	KeymapWindow		*fWindow;

	bool 	IsValidKeymap( BFile *inFile );
	int		NotifyInputServer();

};

#endif // KEYMAP_APPLICATION_H
