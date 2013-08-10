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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FilterView"


enum {
	MSG_CATEGORY_SELECTED		= 'ctsl',
	MSG_REPOSITORY_SELECTED		= 'rpsl',
	MSG_SEARCH_TERMS_MODIFIED	= 'stmd',
};


FilterView::FilterView(const Model& model)
	:
	BGroupView("filter view")
{
	// Contruct category popup
	BPopUpMenu* categoryMenu = new BPopUpMenu(B_TRANSLATE("Show"));
	categoryMenu->AddItem(new BMenuItem(B_TRANSLATE("All packages"), NULL));
	categoryMenu->AddItem(new BSeparatorItem());
	
	const CategoryList& categories = model.Categories();
	for (int i = 0; i < categories.CountItems(); i++) {
		const CategoryRef& category = categories.ItemAtFast(i);
		BMessage* message = new BMessage(MSG_CATEGORY_SELECTED);
		message->AddString("name", category->Name());
		BMenuItem* item = new BMenuItem(category->Label(), message);
		categoryMenu->AddItem(item);
	}
	
	categoryMenu->AddItem(new BSeparatorItem());

	categoryMenu->AddItem(new BMenuItem(B_TRANSLATE("Installed packages"),
		NULL));
	categoryMenu->AddItem(new BMenuItem(B_TRANSLATE("Uninstalled packages"),
		NULL));
	categoryMenu->AddItem(new BMenuItem(B_TRANSLATE("User selected packages"),
		NULL));

	categoryMenu->AddItem(new BSeparatorItem());

	categoryMenu->AddItem(new BMenuItem(B_TRANSLATE("Downloading"), NULL));
	categoryMenu->AddItem(new BMenuItem(B_TRANSLATE("Update available"), NULL));
	categoryMenu->ItemAt(0)->SetMarked(true);

	fShowField = new BMenuField("category", B_TRANSLATE("Show:"),
		categoryMenu);

	// Construct repository popup
	BPopUpMenu* repositoryMenu = new BPopUpMenu(B_TRANSLATE("Depot"));
	repositoryMenu->AddItem(new BMenuItem(B_TRANSLATE("All depots"), NULL));
	repositoryMenu->ItemAt(0)->SetMarked(true);
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
	
	// Build layout
	BLayoutBuilder::Group<>(this)
		.Add(fShowField, 0.0f)
		.Add(fRepositoryField, 0.0f)
		.AddGlue(0.5f)
		.Add(fSearchTermsText, 1.0f)
		
		.SetInsets(B_USE_DEFAULT_SPACING)
	;
}


FilterView::~FilterView()
{
}


void
FilterView::AttachedToWindow()
{
	fShowField->Menu()->SetTargetForItems(this);
	fRepositoryField->Menu()->SetTargetForItems(this);
	fSearchTermsText->SetTarget(this);
}


void
FilterView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}
