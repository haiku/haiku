/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageActionsView.h"

#include <algorithm>
#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Message.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageActionsView"


enum {
	MSG_INSTALL			= 'inst',
	MSG_TOGGLE_ACTIVE	= 'tgac',
	MSG_UPDATE			= 'stmd',
	MSG_UNINSTALL		= 'dein',
};


PackageActionsView::PackageActionsView()
	:
	BGroupView("package actions view")
{
	// Contruct action buttons
	fInstallButton = new BButton("install", B_TRANSLATE("Install"),
		new BMessage(MSG_INSTALL));

	fToggleActiveButton = new BButton("toggle active",
		B_TRANSLATE("Deactivate"), new BMessage(MSG_TOGGLE_ACTIVE));

	fUpdateButton = new BButton("update",
		B_TRANSLATE("Update"), new BMessage(MSG_UPDATE));

	fUninstallButton = new BButton("uninstall", B_TRANSLATE("Uninstall"),
		new BMessage(MSG_UNINSTALL));
	
	// Build layout
	BLayoutBuilder::Group<>(this)
		.AddGlue(1.0f)
		.Add(fUninstallButton)
		.AddGlue(0.1f)
		.Add(fUpdateButton)
		.Add(fToggleActiveButton)
		.Add(fInstallButton)
	;
}


PackageActionsView::~PackageActionsView()
{
}


void
PackageActionsView::AttachedToWindow()
{
	fInstallButton->SetTarget(this);
	fToggleActiveButton->SetTarget(this);
	fUpdateButton->SetTarget(this);
	fUninstallButton->SetTarget(this);
}


void
PackageActionsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}
