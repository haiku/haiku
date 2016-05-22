/*
 * Copyright 2011-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_UI_SETTINGS_FACTORY_H
#define DEBUGGER_UI_SETTINGS_FACTORY_H


#include "TeamUiSettingsFactory.h"


class DebuggerUiSettingsFactory : public TeamUiSettingsFactory {
public:

	static	DebuggerUiSettingsFactory*	Default();
	static	status_t			CreateDefault();
	static	void				DeleteDefault();

	virtual	status_t			Create(const BMessage& archive,
									TeamUiSettings*& settings) const;

private:
								DebuggerUiSettingsFactory();
	virtual						~DebuggerUiSettingsFactory();

	static	DebuggerUiSettingsFactory* sDefaultInstance;
};

#endif // DEBUGGER_UI_SETTINGS_FACTORY_H
