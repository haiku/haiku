/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef GUI_SETTINGS_UTILS_H
#define GUI_SETTINGS_UTILS_H


#include <SupportDefs.h>


class BSplitView;
class GUITeamUISettings;


class GUISettingsUtils
{
public:

	static 	status_t			ArchiveSplitView(const char* sourceName,
									const char* viewName,
									GUITeamUISettings* settings,
									BSplitView* view);
	static 	void				UnarchiveSplitView(const char* sourceName,
									const char* viewName,
									const GUITeamUISettings* settings,
									BSplitView* view);
};


#endif	// GUI_SETTINGS_UTILS_H
