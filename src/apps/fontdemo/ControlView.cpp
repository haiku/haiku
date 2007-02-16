/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mikael Konradson, mikael.konradson@gmail.com
 */


#include "ControlView.h"
#include "messages.h"

#include <Button.h>
#include <CheckBox.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Slider.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>

#include <stdio.h>


ControlView::ControlView(BRect rect)
	: BView(rect, "ControlView", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE_JUMP),
	fMessenger(NULL),
	fMessageRunner(NULL),
	fTextControl(NULL),
	fFontMenuField(NULL),
	fFontsizeSlider(NULL),
	fShearSlider(NULL),
	fRotationSlider(NULL),
	fSpacingSlider(NULL),
	fOutlineSlider(NULL),
	fAliasingCheckBox(NULL),
	fBoundingboxesCheckBox(NULL),
	fCyclingFontButton(NULL),
	fFontFamilyMenu(NULL),
	fCycleFonts(false),
	fFontStyleindex(0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


ControlView::~ControlView()
{
	delete fMessenger;
	delete fMessageRunner;
}


void
ControlView::AttachedToWindow()
{
	BRect rect(Bounds());
	rect.InsetBySelf(10, 0);
	rect.bottom = rect.top + 18;	
	rect.OffsetBy(0, 11);

	float offsetX = 0;
	float offsetY = 0;

	fTextControl = new BTextControl(rect, "TextInput", "Text:", "Haiku, inc.", NULL);
	fTextControl->SetDivider(29.0);
	fTextControl->SetModificationMessage(new BMessage(TEXT_CHANGED_MSG));
	AddChild(fTextControl);

	rect.OffsetBy(0.0, 27.0);
	_AddFontMenu(rect);

	rect.OffsetBy(0.0, 29.0);
	fFontsizeSlider = new BSlider(rect, "Fontsize", "Size: 50", NULL, 4, 360);
	fFontsizeSlider->SetModificationMessage(new BMessage(FONTSIZE_MSG));
	fFontsizeSlider->SetValue(50);
	AddChild(fFontsizeSlider);

	// Get the preferred size for the sliders
	fFontsizeSlider->GetPreferredSize(&offsetY, &offsetX);
	offsetX += 1;

	rect.OffsetBy(0.0, offsetX);
	fShearSlider = new BSlider(rect, "Shear", "Shear: 90", NULL, 45, 135);
	fShearSlider->SetModificationMessage(new BMessage(FONTSHEAR_MSG));
	fShearSlider->SetValue(90);
	AddChild(fShearSlider);

	rect.OffsetBy(0.0, offsetX);
	fRotationSlider = new BSlider(rect, "Rotation", "Rotation: 0", NULL, 0, 360);
	fRotationSlider->SetModificationMessage( new BMessage(ROTATION_MSG));
	fRotationSlider->SetValue(0);
	AddChild(fRotationSlider);

	rect.OffsetBy(0.0, offsetX);
	fSpacingSlider = new BSlider(rect, "Spacing", "Spacing: 0", NULL, -5, 50);
	fSpacingSlider->SetModificationMessage(new BMessage(SPACING_MSG));
	fSpacingSlider->SetValue(0);
	AddChild(fSpacingSlider);

	rect.OffsetBy(0.0, offsetX);
	fOutlineSlider = new BSlider(rect, "Outline", "Outline:", NULL, 0, 20);
	fOutlineSlider->SetModificationMessage(new BMessage(OUTLINE_MSG));
	AddChild(fOutlineSlider);

	rect.OffsetBy(0.0, offsetX);
	fAliasingCheckBox = new BCheckBox(rect, "Aliasing", "Anti-aliased text",
		new BMessage(ALIASING_MSG));
	fAliasingCheckBox->SetValue(B_CONTROL_ON);
	AddChild(fAliasingCheckBox);

	rect.OffsetBy(0.0, 22);
	fBoundingboxesCheckBox = new BCheckBox(rect, "BoundingBoxes", "Bounding boxes",
		new BMessage(BOUNDING_BOX_MSG));
	AddChild(fBoundingboxesCheckBox);

	rect.OffsetBy(0.0, 22.0);
	fCyclingFontButton = new BButton(rect, "Cyclefonts", "Cycle Fonts",
		new BMessage(CYCLING_FONTS_MSG));
	AddChild(fCyclingFontButton);

	fTextControl->SetTarget(this);
	fFontsizeSlider->SetTarget(this);
	fShearSlider->SetTarget(this);
	fRotationSlider->SetTarget(this);
	fSpacingSlider->SetTarget(this);
	fOutlineSlider->SetTarget(this);
	fAliasingCheckBox->SetTarget(this);
	fBoundingboxesCheckBox->SetTarget(this);
	fCyclingFontButton->SetTarget(this);
}


void
ControlView::Draw(BRect updateRect)
{
	BRect rect(Bounds());
	SetHighColor(tint_color(ViewColor(), B_LIGHTEN_2_TINT));
	StrokeLine(rect.LeftTop(), rect.RightTop());
	StrokeLine(rect.LeftTop(), rect.LeftBottom());

	SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
	StrokeLine(rect.LeftBottom(), rect.RightBottom());
	StrokeLine(rect.RightBottom(), rect.RightTop());	
}


void
ControlView::MessageReceived(BMessage* msg)
{
	if (!fMessenger) {
		BView::MessageReceived(msg);
		return;
	}

	switch (msg->what) {
		case TEXT_CHANGED_MSG:
		{
			BMessage fontMsg(TEXT_CHANGED_MSG);
			fontMsg.AddString("_text", fTextControl->Text());
			fMessenger->SendMessage(&fontMsg);
			break;
		}

		case FONTSTYLE_CHANGED_MSG:
			_UpdateAndSendStyle(msg);
			break;

		case FONTFAMILY_CHANGED_MSG:
			_UpdateAndSendFamily(msg);
			break;

		case FONTSIZE_MSG:
		{
			char buff[256];
			sprintf(buff, "Size: %d", static_cast<int>(fFontsizeSlider->Value()));
			fFontsizeSlider->SetLabel(buff);

			BMessage msg(FONTSIZE_MSG);
			msg.AddFloat("_size", static_cast<float>(fFontsizeSlider->Value()));
			fMessenger->SendMessage(&msg);
			break;
		}

		case FONTSHEAR_MSG:
		{
			char buff[256];
			sprintf(buff, "Shear: %d", static_cast<int>(fShearSlider->Value()));
			fShearSlider->SetLabel(buff);

			BMessage msg(FONTSHEAR_MSG);
			msg.AddFloat("_shear", static_cast<float>(fShearSlider->Value()));
			fMessenger->SendMessage(&msg);
			break;
		}

		case ROTATION_MSG:
		{
			char buff[256];
			sprintf(buff, "Rotation: %d", static_cast<int>(fRotationSlider->Value()));
			fRotationSlider->SetLabel(buff);

			BMessage msg(ROTATION_MSG);
			msg.AddFloat("_rotation", static_cast<float>(fRotationSlider->Value()));
			fMessenger->SendMessage(&msg);
			break;
		}

		case SPACING_MSG:
		{
			char buff[256];
			sprintf(buff, "Spacing: %d", (int)fSpacingSlider->Value());
			fSpacingSlider->SetLabel(buff);

			BMessage msg(SPACING_MSG);
			msg.AddFloat("_spacing", static_cast<float>(fSpacingSlider->Value()));
			fMessenger->SendMessage(&msg);
			break;
		}
		
		case ALIASING_MSG:
		{
			BMessage msg(ALIASING_MSG);
			msg.AddBool("_aliased", static_cast<bool>(fAliasingCheckBox->Value()));
			fMessenger->SendMessage(&msg);
			break;
		}

		case BOUNDING_BOX_MSG:
		{
			BMessage msg(BOUNDING_BOX_MSG);
			msg.AddBool("_boundingbox", static_cast<bool>(fBoundingboxesCheckBox->Value()));
			fMessenger->SendMessage(&msg);
			break;
		}

		case OUTLINE_MSG:
		{
			int8 outlineVal = (int8)fOutlineSlider->Value();

			char buff[256];
			sprintf(buff, "Outline: %d", outlineVal);
			fOutlineSlider->SetLabel(buff);

			fAliasingCheckBox->SetEnabled(outlineVal < 1);
			fBoundingboxesCheckBox->SetEnabled(outlineVal < 1);

			BMessage msg(OUTLINE_MSG);
			msg.AddInt8("_outline", outlineVal);
			fMessenger->SendMessage(&msg);
			break;
		}

		case CYCLING_FONTS_MSG:
		{
			fCyclingFontButton->SetLabel(fCycleFonts ? "Cycle Fonts" : "Stop Cycling");
			fCycleFonts = !fCycleFonts;

			if (fCycleFonts) {
				delete fMessageRunner;
				fMessageRunner = new BMessageRunner(this,
					new BMessage(CYCLING_FONTS_UPDATE_MSG), 360000*2, -1);
			} else {
				delete fMessageRunner;
				fMessageRunner = NULL;
				fFontStyleindex	= 0;
				// Delete our MessageRunner and reset the style index
			}
			break;
		}

		case CYCLING_FONTS_UPDATE_MSG:
		{			
			int32 familyindex = -1;
			BMenuItem* currentFamilyItem = fFontFamilyMenu->FindMarked();

			if (currentFamilyItem) {
				familyindex = fFontFamilyMenu->IndexOf(currentFamilyItem);
		 		const int32 installedStyles = count_font_styles(
		 			const_cast<char*>(currentFamilyItem->Label()));

		 		BMenu* submenu = currentFamilyItem->Submenu();
		 		if (submenu) {
		 			BMenuItem* markedStyle = submenu->FindMarked();
		 			fFontStyleindex = submenu->IndexOf(markedStyle);
				}

		 		if (fFontStyleindex < installedStyles - 1)
		 			fFontStyleindex++;
		 		else {
		 			fFontStyleindex = 0;

		 			if (familyindex < count_font_families())
				 		familyindex++;
				 	else
			 			familyindex = 0;
				}

				BMenuItem* newFontFamilyItem = fFontFamilyMenu->ItemAt(familyindex);
		 		BMenuItem* newstyleitem = submenu->ItemAt(fFontStyleindex);

		 		if (newFontFamilyItem && newstyleitem) {
		 			if (msg->AddString("_style", newstyleitem->Label()) != B_OK
		 				|| msg->AddString("_family", newFontFamilyItem->Label()) != B_OK) {
		 				printf("Failed to add _style or family to the message\n");
		 				return;
		 			}
		 			printf("InstalledStyles(%ld), Font(%s), Style(%s)\n",
		 				installedStyles, newFontFamilyItem->Label(),
		 				newstyleitem->Label());
					_UpdateAndSendStyle(msg);
				}
			}
			break;
		}

		default:
			BView::MessageReceived(msg);
	}
}


void
ControlView::SetTarget(BHandler* handler)
{
	delete fMessenger;
	fMessenger = new BMessenger(handler); 	
}


void
ControlView::_UpdateFontmenus(bool setInitialfont)
{
	BFont font;
	BMenu* stylemenu = NULL;

	font_family fontFamilyName, currentFamily;
	font_style fontStyleName, currentStyle;

	GetFont(&font);
	font.GetFamilyAndStyle(&currentFamily, &currentStyle);

	const int32 fontfamilies = count_font_families();

	fFontFamilyMenu->RemoveItems(0, fFontFamilyMenu->CountItems(), true);

	for (int32 i = 0; i < fontfamilies; i++) {
		if (get_font_family(i, &fontFamilyName) == B_OK) {
			stylemenu = new BMenu(fontFamilyName);
			const int32 styles = count_font_styles(fontFamilyName);

			BMessage* familyMsg = new BMessage(FONTFAMILY_CHANGED_MSG);
			familyMsg->AddString("_family", fontFamilyName);
			BMenuItem* familyItem = new BMenuItem(stylemenu, familyMsg);
			fFontFamilyMenu->AddItem(familyItem);
			 
			for (int32 j = 0; j < styles; j++) {
				if (get_font_style(fontFamilyName, j, &fontStyleName) == B_OK) {
					BMessage* fontMsg = new BMessage(FONTSTYLE_CHANGED_MSG);
					fontMsg->AddString("_family", fontFamilyName);
					fontMsg->AddString("_style", fontStyleName);

					BMenuItem* styleItem = new BMenuItem(fontStyleName, fontMsg);
					styleItem->SetMarked(false);

					// setInitialfont is used when we attach the FontField
					if (!strcmp(fontStyleName, currentStyle)
						&& !strcmp(fontFamilyName, currentFamily)
						&& setInitialfont) {
						styleItem->SetMarked(true);
						familyItem->SetMarked(true);

						BString string;
						string << currentFamily << " " << currentStyle;

						if (fFontMenuField)
							fFontMenuField->MenuItem()->SetLabel(string.String());
					}
					stylemenu->AddItem(styleItem);
				}
			}
		}
		stylemenu->SetRadioMode(true);
		stylemenu->SetTargetForItems(this);
	}

	fFontFamilyMenu->SetLabelFromMarked(true);
	fFontFamilyMenu->SetTargetForItems(this);
}


void
ControlView::_AddFontMenu(BRect rect)
{
	fFontFamilyMenu = new BMenu("fontfamlilymenu");

	_UpdateFontmenus(true);

	fFontMenuField = new BMenuField(rect, "FontMenuField", "Font:", fFontFamilyMenu, true);
	fFontMenuField->SetDivider(30.0);
	AddChild(fFontMenuField);
}


void
ControlView::_UpdateAndSendFamily(const BMessage* message)
{
	_DeselectOldItems();

	font_family family;
	font_style style;

	if (message->FindString("_family", (const char **)&family) == B_OK) {
		printf("Family:%s\n\n", family);

		BMenuItem* markedItem = fFontFamilyMenu->FindItem(family);
		if (!markedItem)
			return;

		markedItem->SetMarked(true);

		get_font_style(family, 0, &style);

		BString string;
		string << family << " " << style;

		if (fFontMenuField)
			fFontMenuField->MenuItem()->SetLabel(string.String());

		BMenu* submenu = markedItem->Submenu();

		if (submenu) {
			BMenuItem* styleItem = submenu->FindItem(style);
			if (styleItem && !styleItem->IsMarked())
				styleItem->SetMarked(true);
		}

		BMessage fontMsg(FONTFAMILY_CHANGED_MSG);
		if (fontMsg.AddMessage("_fontMessage", message) == B_OK)
			fMessenger->SendMessage(&fontMsg);
	}
}


void
ControlView::_UpdateAndSendStyle(const BMessage* message)
{
	_DeselectOldItems();

	const char* style;
	const char* family;
	if (message->FindString("_style", &style) == B_OK
		&& message->FindString("_family", &family) == B_OK) {
		BMenuItem* familyItem = fFontFamilyMenu->FindItem(family);

		if (familyItem && !familyItem->IsMarked()) {
			familyItem->SetMarked(true);

			BMenu* submenu = familyItem->Submenu();
			if (submenu) {
				BMenuItem* styleItem = submenu->FindItem(style);
				if (styleItem && !styleItem->IsMarked())
					styleItem->SetMarked(true);
			}
		}

		BString string;
		string << family << " " << style;

		if (fFontMenuField)
			fFontMenuField->MenuItem()->SetLabel(string.String());
	}

	BMessage fontMsg(FONTSTYLE_CHANGED_MSG);
	if (fontMsg.AddMessage("_fontMessage", message) == B_OK)
		fMessenger->SendMessage(&fontMsg);
}


void
ControlView::_DeselectOldItems()
{
	BMenuItem* oldItem = fFontFamilyMenu->FindMarked();
	if (oldItem) {
		oldItem->SetMarked(false);

		BMenu* submenu = oldItem->Submenu();
		if (submenu) {
			BMenuItem* marked = submenu->FindMarked();
			if (marked)
				marked->SetMarked(false);			
		}
	}
}

