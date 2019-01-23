/*
 *  Copyright 2010-2012 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Alexander von Gluck <kallisti5@unixzen.com>
 *		John Scipione <jscipione@gmail.com>
 *		Ryan Leavengood <leavengood@gmail.com>
 */


#include "LookAndFeelSettingsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Alignment.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <StringView.h>
#include <Size.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <TextView.h>

#include "APRWindow.h"
#include "FakeScrollBar.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DecorSettingsView"
	// This was not renamed to keep from breaking translations


static const int32 kMsgSetDecor = 'deco';
static const int32 kMsgDecorInfo = 'idec';

static const int32 kMsgDoubleScrollBarArrows = 'dsba';

static const int32 kMsgArrowStyleSingle = 'mass';
static const int32 kMsgArrowStyleDouble = 'masd';

static const int32 kMsgKnobStyleNone = 'mksn';
static const int32 kMsgKnobStyleDots = 'mksd';
static const int32 kMsgKnobStyleLines = 'mksl';

static const bool kDefaultDoubleScrollBarArrowsSetting = false;


//	#pragma mark -


LookAndFeelSettingsView::LookAndFeelSettingsView(const char* name)
	:
	BView(name, 0),
	fDecorInfoButton(NULL),
	fDecorMenuField(NULL),
	fDecorMenu(NULL)
{
	// Decorator menu
	_BuildDecorMenu();
	fDecorMenuField = new BMenuField("decorator",
		B_TRANSLATE("Decorator:"), fDecorMenu);

	fDecorInfoButton = new BButton(B_TRANSLATE("About"),
		new BMessage(kMsgDecorInfo));

	// scroll bar arrow style
	BBox* arrowStyleBox = new BBox("arrow style");
	arrowStyleBox->SetLabel(B_TRANSLATE("Arrow style"));

	fSavedDoubleArrowsValue = _DoubleScrollBarArrows();

	fArrowStyleSingle = new FakeScrollBar(true, false,
		new BMessage(kMsgArrowStyleSingle));
	fArrowStyleDouble = new FakeScrollBar(true, true,
		new BMessage(kMsgArrowStyleDouble));

	BView* arrowStyleView;
	arrowStyleView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 1)
			.Add(new BStringView("single", B_TRANSLATE("Single:")))
			.Add(fArrowStyleSingle)
			.AddStrut(B_USE_DEFAULT_SPACING)
			.Add(new BStringView("double", B_TRANSLATE("Double:")))
			.Add(fArrowStyleDouble)
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.End()
		.View();
	arrowStyleBox->AddChild(arrowStyleView);
	arrowStyleBox->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));
	arrowStyleBox->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BStringView* scrollBarLabel
		= new BStringView("scroll bar", B_TRANSLATE("Scroll bar:"));
	scrollBarLabel->SetExplicitAlignment(
		BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));

	// control layout
	BLayoutBuilder::Grid<>(this, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fDecorMenuField->CreateLabelLayoutItem(), 0, 0)
		.Add(fDecorMenuField->CreateMenuBarLayoutItem(), 1, 0)
		.Add(fDecorInfoButton, 2, 0)
		.Add(scrollBarLabel, 0, 1)
		.Add(arrowStyleBox, 1, 1)
		.AddGlue(0, 2)
		.SetInsets(B_USE_WINDOW_SPACING);

	// TODO : Decorator Preview Image?
}


LookAndFeelSettingsView::~LookAndFeelSettingsView()
{
}


void
LookAndFeelSettingsView::AttachedToWindow()
{
	AdoptParentColors();

	if (Parent() == NULL)
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fDecorMenu->SetTargetForItems(this);
	fDecorInfoButton->SetTarget(this);
	fArrowStyleSingle->SetTarget(this);
	fArrowStyleDouble->SetTarget(this);

	if (fSavedDoubleArrowsValue)
		fArrowStyleDouble->SetValue(B_CONTROL_ON);
	else
		fArrowStyleSingle->SetValue(B_CONTROL_ON);
}


void
LookAndFeelSettingsView::MessageReceived(BMessage *msg)
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
			authorsText.ReplaceAll(", ", "\n\t");

			BString infoText(B_TRANSLATE("%decorName\n\n"
				"Authors:\n\t%decorAuthors\n\n"
				"URL: %decorURL\n"
				"License: %decorLic\n\n"
				"%decorDesc\n"));


			infoText.ReplaceFirst("%decorName", decor->Name().String());
			infoText.ReplaceFirst("%decorAuthors", authorsText.String());
			infoText.ReplaceFirst("%decorLic", decor->LicenseName().String());
			infoText.ReplaceFirst("%decorURL", decor->SupportURL().String());
			infoText.ReplaceFirst("%decorDesc", decor->ShortDescription().String());

			BAlert *infoAlert = new BAlert(B_TRANSLATE("About decorator"),
				infoText.String(), B_TRANSLATE("OK"));
			infoAlert->SetFlags(infoAlert->Flags() | B_CLOSE_ON_ESCAPE);
			infoAlert->Go();

			break;
		}

		case kMsgArrowStyleSingle:
			_SetDoubleScrollBarArrows(false);
			break;

		case kMsgArrowStyleDouble:
			_SetDoubleScrollBarArrows(true);
			break;

		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
LookAndFeelSettingsView::_BuildDecorMenu()
{
	fDecorMenu = new BPopUpMenu(B_TRANSLATE("Choose Decorator"));

	// collect the current system decor settings
	int32 count = fDecorUtility.CountDecorators();
	for (int32 i = 0; i < count; ++i) {
		DecorInfo* decorator = fDecorUtility.DecoratorAt(i);
		if (decorator == NULL) {
			fprintf(stderr, "Decorator : error NULL entry @ %" B_PRId32
				" / %" B_PRId32 "\n", i, count);
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
LookAndFeelSettingsView::_SetDecor(const BString& name)
{
	_SetDecor(fDecorUtility.FindDecorator(name));
}


void
LookAndFeelSettingsView::_SetDecor(DecorInfo* decorInfo)
{
	if (fDecorUtility.SetDecorator(decorInfo) == B_OK) {
		_AdoptToCurrentDecor();
		Window()->PostMessage(kMsgUpdate);
	}
}


void
LookAndFeelSettingsView::_AdoptToCurrentDecor()
{
	fCurrentDecor = fDecorUtility.CurrentDecorator()->Name();
	if (fSavedDecor.Length() == 0)
		fSavedDecor = fCurrentDecor;
	_AdoptInterfaceToCurrentDecor();
}

void
LookAndFeelSettingsView::_AdoptInterfaceToCurrentDecor()
{
	BMenuItem* item = fDecorMenu->FindItem(fCurrentDecor);
	if (item != NULL)
		item->SetMarked(true);
}


bool
LookAndFeelSettingsView::_DoubleScrollBarArrows()
{
	scroll_bar_info info;
	get_scroll_bar_info(&info);

	return info.double_arrows;
}


void
LookAndFeelSettingsView::_SetDoubleScrollBarArrows(bool doubleArrows)
{
	scroll_bar_info info;
	get_scroll_bar_info(&info);

	info.double_arrows = doubleArrows;
	set_scroll_bar_info(&info);

	if (doubleArrows)
		fArrowStyleDouble->SetValue(B_CONTROL_ON);
	else
		fArrowStyleSingle->SetValue(B_CONTROL_ON);

	Window()->PostMessage(kMsgUpdate);
}


bool
LookAndFeelSettingsView::IsDefaultable()
{
	return fCurrentDecor != fDecorUtility.DefaultDecorator()->Name()
		|| _DoubleScrollBarArrows() != false;
}


void
LookAndFeelSettingsView::SetDefaults()
{
	_SetDecor(fDecorUtility.DefaultDecorator());
	_SetDoubleScrollBarArrows(false);
}


bool
LookAndFeelSettingsView::IsRevertable()
{
	return fCurrentDecor != fSavedDecor
		|| _DoubleScrollBarArrows() != fSavedDoubleArrowsValue;
}


void
LookAndFeelSettingsView::Revert()
{
	if (IsRevertable()) {
		_SetDecor(fSavedDecor);
		_SetDoubleScrollBarArrows(fSavedDoubleArrowsValue);
	}
}
