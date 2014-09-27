/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "FilterView.h"

#include <algorithm>
#include <stdio.h>

#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <TextControl.h>

#include "Model.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FilterView"


static void
add_categories_to_menu(const CategoryList& categories, BMenu* menu)
{
	for (int i = 0; i < categories.CountItems(); i++) {
		const CategoryRef& category = categories.ItemAtFast(i);
		BMessage* message = new BMessage(MSG_CATEGORY_SELECTED);
		message->AddString("name", category->Name());
		BMenuItem* item = new BMenuItem(category->Label(), message);
		menu->AddItem(item);
	}
}


static void
set_small_font(BView* view)
{
	BFont font;
	view->GetFont(&font);
	font.SetSize(ceilf(font.Size() * 0.8));
	view->SetFont(&font);
}


static BCheckBox*
create_check_box(const char* label, const char* name)
{
	BMessage* message = new BMessage(MSG_FILTER_SELECTED);
	message->AddString("name", name);
	BCheckBox* checkBox = new BCheckBox(label, message);
	set_small_font(checkBox);
	return checkBox;
}


FilterView::FilterView()
	:
	BGroupView("filter view", B_VERTICAL)
{
	// Contruct category popup
	BPopUpMenu* showMenu = new BPopUpMenu(B_TRANSLATE("Category"));
	fShowField = new BMenuField("category", B_TRANSLATE("Category:"), showMenu);

	// Construct repository popup
	BPopUpMenu* repositoryMenu = new BPopUpMenu(B_TRANSLATE("Depot"));
	fRepositoryField = new BMenuField("repository", B_TRANSLATE("Depot:"),
		repositoryMenu);

	// Construct search terms field
	fSearchTermsText = new BTextControl("search terms",
		B_TRANSLATE("Search terms:"), "", NULL);
	fSearchTermsText->SetModificationMessage(
		new BMessage(MSG_SEARCH_TERMS_MODIFIED));

	BSize minSearchSize = fSearchTermsText->MinSize();
	float minSearchWidth
		= be_plain_font->StringWidth(fSearchTermsText->Label())
			+ be_plain_font->StringWidth("XXX") * 6;
	minSearchWidth = std::max(minSearchSize.width, minSearchWidth);
	minSearchSize.width = minSearchWidth;
	fSearchTermsText->SetExplicitMinSize(minSearchSize);
	float maxSearchWidth = minSearchWidth * 2;
	fSearchTermsText->SetExplicitMaxSize(BSize(maxSearchWidth, B_SIZE_UNSET));

	// Construct check boxen
	fAvailableCheckBox = create_check_box(
		B_TRANSLATE("Available"), "available");
	fInstalledCheckBox = create_check_box(
		B_TRANSLATE("Installed"), "installed");
	fDevelopmentCheckBox = create_check_box(
		B_TRANSLATE("Development"), "development");
	fSourceCodeCheckBox = create_check_box(
		B_TRANSLATE("Source code"), "source code");

	// Logged in user label
	fUsername = new BStringView("logged in user", "");
	set_small_font(fUsername);
	fUsername->SetHighColor(80, 80, 80);
	
	// Build layout
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1.2f)
				.Add(fShowField, 0.0f)
				.Add(fRepositoryField, 0.0f)
				.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
			.End()
			.AddGlue(0.5f)
			.Add(fSearchTermsText, 1.0f)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fAvailableCheckBox)
			.Add(fInstalledCheckBox)
			.Add(fDevelopmentCheckBox)
			.Add(fSourceCodeCheckBox)
			.AddGlue(0.5f)
			.Add(fUsername)
		.End()

		.SetInsets(B_USE_DEFAULT_SPACING)
	;
}


FilterView::~FilterView()
{
}


void
FilterView::AttachedToWindow()
{
	fShowField->Menu()->SetTargetForItems(Window());
	fRepositoryField->Menu()->SetTargetForItems(Window());
	fSearchTermsText->SetTarget(this);

	fSearchTermsText->MakeFocus();

	fAvailableCheckBox->SetTarget(Window());
	fInstalledCheckBox->SetTarget(Window());
	fDevelopmentCheckBox->SetTarget(Window());
	fSourceCodeCheckBox->SetTarget(Window());
}


void
FilterView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SEARCH_TERMS_MODIFIED:
		{
			BMessage searchTerms(MSG_SEARCH_TERMS_MODIFIED);
			searchTerms.AddString("search terms", fSearchTermsText->Text());
			Window()->PostMessage(&searchTerms);
			break;
		}

		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
FilterView::AdoptModel(const Model& model)
{
	BMenu* repositoryMenu = fRepositoryField->Menu();
		repositoryMenu->RemoveItems(0, repositoryMenu->CountItems(), true);

	repositoryMenu->AddItem(new BMenuItem(B_TRANSLATE("All depots"),
		new BMessage(MSG_DEPOT_SELECTED)));
	repositoryMenu->ItemAt(0)->SetMarked(true);

	repositoryMenu->AddItem(new BSeparatorItem());

	const DepotList& depots = model.Depots();
	for (int i = 0; i < depots.CountItems(); i++) {
		const DepotInfo& depot = depots.ItemAtFast(i);
		BMessage* message = new BMessage(MSG_DEPOT_SELECTED);
		message->AddString("name", depot.Name());
		BMenuItem* item = new BMenuItem(depot.Name(), message);
		repositoryMenu->AddItem(item);
	}

	BMenu* showMenu = fShowField->Menu();
	showMenu->RemoveItems(0, showMenu->CountItems(), true);

	showMenu->AddItem(new BMenuItem(B_TRANSLATE("All categories"),
		new BMessage(MSG_CATEGORY_SELECTED)));

	showMenu->AddItem(new BSeparatorItem());

	add_categories_to_menu(model.Categories(), showMenu);

	showMenu->ItemAt(0)->SetMarked(true);

	AdoptCheckmarks(model);
}


void
FilterView::AdoptCheckmarks(const Model& model)
{
	fAvailableCheckBox->SetValue(model.ShowAvailablePackages());
	fInstalledCheckBox->SetValue(model.ShowInstalledPackages());
	fDevelopmentCheckBox->SetValue(model.ShowDevelopPackages());
	fSourceCodeCheckBox->SetValue(model.ShowSourcePackages());
}


void
FilterView::SetUsername(const BString& username)
{
	BString label;
	if (username.Length() == 0) {
		label = B_TRANSLATE("Not logged in");
	} else {
		label = B_TRANSLATE("Logged in as %User%");
		label.ReplaceAll("%User%", username);
	}
	fUsername->SetText(label);
}

