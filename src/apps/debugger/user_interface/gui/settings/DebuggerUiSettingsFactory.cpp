/*
 * Copyright 2011-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DebuggerUiSettingsFactory.h"

#include <Message.h>

#include "GuiTeamUiSettings.h"


DebuggerUiSettingsFactory* DebuggerUiSettingsFactory::sDefaultInstance = NULL;


DebuggerUiSettingsFactory::DebuggerUiSettingsFactory()
{
}


DebuggerUiSettingsFactory::~DebuggerUiSettingsFactory()
{
}


DebuggerUiSettingsFactory*
DebuggerUiSettingsFactory::Default()
{
	return sDefaultInstance;
}


status_t
DebuggerUiSettingsFactory::CreateDefault()
{
	sDefaultInstance = new(std::nothrow) DebuggerUiSettingsFactory();
	if (sDefaultInstance == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
DebuggerUiSettingsFactory::DeleteDefault()
{
	delete sDefaultInstance;
	sDefaultInstance = NULL;
}


status_t
DebuggerUiSettingsFactory::Create(const BMessage& archive,
	TeamUiSettings*& settings) const
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
