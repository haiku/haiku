/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SETTINGS_HOST_H
#define _SETTINGS_HOST_H

#include <vector>

#include "SettingsPane.h"

class SettingsHost {
public:
					SettingsHost() {}

	virtual	void	SettingChanged(bool showExample = false) = 0;
};

#endif // _SETTINGS_HOST_H
