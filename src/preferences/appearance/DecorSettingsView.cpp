/*
 *  Copyright 2010-2011 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "DecorSettingsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Box.h>
#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TextView.h>

#include "APRWindow.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "DecorSettingsView"


static const int32 kMsgSetDecor = 'deco';
static const int32 kMsgDecorInfo = 'idec';


//	#pragma mark -


DecorSettingsView::DecorSettingsView(const char* name)
	:
	BView(name, 0)
{
	// Decorator menu
	_BuildDecorMenu();
	fDecorMenuField = new BMenuField("decorator",
		B_TRANSLATE("Window Decorator:"), fDecorMenu);

	fDecorInfoButton = new BButton(B_TRANSLATE("About"),
		new BMessage(kMsgDecorInfo));

	SetLayout(new BGroupLayout(B_VERTICAL));

	// control layout
	AddChild(BGridLayoutBuilder(10, 10)
		.Add(fDecorMenuField->CreateLabelLayoutItem(), 0, 0)
        .Add(fDecorMenuField->CreateMenuBarLayoutItem(), 1, 0)
		.Add(fDecorInfoButton, 2, 0)

        .Add(BSpaceLayoutItem::CreateGlue(), 0, 3, 2)
        .SetInsets(10, 10, 10, 10)
    );
	// TODO : Decorator Preview Image?
}


DecorSettingsView::~DecorSettingsView()
{
}


void
DecorSettingsView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fDecorMenu->SetTargetForItems(this);
	fDecorInfoButton->SetTarget(this);
}


void
DecorSettingsView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgSetDecor:
		{
			BString newDecor;
			if (msg->FindString("decor", &newDecor) != B_OK)
				break;

			DecorInfoUtility* decorUtility
				= new(std::nothrow) DecorInfoUtility();

			if (decorUtility == NULL)
				return;

			DecorInfo* decor = decorUtility->FindDecorator(newDecor);
			if (decor == NULL)
				return;

			fSavedDecor = fCurrentDecor;
			fCurrentDecor = (char*)decor->Name().String();

			decorUtility->SetDecorator(decor);

			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case kMsgDecorInfo:
		{
			DecorInfoUtility* decorUtility
				= new(std::nothrow) DecorInfoUtility();

			if (decorUtility == NULL)
				return;

			BString decoratorName(fCurrentDecor);
			DecorInfo* decor = decorUtility->FindDecorator(decoratorName);
			if (decor == NULL)
				return;

			BString authorsText(decor->Authors().String());
			authorsText.ReplaceAll(", ", "\n    ");

			BString infoText("Name: %decorName\n"
				"Authors:\n    %decorAuthors\n"
				"URL: %decorURL\n"
				"License: %decorLic\n"
				"Description:\n    %decorDesc\n");

			infoText.ReplaceFirst("%decorName", decor->Name().String());
			infoText.ReplaceFirst("%decorAuthors", authorsText.String());
			infoText.ReplaceFirst("%decorLic", decor->LicenseName().String());
			infoText.ReplaceFirst("%decorURL", decor->SupportURL().String());
			infoText.ReplaceFirst("%decorDesc", decor->ShortDescription().String());

			BAlert *infoAlert = new BAlert(B_TRANSLATE("About Decorator"),
				infoText.String(), B_TRANSLATE("OK"));
			infoAlert->SetShortcut(0, B_ESCAPE); 
			infoAlert->Go(); 

			Window()->PostMessage(kMsgUpdate);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
DecorSettingsView::_BuildDecorMenu()
{
	fDecorMenu = new BPopUpMenu(B_TRANSLATE("Choose Decorator"));
	DecorInfo* decorator = NULL;

	// collect the current system decor settings
	DecorInfoUtility* decorUtility = new DecorInfoUtility();

	if (decorUtility == NULL) {
		return;
	}

	int32 count = decorUtility->CountDecorators();
	for (int32 i = 0; i < count; ++i) {
		decorator = decorUtility->DecoratorAt(i);
		if (decorator == NULL) {
			fprintf(stderr, "Decorator : error NULL entry @ %li / %li\n",
				i, count);
		}

		BString decorName = decorator->Name();

		if (decorUtility->CurrentDecorator() == decorator)
			fCurrentDecor = (char*)decorName.String();

		BMessage* message = new BMessage(kMsgSetDecor);
		message->AddString("decor", decorator->Name());

		BMenuItem* item
			= new BMenuItem(decorator->Name(), message);

		fDecorMenu->AddItem(item);
	}

	_SetCurrentDecor();
}


void
DecorSettingsView::_SetCurrentDecor()
{
	BMenuItem *item = fDecorMenu->FindItem(fCurrentDecor);
	BString currDecor = fCurrentDecor;
	if (item != NULL)
		item->SetMarked(true);
}


void
DecorSettingsView::SetDefaults()
{
	DecorInfoUtility* decorUtility
		= new(std::nothrow) DecorInfoUtility();

	if (decorUtility == NULL)
		return;
	DecorInfo* defaultDecorator(decorUtility->DefaultDecorator());
	decorUtility->SetDecorator(defaultDecorator);
	_BuildDecorMenu();

	delete decorUtility;
}


bool
DecorSettingsView::IsDefaultable()
{
	return true;
}


bool
DecorSettingsView::IsRevertable()
{
	return fCurrentDecor != fSavedDecor;
}


void
DecorSettingsView::Revert()
{
	if (!IsRevertable())
		return;
}

