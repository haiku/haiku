/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef GUI_TEAM_UI_SETTINGS_H
#define GUI_TEAM_UI_SETTINGS_H


#include <String.h>

#include <ObjectList.h>

#include "TeamUISettings.h"

class BMessage;


class GUITeamUISettings : public TeamUISettings {
public:
								GUITeamUISettings();
								GUITeamUISettings(const char* settingsID);
								GUITeamUISettings(const GUITeamUISettings&
									other);
									// throws std::bad_alloc
								~GUITeamUISettings();

	virtual team_ui_settings_type Type() const;
	virtual	const char*			ID() const;
	virtual	status_t			SetTo(const BMessage& archive);
	virtual	status_t			WriteTo(BMessage& archive) const;
	virtual TeamUISettings*		Clone() const;

			GUITeamUISettings&	operator=(const GUITeamUISettings& other);
									// throws std::bad_alloc

private:

			BString				fID;			
};


#endif	// GUI_TEAM_UI_SETTINGS_H
