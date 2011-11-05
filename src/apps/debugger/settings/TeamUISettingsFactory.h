/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_UI_SETTINGS_FACTORY_H
#define TEAM_UI_SETTINGS_FACTORY_H


#include <SupportDefs.h>


class BMessage;
class TeamUISettings;

class TeamUISettingsFactory {
public:
								TeamUISettingsFactory();
								~TeamUISettingsFactory();
						
	static	status_t			Create(const BMessage& archive,
									TeamUISettings*& settings);
};

#endif // TEAM_UI_SETTINGS_FACTORY_H
