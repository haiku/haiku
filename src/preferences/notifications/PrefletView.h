/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PREFLET_VIEW_H
#define _PREFLET_VIEW_H

#include <Messenger.h>
#include <TabView.h>

#include "GeneralView.h"

class SettingsHost;

const int32 kShowButtons = '_SHB';
#define kShowButtonsKey "showButtons"

class PrefletView : public BTabView {
public:
						PrefletView(SettingsHost* host);

			BView*		CurrentPage();
			BView*		PageAt(int32 index);
	virtual	void		Select(int32 index);

private:
			GeneralView*	fGeneralView;
			BMessenger		fMessenger;
};

#endif // PREFLETVIEW_H
