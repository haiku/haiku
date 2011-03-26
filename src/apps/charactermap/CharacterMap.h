/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHARACTER_MAP_H
#define CHARACTER_MAP_H


#include <Application.h>

class BMessage;
class CharacterWindow;


class CharacterMap : public BApplication {
public:
							CharacterMap();
	virtual					~CharacterMap();

	virtual	void			ReadyToRun();

	virtual	void			RefsReceived(BMessage* message);
	virtual	void			MessageReceived(BMessage* message);

private:
	CharacterWindow*		fWindow;
};

extern const char* kSignature;

#endif	// CHARACTER_MAP_H
