/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_UI_SETTINGS_H
#define TEAM_UI_SETTINGS_H


#include <String.h>


class BMessage;


class TeamUISettings {
public:
								TeamUISettings();
								~TeamUISettings();

	virtual	const char*			ID() const = 0;
	virtual	status_t			SetTo(const BMessage& archive) = 0;
	virtual	status_t			WriteTo(BMessage& archive) const = 0;
	
	virtual TeamUISettings*		Clone() const = 0;
									// throws std::bad_alloc

};


#endif	// TEAM_UI_SETTINGS_H
