/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfoView.h"

#include <algorithm>
#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Message.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageInfoView"


PackageInfoView::PackageInfoView()
	:
	BTabView("package info view", B_WIDTH_FROM_WIDEST)
{
	fDescriptionView = new BView("about", 0);
	AddTab(fDescriptionView);

	fRatingAndCommentsView = new BView("rating and comments", 0);
	AddTab(fRatingAndCommentsView);

	fChangeLogView = new BView("changelog", 0);
	AddTab(fChangeLogView);

	TabAt(0)->SetLabel(B_TRANSLATE("About"));
	TabAt(1)->SetLabel(B_TRANSLATE("Rating & comments"));
	TabAt(2)->SetLabel(B_TRANSLATE("Changelog"));

	Select(0);
}


PackageInfoView::~PackageInfoView()
{
}


void
PackageInfoView::AttachedToWindow()
{
}


void
PackageInfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BTabView::MessageReceived(message);
			break;
	}
}
