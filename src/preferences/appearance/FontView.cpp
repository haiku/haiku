/*
 * Copyright 2001-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Philippe St-Pierre, stpere@gmail.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "FontView.h"

#include <algorithm>

#include <string.h>

#include <Catalog.h>
#include <ControlLook.h>
#include <GridLayout.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MessageRunner.h>
#include <SpaceLayoutItem.h>

#include "APRWindow.h"
#include "FontSelectionView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Font view"


static const uint32 kMsgCheckFonts = 'chkf';


FontView::FontView(const char* name)
	:
	BView(name, B_WILL_DRAW )
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	const char* plainLabel = B_TRANSLATE("Plain font:");
	const char* boldLabel = B_TRANSLATE("Bold font:");
	const char* fixedLabel = B_TRANSLATE("Fixed font:");
	const char* menuLabel = B_TRANSLATE("Menu font:");

	fPlainView = new FontSelectionView("plain", plainLabel);
	fBoldView = new FontSelectionView("bold", boldLabel);
	fFixedView = new FontSelectionView("fixed", fixedLabel);
	fMenuView = new FontSelectionView("menu", menuLabel);

	// find the longest label
	float longestLabel = StringWidth(plainLabel);
	longestLabel = std::max(longestLabel, StringWidth(boldLabel));
	longestLabel = std::max(longestLabel, StringWidth(fixedLabel));
	longestLabel = std::max(longestLabel, StringWidth(menuLabel));
	longestLabel += be_control_look->DefaultLabelSpacing();

	// set the first column to the width of the longest label
	BGridLayout* gridLayout;
	gridLayout = (BGridLayout*)fPlainView->GetLayout();
	gridLayout->SetMinColumnWidth(0, longestLabel);
	gridLayout = (BGridLayout*)fBoldView->GetLayout();
	gridLayout->SetMinColumnWidth(0, longestLabel);
	gridLayout = (BGridLayout*)fFixedView->GetLayout();
	gridLayout->SetMinColumnWidth(0, longestLabel);
	gridLayout = (BGridLayout*)fMenuView->GetLayout();
	gridLayout->SetMinColumnWidth(0, longestLabel);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fPlainView)
		.Add(fBoldView)
		.Add(fFixedView)
		.Add(fMenuView)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
		.SetInsets(B_USE_WINDOW_SPACING);
}


void
FontView::AttachedToWindow()
{
	fPlainView->SetTarget(this);
	fBoldView->SetTarget(this);
	fFixedView->SetTarget(this);
	fMenuView->SetTarget(this);

	UpdateFonts();
	fRunner = new BMessageRunner(this, new BMessage(kMsgCheckFonts), 3000000);
		// every 3 seconds
}


void
FontView::DetachedFromWindow()
{
	delete fRunner;
	fRunner = NULL;
}


void
FontView::SetDefaults()
{
	fPlainView->SetDefaults();
	fBoldView->SetDefaults();
	fFixedView->SetDefaults();
	fMenuView->SetDefaults();
}


void
FontView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSetSize:
		case kMsgSetFamily:
		case kMsgSetStyle:
		{
			const char* name;
			if (message->FindString("name", &name) != B_OK)
				break;

			if (!strcmp(name, "plain"))
				fPlainView->MessageReceived(message);
			else if (!strcmp(name, "bold"))
				fBoldView->MessageReceived(message);
			else if (!strcmp(name, "fixed"))
				fFixedView->MessageReceived(message);
			else if (!strcmp(name, "menu"))
				fMenuView->MessageReceived(message);
			else
				break;

			Window()->PostMessage(kMsgUpdate);
			break;
		}

		case kMsgCheckFonts:
			if (update_font_families(true))
				UpdateFonts();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
FontView::Revert()
{
	fPlainView->Revert();
	fBoldView->Revert();
	fFixedView->Revert();
	fMenuView->Revert();
}


void
FontView::UpdateFonts()
{
	fPlainView->UpdateFontsMenu();
	fBoldView->UpdateFontsMenu();
	fFixedView->UpdateFontsMenu();
	fMenuView->UpdateFontsMenu();
}


bool
FontView::IsDefaultable()
{
	return fPlainView->IsDefaultable()
		|| fBoldView->IsDefaultable()
		|| fFixedView->IsDefaultable()
		|| fMenuView->IsDefaultable();
}


bool
FontView::IsRevertable()
{
	return fPlainView->IsRevertable()
		|| fBoldView->IsRevertable()
		|| fFixedView->IsRevertable()
		|| fMenuView->IsRevertable();
}

