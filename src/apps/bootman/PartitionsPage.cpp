/*
 * Copyright 2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 *		Michael Pfeiffer, laplace@users.sourceforge.net
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */


#include "PartitionsPage.h"


#include <CheckBox.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>

#include <stdio.h>
#include <string.h>
#include <String.h>


const uint32 kMessageShow = 'show';
const uint32 kMessageName = 'name';


PartitionsPage::PartitionsPage(BMessage* settings, BRect frame, const char* name)
	: WizardPageView(settings, frame, name, B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	_BuildUI();
}


PartitionsPage::~PartitionsPage()
{
}


void
PartitionsPage::PageCompleted()
{
	int32 i;
	const int32 n = fPartitions->CountChildren();
	for (i = 0; i < n; i ++) {
		BView* row = fPartitions->ChildAt(i);
		int32 j;
		const int32 m = row->CountChildren();
		for (j = 0; j < m; j ++) {
			BView* child = row->ChildAt(j);
			BControl* control = dynamic_cast<BControl*>(child);
			if (control == NULL)
				continue;
			
			int32 index;
			BMessage* message = control->Message();
			if (message == NULL || message->FindInt32("index", &index) != B_OK)
				continue;
			
			BMessage partition;
			if (fSettings->FindMessage("partition", index, &partition) != B_OK)
				continue;
					
			if (kMessageShow == message->what) {
				partition.ReplaceBool("show", control->Value() == 1);
			} else if (kMessageName == message->what &&
					dynamic_cast<BTextControl*>(control) != NULL) {
				partition.ReplaceString("name", ((BTextControl*)control)->Text());
			}
			
			fSettings->ReplaceMessage("partition", index, &partition);
		}
	}
}


void
PartitionsPage::FrameResized(float width, float height)
{
	WizardPageView::FrameResized(width, height);
	_Layout();
}


static const float kTextDistance = 10;

void
PartitionsPage::_BuildUI()
{
	BRect rect(Bounds());
	
	fDescription = CreateDescription(rect, "description", 
		"Partitions\n\n"
		"The following partitions were detected. Please "
		"check the box next to the partitions to be included "
		"in the boot menu. You can also set the names of the "
		"partitions as you would like them to appear in the "
		"boot menu."
		);
	MakeHeading(fDescription);
	AddChild(fDescription);
	LayoutDescriptionVertically(fDescription);
	
	rect.left = fDescription->Frame().left + 1;
	rect.top = fDescription->Frame().bottom + kTextDistance;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom -= B_H_SCROLL_BAR_HEIGHT + 3;
		
	fPartitions = new BView(rect, "partitions", B_FOLLOW_ALL,
		B_WILL_DRAW);
	
	fPartitionsScrollView = new BScrollView("scrollView", fPartitions, 
		B_FOLLOW_ALL,
		0,
		true, true);
	fPartitionsScrollView->SetViewColor(ViewColor());
	AddChild(fPartitionsScrollView);
	
	_FillPartitionsView(fPartitions);
	
	_Layout();
}


void
PartitionsPage::_Layout()
{
	LayoutDescriptionVertically(fDescription);

	float left = fPartitionsScrollView->Frame().left;
	float top = fDescription->Frame().bottom + kTextDistance;
	fPartitionsScrollView->MoveTo(left, top);
	
	float width = fPartitionsScrollView->Frame().Width();
	float height = Bounds().bottom - top;
	fPartitionsScrollView->ResizeTo(width, height);
	
	BScrollBar* scrollbar = fPartitionsScrollView->ScrollBar(B_HORIZONTAL);
	float max = fPartitionsWidth - fPartitions->Bounds().IntegerWidth();
	if (max < 0)
		max = 0;
	scrollbar->SetRange(0, max);
	
	scrollbar = fPartitionsScrollView->ScrollBar(B_VERTICAL);
	max = fPartitionsHeight - fPartitions->Bounds().IntegerHeight();
	if (max < 0)
		max = 0;
	scrollbar->SetRange(0, max);
}


const rgb_color kOddRowColor = {224, 255, 224};
const rgb_color kEvenRowColor = {224, 224, 255};


void
PartitionsPage::_FillPartitionsView(BView* view)
{
	const int32 inset = 1;
	

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	
	const int32 height = (int32)(6 + 2*inset + fontHeight.ascent + fontHeight.descent);
	const int32 kDistance = (int32)(ceil(be_plain_font->StringWidth("x")));
	
	// show | name | type | size | path  
	int32 showWidth = 0;
	int32 nameWidth = 0;
	int32 typeWidth = 0;
	int32 sizeWidth = 0;
	int32 pathWidth = 0;
	_ComputeColumnWidths(showWidth, nameWidth, typeWidth, sizeWidth, pathWidth);
	
	int32 totalWidth = showWidth + nameWidth + typeWidth + sizeWidth + pathWidth + 
		2*inset + 4 * kDistance;
	
	int32 rowNumber = 0;
	
	BRect frame(view->Bounds());
	frame.bottom = frame.top + height;
	frame.right = frame.left + 65000;

	BMessage message;
	for (int32 i = 0; fSettings->FindMessage("partition", i, &message) == B_OK; i ++, rowNumber ++) {
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
		
		// create row for partition data
		BView* row = new BView(frame, "row", B_FOLLOW_TOP | B_FOLLOW_LEFT, 0);
		row->SetViewColor(((rowNumber % 2) == 0) ? kEvenRowColor : kOddRowColor);
		view->AddChild(row);		
		frame.OffsetBy(0, height);

		// box
		BRect rect(row->Bounds());
		rect.InsetBy(inset, inset);
		rect.right = rect.left + showWidth;
		BCheckBox* checkBox = new BCheckBox(rect, "show", "", 
			_CreateControlMessage(kMessageShow, i));
		if (show)
			checkBox->SetValue(1);
		row->AddChild(checkBox);
		rect.OffsetBy(showWidth + kDistance, 0);		
		
		// name
		rect.right = rect.left + nameWidth;
		BTextControl* nameControl = new BTextControl(rect, "name", "", name.String(), 
			_CreateControlMessage(kMessageName, i));
		nameControl->SetDivider(0);
		row->AddChild(nameControl);
		rect.OffsetBy(nameWidth + kDistance, 0);
		
		// type
		rect.right = rect.left + typeWidth;
		BStringView* typeView = new BStringView(rect, "type", type.String());
		row->AddChild(typeView);
		rect.OffsetBy(typeWidth + kDistance, 0);

		
		// size
		BString sizeText;
		_CreateSizeText(size, &sizeText);
		rect.right = rect.left + sizeWidth;
		BStringView* sizeView = new BStringView(rect, "type", sizeText.String());
		sizeView->SetAlignment(B_ALIGN_RIGHT);
		row->AddChild(sizeView);
		rect.OffsetBy(sizeWidth + kDistance, 0);
		
		// path
		rect.right = rect.left + pathWidth;
		BStringView* pathView = new BStringView(rect, "path", path.String());
		row->AddChild(pathView);				
	}
	
	
	fPartitionsWidth = totalWidth;
	fPartitionsHeight = frame.Height();	
}


void
PartitionsPage::_ComputeColumnWidths(int32& showWidth, int32& nameWidth, 
		int32& typeWidth, int32& sizeWidth, int32& pathWidth)
{
	BCheckBox checkBox(BRect(0, 0, 100, 100), "show", "", new BMessage());
	checkBox.ResizeToPreferred();
	showWidth = checkBox.Bounds().IntegerWidth();
	// reserve space for about 16 characters  
	nameWidth = (int32)ceil(be_plain_font->StringWidth("oooooooooooooooo"));
	
	const int32 kTextControlInsets = 6;
	const int32 kStringViewInsets = 2;
	
	BMessage message;
	for (int32 i = 0; fSettings->FindMessage("partition", i, &message) == B_OK; i ++) {
		// get partition data
		BString name;
		BString type;
		BString path;
		int64 size;
		message.FindString("name", &name);
		message.FindString("type", &type);
		message.FindString("path", &path);
		message.FindInt64("size", &size);
		BString sizeText;
		_CreateSizeText(size, &sizeText);

		int32 width = (int32)ceil(be_plain_font->StringWidth(name.String())) +
			kTextControlInsets;	
		if (nameWidth < width)
			nameWidth = width;

		width = (int32)ceil(be_plain_font->StringWidth(type.String())) +
			kStringViewInsets;	
		if (typeWidth < width)
			typeWidth = width;

		width = (int32)ceil(be_plain_font->StringWidth(path.String())) +
			kStringViewInsets;
		if (pathWidth < width)
			pathWidth = width;

		width = (int32)ceil(be_plain_font->StringWidth(sizeText.String())) +
			kStringViewInsets;
		if (sizeWidth < width)
			sizeWidth = width;
	}
}


static const char kPrefix[] = " KMGTPE";


void 
PartitionsPage::_CreateSizeText(int64 _size, BString* text)
{
	const char* suffixes[] = {
		"", "K", "M", "G", "T", NULL
	};

	double size = _size;
	int index = 0;
	while (size > 1024 && suffixes[index + 1]) {
		size /= 1024;
		index++;
	}

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "%.2f%s", size, suffixes[index]);

	*text = buffer;
}


BMessage* 
PartitionsPage::_CreateControlMessage(uint32 what, int32 index)
{
	BMessage* message = new BMessage(what);
	message->AddInt32("index", index);
	return message;
}
