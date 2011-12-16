/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PREFLET_VIEW_H
#define _PREFLET_VIEW_H

#include <TabView.h>

class BIconRule;

class SettingsHost;

class PrefletView : public BTabView {
public:
						PrefletView(SettingsHost* host);

			BView*		CurrentPage();
			int32		CountPages() const;
			BView*		PageAt(int32 index);
};

#endif // PREFLETVIEW_H
