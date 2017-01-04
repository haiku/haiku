/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "FilterView.h"

#include <algorithm>
#include <stdio.h>

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include "Model.h"
#include "support.h"


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


FilterView::FilterView()
	:
	BGroupView("filter view", B_VERTICAL)
{
	// Contruct category popup
	BPopUpMenu* showMenu = new BPopUpMenu(B_TRANSLATE("Category"));
	fShowField = new BMenuField("category", B_TRANSLATE("Category:"), showMenu);

	// Construct repository popup
	BPopUpMenu* repositoryMenu = new BPopUpMenu(B_TRANSLATE("Repository"));
	fRepositoryField = new BMenuField("repository", B_TRANSLATE("Repository:"),
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
	// Adopt depots
	BMenu* repositoryMenu = fRepositoryField->Menu();
		repositoryMenu->RemoveItems(0, repositoryMenu->CountItems(), true);

	repositoryMenu->AddItem(new BMenuItem(B_TRANSLATE("All repositories"),
		new BMessage(MSG_DEPOT_SELECTED)));

	repositoryMenu->AddItem(new BSeparatorItem());

	bool foundSelectedDepot = false;
	const DepotList& depots = model.Depots();
	for (int i = 0; i < depots.CountItems(); i++) {
		const DepotInfo& depot = depots.ItemAtFast(i);
		BMessage* message = new BMessage(MSG_DEPOT_SELECTED);
		message->AddString("name", depot.Name());
		BMenuItem* item = new BMenuItem(depot.Name(), message);
		repositoryMenu->AddItem(item);

		if (depot.Name() == model.Depot()) {
			item->SetMarked(true);
			foundSelectedDepot = true;
		}
	}

	if (!foundSelectedDepot)
		repositoryMenu->ItemAt(0)->SetMarked(true);

	// Adopt categories
	BMenu* showMenu = fShowField->Menu();
	showMenu->RemoveItems(0, showMenu->CountItems(), true);

	showMenu->AddItem(new BMenuItem(B_TRANSLATE("All categories"),
		new BMessage(MSG_CATEGORY_SELECTED)));

	showMenu->AddItem(new BSeparatorItem());

	add_categories_to_menu(model.Categories(), showMenu);

	bool foundSelectedCategory = false;
	for (int32 i = 0; i < showMenu->CountItems(); i++) {
		BMenuItem* item = showMenu->ItemAt(i);
		BMessage* message = item->Message();
		if (message == NULL)
			continue;
		BString category;
		if (message->FindString("name", &category) == B_OK
			&& model.Category() == category) {
			item->SetMarked(true);
			foundSelectedCategory = true;
			break;
		}
	}
	if (!foundSelectedCategory)
		showMenu->ItemAt(0)->SetMarked(true);
}

