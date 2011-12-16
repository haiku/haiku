/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
 
#include "TeamUISettingsFactory.h"

#include <Message.h>

#include "GUITeamUISettings.h"


TeamUISettingsFactory::TeamUISettingsFactory()
{
}


TeamUISettingsFactory::~TeamUISettingsFactory()
{
}


status_t
TeamUISettingsFactory::Create(const BMessage& archive, TeamUISettings*& 
	settings)
{
	int32 type;
	status_t error = archive.FindInt32("type", &type);
	if (error != B_OK)
		return error;
	
	switch (type) {
		case TEAM_UI_SETTINGS_TYPE_GUI:
			settings = new(std::nothrow) GUITeamUISettings();
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
