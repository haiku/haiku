/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NOTIFICATION_WINDOW_H
#define _NOTIFICATION_WINDOW_H

#include <cmath>
#include <vector>
#include <map>

#include <AppFileInfo.h>
#include <String.h>
#include <Window.h>

#include "NotificationView.h"


class AppGroupView;
class AppUsage;

struct property_info;

typedef std::map<BString, AppGroupView*> appview_t;
typedef std::map<BString, AppUsage*> appfilter_t;
typedef std::vector<NotificationView*> views_t;

extern const float kEdgePadding;
extern const float kSmallPadding;
extern const float kCloseSize;
extern const float kExpandSize;
extern const float kPenSize;

const uint32 kRemoveGroupView = 'RGVi';


class NotificationWindow : public BWindow {
public:
									NotificationWindow();
	virtual							~NotificationWindow();

	virtual	bool					QuitRequested();
	virtual	void					MessageReceived(BMessage*);
	virtual	void 					WorkspaceActivated(int32, bool);
	virtual	BHandler*				ResolveSpecifier(BMessage*, int32, BMessage*,
										int32, const char*);
										
			icon_size				IconSize();
			int32					Timeout();
			float					Width();

			void					_ShowHide();

private:
	friend class AppGroupView;

			void					SetPosition();
			void					_LoadSettings(bool startMonitor = false);
			void					_LoadAppFilters(bool startMonitor = false);
			void					_SaveAppFilters();
			void					_LoadGeneralSettings(bool startMonitor);
			void					_LoadDisplaySettings(bool startMonitor);

			views_t					fViews;
			appview_t				fAppViews;

			BString					fStatusText;
			BString					fMessageText;

			float					fWidth;
			icon_size				fIconSize;
			int32					fTimeout;

			appfilter_t				fAppFilters;
};

extern property_info main_prop_list[];

#endif	// _NOTIFICATION_WINDOW_H
