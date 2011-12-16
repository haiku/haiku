/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <Catalog.h>
#include <Message.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <CardLayout.h>
#include <LayoutItem.h>

#include "SettingsHost.h"
#include "PrefletView.h"
#include "GeneralView.h"
#include "DisplayView.h"
#include "NotificationsView.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "PrefletView"


const int32 kPageSelected = '_LCH';


PrefletView::PrefletView(SettingsHost* host)
	:
	BTabView("pages")
{
	// Pages
	GeneralView* general = new GeneralView(host);
	DisplayView* display = new DisplayView(host);
	NotificationsView* apps = new NotificationsView();

	// Page selector
	BTab* tab = new BTab();
	AddTab(general, tab);
	tab->SetLabel(B_TRANSLATE("General"));

	tab = new BTab();
	AddTab(display, tab);
	tab->SetLabel(B_TRANSLATE("Display"));

	tab = new BTab();
	AddTab(apps, tab);
	tab->SetLabel(B_TRANSLATE("Notifications"));
}


BView*
PrefletView::CurrentPage()
{
	return PageAt(FocusTab());
}


int32
PrefletView::CountPages() const
{
	return 3;
}


BView*
PrefletView::PageAt(int32 index)
{
	return TabAt(index)->View();
}
