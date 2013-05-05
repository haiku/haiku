/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_UI_SETTINGS_FACTORY_H
#define TEAM_UI_SETTINGS_FACTORY_H


#include <SupportDefs.h>


class BMessage;
class TeamUiSettings;

class TeamUiSettingsFactory {
public:
								TeamUiSettingsFactory();
								~TeamUiSettingsFactory();
						
	static	status_t			Create(const BMessage& archive,
									TeamUiSettings*& settings);
};

#endif // TEAM_UI_SETTINGS_FACTORY_H
