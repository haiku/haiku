/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler, <axeld@pinc-software.de>
 *		Michael Pfeiffer, laplace@users.sourceforge.net
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */


#include "PartitionsPage.h"

#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>

#include <StringForSize.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PartitionsPage"


const uint32 kMessageShow = 'show';
const uint32 kMessageName = 'name';


PartitionsPage::PartitionsPage(BMessage* settings, const char* name)
	:
	WizardPageView(settings, name)
{
	_BuildUI();
}


PartitionsPage::~PartitionsPage()
{
}


void
PartitionsPage::PageCompleted()
{
	BGridLayout* layout = (BGridLayout*)fPartitions->GetLayout();
	int32 index = 0;

	for (int32 row = 0; row < layout->CountRows(); row += 3, index++) {
		BCheckBox* showBox
			= dynamic_cast<BCheckBox*>(layout->ItemAt(0, row)->View());
		BTextControl* nameControl
			= dynamic_cast<BTextControl*>(layout->ItemAt(1, row)->View());
		if (nameControl == NULL || showBox == NULL)
			debugger("partitions page is broken");

		BMessage partition;
		if (fSettings->FindMessage("partition", index, &partition) != B_OK)
			continue;

		partition.ReplaceBool("show", showBox->Value() != 0);
		partition.ReplaceString("name", nameControl->Text());

		fSettings->ReplaceMessage("partition", index, &partition);
	}
}


void
PartitionsPage::_BuildUI()
{
	BString text;
	text << B_TRANSLATE_COMMENT("Partitions", "Title") << "\n"
		<< B_TRANSLATE("The following partitions were detected. Please "
			"check the box next to the partitions to be included "
			"in the boot menu. You can also set the names of the "
			"partitions as you would like them to appear in the "
			"boot menu.");
	fDescription = CreateDescription("description", text);
	MakeHeading(fDescription);

	fPartitions = new BGridView("partitions", 0,
		be_control_look->DefaultItemSpacing() / 3);

	BLayoutBuilder::Grid<>(fPartitions)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.SetColumnWeight(0, 0)
		.SetColumnWeight(1, 1)
		.SetColumnWeight(2, 0.5)
		.SetColumnWeight(3, 0.5);
	_FillPartitionsView(fPartitions);

	BScrollView* scrollView = new BScrollView("scrollView", fPartitions, 0,
		false, true);

	SetLayout(new BGroupLayout(B_VERTICAL));

	BLayoutBuilder::Group<>((BGroupLayout*)GetLayout())
		.Add(fDescription)
		.Add(scrollView);
}


void
PartitionsPage::_FillPartitionsView(BView* view)
{
	// show | name | type | size | path

	int32 rowNumber = 0;

	BMessage message;
	for (int32 i = 0; fSettings->FindMessage("partition", i, &message) == B_OK;
			i++, rowNumber++) {
		// get partition data
		bool show;
		BString name;
		BString type;
		BString path;
		int64 size;
		message.FindBool("show", &show);
		message.FindString("name", &name);
		message.FindString("type", &type);
		message.FindString("path", &path);
		message.FindInt64("size", &size);

		// check box
		BCheckBox* checkBox = new BCheckBox("show", "",
			_CreateControlMessage(kMessageShow, i));
		if (show)
			checkBox->SetValue(1);

		// name
		BTextControl* nameControl = new BTextControl("name", "",
			name.String(), _CreateControlMessage(kMessageName, i));
		nameControl->SetExplicitMinSize(BSize(StringWidth("WWWWWWWWWWWWWW"),
			B_SIZE_UNSET));

		// size
		BString sizeText;
		_CreateSizeText(size, &sizeText);
		sizeText << ", " << type.String();
		BStringView* typeView = new BStringView("type", sizeText.String());
		typeView->SetExplicitAlignment(
			BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));

		// path
		BStringView* pathView = new BStringView("path", path.String());
		pathView->SetExplicitAlignment(
			BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));

		if (rowNumber > 0) {
			BLayoutBuilder::Grid<>((BGridLayout*)view->GetLayout())
				.Add(new BSeparatorView(B_HORIZONTAL), 0, rowNumber, 4, 1)
				.SetRowWeight(rowNumber, 0);
			rowNumber++;
		}

		BLayoutBuilder::Grid<>((BGridLayout*)view->GetLayout())
			.Add(checkBox, 0, rowNumber, 1, 2)
			.Add(nameControl, 1, rowNumber, 1, 2)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(10), 2, rowNumber)
			.Add(typeView, 3, rowNumber)
			.Add(pathView, 3, rowNumber + 1)
			.SetRowWeight(rowNumber + 1, 1);
		rowNumber++;
	}
}


void
PartitionsPage::_CreateSizeText(int64 size, BString* text)
{
	char buffer[256];
	*text = string_for_size(size, buffer, sizeof(buffer));
}


BMessage*
PartitionsPage::_CreateControlMessage(uint32 what, int32 index)
{
	BMessage* message = new BMessage(what);
	message->AddInt32("index", index);
	return message;
}
