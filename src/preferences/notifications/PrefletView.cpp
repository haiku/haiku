/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <Message.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <CardLayout.h>
#include <LayoutItem.h>

#include "SettingsHost.h"
#include "PrefletView.h"
#include "IconRule.h"
#include "GeneralView.h"
#include "DisplayView.h"
#include "NotificationsView.h"

#define _T(str) (str)

const int32 kPageSelected = '_LCH';


PrefletView::PrefletView(SettingsHost* host)
	:
	BView("pages", B_WILL_DRAW)
{
	// Page selector
	fRule = new BIconRule("icon_rule");
	fRule->SetSelectionMessage(new BMessage(kPageSelected));
	fRule->AddIcon(_T("General"), NULL);
	fRule->AddIcon(_T("Display"), NULL);
	//fRule->AddIcon(_T("Notifications"), NULL);

	// View for card layout
	fPagesView = new BView("pages", B_WILL_DRAW);

	// Pages
	GeneralView* general = new GeneralView(host);
	DisplayView* display = new DisplayView(host);
	NotificationsView* apps = new NotificationsView();

	// Calculate inset
	float inset = ceilf(be_plain_font->Size() * 0.7f);

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	// Card layout for pages
	BCardLayout* layout = new BCardLayout();
	fPagesView->SetLayout(layout);
	layout->AddView(general);
	layout->AddView(display);
	layout->AddView(apps);

	// Add childs
	AddChild(BGroupLayoutBuilder(B_VERTICAL, inset)
		.Add(fRule)
		.Add(fPagesView)
	);

	// Select the first view
	Select(0);
}


void
PrefletView::AttachedToWindow()
{
	fRule->SetTarget(this);
}


void
PrefletView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kPageSelected:
		{
			int32 index = B_ERROR;
			if (message->FindInt32("index", &index) != B_OK)
				return;

			Select(index);
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}


void
PrefletView::Select(int32 index)
{
	BCardLayout* layout
		= dynamic_cast<BCardLayout*>(fPagesView->GetLayout());
	if (layout)
		layout->SetVisibleItem(index);
}


BView*
PrefletView::CurrentPage()
{
	BCardLayout* layout
		= dynamic_cast<BCardLayout*>(fPagesView->GetLayout());
	if (layout)
		return layout->VisibleItem()->View();
	return NULL;
}


int32
PrefletView::CountPages() const
{
	BCardLayout* layout
		= dynamic_cast<BCardLayout*>(fPagesView->GetLayout());
	if (layout)
		return layout->CountItems();
	return 0;
}


BView*
PrefletView::PageAt(int32 index)
{
	BCardLayout* layout
		= dynamic_cast<BCardLayout*>(fPagesView->GetLayout());
	if (layout)
		return layout->ItemAt(index)->View();
	return NULL;
}
