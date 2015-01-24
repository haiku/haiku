/*
 *  Copyright 2010-2015 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		John Scipione, jscipione@gmail.com
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
#include <RadioButton.h>
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


typedef enum {
	KNOB_NONE = 0,
	KNOB_DOTS,
	KNOB_LINES
} knob_style;


static const int32 kMsgSetDecor = 'deco';
static const int32 kMsgDecorInfo = 'idec';

static const int32 kMsgDoubleScrollBarArrows = 'dsba';

static const int32 kMsgArrowStyleSingle = 'mass';
static const int32 kMsgArrowStyleDouble = 'masd';

static const int32 kMsgKnobStyleNone = 'mksn';
static const int32 kMsgKnobStyleDots = 'mksd';
static const int32 kMsgKnobStyleLines = 'mksl';

static const bool kDefaultDoubleScrollBarArrowsSetting = false;


//	#pragma mark - LookAndFeelSettingsView


LookAndFeelSettingsView::LookAndFeelSettingsView(const char* name)
	:
	BView(name, 0),
	fDecorInfoButton(NULL),
	fDecorMenuField(NULL),
	fDecorMenu(NULL),
	fSavedDecor(NULL),
	fCurrentDecor(NULL),
	fArrowStyleSingle(NULL),
	fArrowStyleDouble(NULL),
	fKnobStyleNone(NULL),
	fKnobStyleDots(NULL),
	fKnobStyleLines(NULL),
	fSavedKnobStyleValue(_ScrollBarKnobStyle()),
	fSavedDoubleArrowsValue(_DoubleScrollBarArrows())
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

	fArrowStyleSingle = new FakeScrollBar(true, false, _ScrollBarKnobStyle(),
		new BMessage(kMsgArrowStyleSingle));
	fArrowStyleDouble = new FakeScrollBar(true, true, _ScrollBarKnobStyle(),
		new BMessage(kMsgArrowStyleDouble));

	BView* arrowStyleView = BLayoutBuilder::Group<>(B_VERTICAL, 1)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(new BStringView("single", B_TRANSLATE("Single:")))
		.Add(fArrowStyleSingle)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(new BStringView("double", B_TRANSLATE("Double:")))
		.Add(fArrowStyleDouble)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.AddStrut(B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.View();
	arrowStyleBox->AddChild(arrowStyleView);
	arrowStyleBox->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));
	arrowStyleBox->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	// scrollbar knob style
	BBox* knobStyleBox = new BBox("knob style");
	knobStyleBox->SetLabel(B_TRANSLATE("Knob style"));

	fKnobStyleNone = new FakeScrollBar(false, false, KNOB_NONE,
		new BMessage(kMsgKnobStyleNone));
	fKnobStyleDots = new FakeScrollBar(false, false, KNOB_DOTS,
		new BMessage(kMsgKnobStyleDots));
	fKnobStyleLines = new FakeScrollBar(false, false, KNOB_LINES,
		new BMessage(kMsgKnobStyleLines));

	BView* knobStyleView;
	knobStyleView = BLayoutBuilder::Group<>(B_VERTICAL, 0)
		.Add(new BStringView("none", B_TRANSLATE("None:")))
		.Add(fKnobStyleNone)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(new BStringView("none", B_TRANSLATE("Dots:")))
		.Add(fKnobStyleDots)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(new BStringView("none", B_TRANSLATE("Lines:")))
		.Add(fKnobStyleLines)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.View();
	knobStyleBox->AddChild(knobStyleView);

	// control layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fDecorMenuField->CreateLabelLayoutItem(), 0, 0)
			.Add(fDecorMenuField->CreateMenuBarLayoutItem(), 1, 0)
			.Add(fDecorInfoButton, 2, 0)
		.End()
		.Add(new BStringView("scroll bar", B_TRANSLATE("Scroll bar:")))
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL)
				.Add(arrowStyleBox)
				.AddGlue()
				.End()
			.AddGroup(B_VERTICAL)
				.Add(knobStyleBox)
				.AddGlue()
				.End()
			.End()
		.AddGlue()
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
	fKnobStyleNone->SetTarget(this);
	fKnobStyleDots->SetTarget(this);
	fKnobStyleLines->SetTarget(this);

	if (fSavedDoubleArrowsValue)
		fArrowStyleDouble->SetValue(B_CONTROL_ON);
	else
		fArrowStyleSingle->SetValue(B_CONTROL_ON);

	switch (fSavedKnobStyleValue) {
		case KNOB_DOTS:
			fKnobStyleDots->SetValue(B_CONTROL_ON);
			break;

		case KNOB_LINES:
			fKnobStyleLines->SetValue(B_CONTROL_ON);
			break;

		default:
		case KNOB_NONE:
			fKnobStyleNone->SetValue(B_CONTROL_ON);
			break;
	}
}


void
LookAndFeelSettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSetDecor:
		{
			BString newDecor;
			if (message->FindString("decor", &newDecor) == B_OK)
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
			infoText.ReplaceFirst("%decorDesc",
				decor->ShortDescription().String());

			BAlert* infoAlert = new BAlert(B_TRANSLATE("About decorator"),
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

		case kMsgKnobStyleNone:
			_SetScrollBarKnobStyle(KNOB_NONE);
			break;

		case kMsgKnobStyleDots:
			_SetScrollBarKnobStyle(KNOB_DOTS);
			break;

		case kMsgKnobStyleLines:
			_SetScrollBarKnobStyle(KNOB_LINES);
			break;

		default:
			BView::MessageReceived(message);
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

	if (info.double_arrows == doubleArrows)
		return;

	info.double_arrows = doubleArrows;
	set_scroll_bar_info(&info);

	if (doubleArrows)
		fArrowStyleDouble->SetValue(B_CONTROL_ON);
	else
		fArrowStyleSingle->SetValue(B_CONTROL_ON);

	Window()->PostMessage(kMsgUpdate);
}


int32
LookAndFeelSettingsView::_ScrollBarKnobStyle()
{
	scroll_bar_info info;
	get_scroll_bar_info(&info);

	return info.knob;
}


void
LookAndFeelSettingsView::_SetScrollBarKnobStyle(int32 knobStyle)
{
	scroll_bar_info info;
	get_scroll_bar_info(&info);

	if (info.knob == knobStyle)
		return;

	info.knob = knobStyle;
	set_scroll_bar_info(&info);

	switch (knobStyle) {
		case KNOB_DOTS:
			fKnobStyleDots->SetValue(B_CONTROL_ON);
			break;

		case KNOB_LINES:
			fKnobStyleLines->SetValue(B_CONTROL_ON);
			break;

		default:
		case KNOB_NONE:
			fKnobStyleNone->SetValue(B_CONTROL_ON);
			break;
	}

	fArrowStyleSingle->SetKnobStyle(knobStyle);
	fArrowStyleDouble->SetKnobStyle(knobStyle);

	Window()->PostMessage(kMsgUpdate);
}


bool
LookAndFeelSettingsView::IsDefaultable()
{
	return fCurrentDecor != fDecorUtility.DefaultDecorator()->Name()
		|| _DoubleScrollBarArrows() != false
		|| _ScrollBarKnobStyle() != KNOB_NONE;
}


void
LookAndFeelSettingsView::SetDefaults()
{
	_SetDecor(fDecorUtility.DefaultDecorator());
	_SetDoubleScrollBarArrows(false);
	_SetScrollBarKnobStyle(KNOB_NONE);
}


bool
LookAndFeelSettingsView::IsRevertable()
{
	return fCurrentDecor != fSavedDecor
		|| _DoubleScrollBarArrows() != fSavedDoubleArrowsValue
		|| _ScrollBarKnobStyle() != fSavedKnobStyleValue;
}


void
LookAndFeelSettingsView::Revert()
{
	if (IsRevertable()) {
		_SetDecor(fSavedDecor);
		_SetDoubleScrollBarArrows(fSavedDoubleArrowsValue);
		_SetScrollBarKnobStyle(fSavedKnobStyleValue);
	}
}
