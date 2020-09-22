/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "FilterView.h"

#include <algorithm>
#include <stdio.h>

#include <AutoLocker.h>
#include <Catalog.h>
#include <CheckBox.h>
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


FilterView::FilterView()
	:
	BGroupView("filter view", B_VERTICAL)
{
	// Construct category popup
	BPopUpMenu* showMenu = new BPopUpMenu(B_TRANSLATE("Category"));
	fShowField = new BMenuField("category", B_TRANSLATE("Category:"), showMenu);

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
FilterView::AdoptModel(Model& model)
{
	// Adopt categories
	BMenu* showMenu = fShowField->Menu();
	showMenu->RemoveItems(0, showMenu->CountItems(), true);

	showMenu->AddItem(new BMenuItem(B_TRANSLATE("All categories"),
		new BMessage(MSG_CATEGORY_SELECTED)));

	AutoLocker<BLocker> locker(model.Lock());
	int32 categoryCount = model.CountCategories();

	if (categoryCount > 0) {
		showMenu->AddItem(new BSeparatorItem());
		_AddCategoriesToMenu(model, showMenu);
	}

	showMenu->SetEnabled(categoryCount > 0);

	if (!_SelectCategoryCode(showMenu, model.Category()))
		showMenu->ItemAt(0)->SetMarked(true);
}


/*! Tries to mark the menu item that corresponds to the supplied
    category code.  If the supplied code was found and the item marked
    then the method will return true.
*/

/*static*/ bool
FilterView::_SelectCategoryCode(BMenu* menu, const BString& code)
{
	for (int32 i = 0; i < menu->CountItems(); i++) {
		BMenuItem* item = menu->ItemAt(i);
		if (_MatchesCategoryCode(item, code)) {
			item->SetMarked(true);
			return true;
		}
	}
	return false;
}


/*static*/ bool
FilterView::_MatchesCategoryCode(BMenuItem* item, const BString& code)
{
	BMessage* message = item->Message();
	if (message == NULL)
		return false;
	BString itemCode;
	message->FindString("code", &itemCode);
	return itemCode == code;
}


/*static*/ void
FilterView::_AddCategoriesToMenu(Model& model, BMenu* menu)
{
	int count = model.CountCategories();
	for (int i = 0; i < count; i++) {
		const CategoryRef& category = model.CategoryAtIndex(i);
		BMessage* message = new BMessage(MSG_CATEGORY_SELECTED);
		message->AddString("code", category->Code());
		BMenuItem* item = new BMenuItem(category->Name(), message);
		menu->AddItem(item);
	}
}
