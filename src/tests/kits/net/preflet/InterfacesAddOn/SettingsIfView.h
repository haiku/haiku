/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */

#ifndef SETTINGS_IFVIEW_H
#define SETTINGS_IFVIEW_H

#include "NetworkSettings.h"

#include <Screen.h>
#include <View.h>


class SettingsIfView : public BView {
public:
								SettingsIfView(BRect frame, const char* name,
									int family, NetworkSettings* settings);
	virtual						~SettingsIfView();

private:
			NetworkSettings*	fSettings;
			int					fFamily;
};


#endif /* SETTINGS_IFVIEW_H */

