/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TeamUiSettingsFactory.h"

#include <Message.h>

#include "GuiTeamUiSettings.h"


TeamUiSettingsFactory::TeamUiSettingsFactory()
{
}


TeamUiSettingsFactory::~TeamUiSettingsFactory()
{
}


status_t
TeamUiSettingsFactory::Create(const BMessage& archive,
	TeamUiSettings*& settings)
{
	int32 type;
	status_t error = archive.FindInt32("type", &type);
	if (error != B_OK)
		return error;

	switch (type) {
		case TEAM_UI_SETTINGS_TYPE_GUI:
			settings = new(std::nothrow) GuiTeamUiSettings();
			if (settings == NULL)
				return B_NO_MEMORY;

			error = settings->SetTo(archive);
			if (error != B_OK) {
				delete settings;
				settings = NULL;
				return error;
			}
			break;

		case TEAM_UI_SETTINGS_TYPE_CLI:
			// TODO: implement once we have a CLI interface
			// (and corresponding settings)
			return B_UNSUPPORTED;

		default:
			return B_BAD_DATA;
	}

	return B_OK;
}
