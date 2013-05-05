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
#include "TeamUiSettings.h"


class GuiTeamUiSettings : public TeamUiSettings {
public:
								GuiTeamUiSettings();
								GuiTeamUiSettings(const char* settingsID);
								GuiTeamUiSettings(const GuiTeamUiSettings&
									other);
									// throws std::bad_alloc
								~GuiTeamUiSettings();

	virtual team_ui_settings_type Type() const;
	virtual	const char*			ID() const;
	virtual	status_t			SetTo(const BMessage& archive);
	virtual	status_t			WriteTo(BMessage& archive) const;
	virtual TeamUiSettings*		Clone() const;

			bool				AddSettings(const char* settingID,
									const BMessage& data);
			status_t			Settings(const char* settingID,
									BMessage& data) const;

			const BMessage&		Values() const;

			GuiTeamUiSettings&	operator=(const GuiTeamUiSettings& other);
									// throws std::bad_alloc

private:

			status_t			_SetTo(const GuiTeamUiSettings& other);
			void				_Unset();

			BMessage			fValues;
			BString				fID;
};


#endif	// GUI_TEAM_UI_SETTINGS_H
