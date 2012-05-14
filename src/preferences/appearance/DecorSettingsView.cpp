/*
 *  Copyright 2010-2012 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "DecorSettingsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <TextView.h>

#include "APRWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DecorSettingsView"


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
		B_TRANSLATE("Window decorator:"), fDecorMenu);

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
			if (msg->FindString("decor", &newDecor) == B_OK)
				_SetDecor(newDecor);
			break;
		}
		case kMsgDecorInfo:
		{
			DecorInfo* decor = fDecorUtility.FindDecorator(fCurrentDecor);
			if (decor == NULL)
				break;

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

			break;
		}

		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
DecorSettingsView::_BuildDecorMenu()
{
	fDecorMenu = new BPopUpMenu(B_TRANSLATE("Choose Decorator"));

	// collect the current system decor settings
	int32 count = fDecorUtility.CountDecorators();
	for (int32 i = 0; i < count; ++i) {
		DecorInfo* decorator = fDecorUtility.DecoratorAt(i);
		if (decorator == NULL) {
			fprintf(stderr, "Decorator : error NULL entry @ %li / %li\n",
				i, count);
			continue;
		}

		BString decorName = decorator->Name();

		BMessage* message = new BMessage(kMsgSetDecor);
		message->AddString("decor", decorName);

		BMenuItem* item = new BMenuItem(decorName, message);

		fDecorMenu->AddItem(item);
	}

	_AdoptToCurrentDecor();
}


void
DecorSettingsView::_SetDecor(const BString& name)
{
	_SetDecor(fDecorUtility.FindDecorator(name));
}	


void
DecorSettingsView::_SetDecor(DecorInfo* decorInfo)
{
	if (fDecorUtility.SetDecorator(decorInfo) == B_OK) {
		_AdoptToCurrentDecor();
		Window()->PostMessage(kMsgUpdate);
	}
}	


void
DecorSettingsView::_AdoptToCurrentDecor()
{
	fCurrentDecor = fDecorUtility.CurrentDecorator()->Name();
	if (fSavedDecor.Length() == 0)
		fSavedDecor = fCurrentDecor;
	_AdoptInterfaceToCurrentDecor();
}

void
DecorSettingsView::_AdoptInterfaceToCurrentDecor()
{
	BMenuItem* item = fDecorMenu->FindItem(fCurrentDecor);
	if (item != NULL)
		item->SetMarked(true);
}


void
DecorSettingsView::SetDefaults()
{
	_SetDecor(fDecorUtility.DefaultDecorator());
}


bool
DecorSettingsView::IsDefaultable()
{
	return fCurrentDecor != fDecorUtility.DefaultDecorator()->Name();
}


bool
DecorSettingsView::IsRevertable()
{
	return fCurrentDecor != fSavedDecor;
}


void
DecorSettingsView::Revert()
{
	_SetDecor(fSavedDecor);
}
