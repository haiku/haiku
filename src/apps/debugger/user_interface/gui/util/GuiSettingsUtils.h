/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef GUI_SETTINGS_UTILS_H
#define GUI_SETTINGS_UTILS_H


#include <SupportDefs.h>


class AbstractTable;
class BMessage;
class BSplitView;


class GuiSettingsUtils {
public:

	static 	status_t			ArchiveSplitView(BMessage& settings,
									BSplitView* view);
	static 	void				UnarchiveSplitView(const BMessage& settings,
									BSplitView* view);

	static	status_t			ArchiveTableSettings(BMessage& settings,
									AbstractTable* table);

	static	void				UnarchiveTableSettings(
									const BMessage& settings,
									AbstractTable* table);


};


#endif	// GUI_SETTINGS_UTILS_H
