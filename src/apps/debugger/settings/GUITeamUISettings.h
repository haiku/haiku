/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef GUI_TEAM_UI_SETTINGS_H
#define GUI_TEAM_UI_SETTINGS_H


#include <Message.h>
#include <String.h>

#include <ObjectList.h>

#include "Setting.h"
#include "TeamUISettings.h"


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

			bool				SetValue(const char* settingID,
									const BVariant& value);
			status_t			Value(const char* settingID,
									BVariant& value) const;

			GUITeamUISettings&	operator=(const GUITeamUISettings& other);
									// throws std::bad_alloc

private:

			status_t			_SetTo(const GUITeamUISettings& other);
			void				_Unset();

			BMessage			fValues;
			BString				fID;
};


#endif	// GUI_TEAM_UI_SETTINGS_H
