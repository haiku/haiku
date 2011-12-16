/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_UI_SETTINGS_H
#define TEAM_UI_SETTINGS_H


#include <String.h>


class BMessage;


enum team_ui_settings_type {
	TEAM_UI_SETTINGS_TYPE_GUI,
	TEAM_UI_SETTINGS_TYPE_CLI
};


class TeamUISettings {
public:
								TeamUISettings();
	virtual						~TeamUISettings();

	virtual team_ui_settings_type Type() const = 0;
	virtual	const char*			ID() const = 0;
	virtual	status_t			SetTo(const BMessage& archive) = 0;
	virtual	status_t			WriteTo(BMessage& archive) const = 0;
	
	virtual TeamUISettings*		Clone() const = 0;
									// throws std::bad_alloc

};


#endif	// TEAM_UI_SETTINGS_H
