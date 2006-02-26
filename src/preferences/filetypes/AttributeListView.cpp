/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeListView.h"

#include <stdio.h>


const struct type_map kTypeMap[] = {
	{"String",			B_STRING_TYPE},
	{"Boolean",			B_BOOL_TYPE},
	{"Integer 8 bit",	B_INT8_TYPE},
	{"Integer 16 bit",	B_INT16_TYPE},
	{"Integer 32 bit",	B_INT32_TYPE},
	{"Integer 64 bit",	B_INT64_TYPE},
	{"Float",			B_FLOAT_TYPE},
	{"Double",			B_DOUBLE_TYPE},
	{"Time",			B_TIME_TYPE},
	{NULL,				0}
};


static void
name_for_type(BString& string, type_code type)
{
	for (int32 i = 0; kTypeMap[i].name != NULL; i++) {
		if (kTypeMap[i].type == type) {
			string = kTypeMap[i].name;
			return;
		}
	}

	char buffer[32];
	buffer[0] = '\'';
	buffer[1] = 0xff & (type >> 24);
	buffer[2] = 0xff & (type >> 16);
	buffer[3] = 0xff & (type >> 8);
	buffer[4] = 0xff & (type);
	buffer[5] = '\'';
	buffer[6] = 0;
	for (int16 i = 0;i < 4;i++) {
		if (buffer[i] < ' ')
			buffer[i] = '.';
	}

	snprintf(buffer + 6, sizeof(buffer), " (0x%lx)", type);
	string = buffer;
}


AttributeItem *
create_attribute_item(BMessage& attributes, int32 index)
{
	const char* publicName;
	if (attributes.FindString("attr:public_name", index, &publicName) != B_OK)
		return NULL;

	const char* name;
	if (attributes.FindString("attr:name", index, &name) != B_OK)
		name = "-";

	type_code type;
	if (attributes.FindInt32("attr:type", index, (int32 *)&type) != B_OK)
		type = B_STRING_TYPE;

	const char* displayAs;
	if (attributes.FindString("attr:display_as", index, &displayAs) != B_OK)
		displayAs = NULL;

	bool editable;
	if (attributes.FindBool("attr:editable", index, &editable) != B_OK)
		editable = false;
	bool visible;
	if (attributes.FindBool("attr:viewable", index, &visible) != B_OK)
		visible = false;

	int32 alignment;
	if (attributes.FindInt32("attr:alignment", index, &alignment) != B_OK)
		alignment = B_ALIGN_LEFT;

	int32 width;
	if (attributes.FindInt32("attr:width", index, &width) != B_OK)
		width = 50;

	return new AttributeItem(name, publicName, type, displayAs, alignment,
		width, visible, editable);
}


//	#pragma mark -


AttributeItem::AttributeItem(const char* name, const char* publicName,
		type_code type, const char* displayAs, int32 alignment,
		int32 width, bool visible, bool editable)
	: BStringItem(publicName),
	fName(name),
	fType(type),
	fDisplayAs(displayAs),
	fAlignment(alignment),
	fWidth(width),
	fVisible(visible),
	fEditable(editable)
{
}


AttributeItem::AttributeItem()
	: BStringItem(""),
	fType(B_STRING_TYPE),
	fAlignment(B_ALIGN_LEFT),
	fWidth(60),
	fVisible(true),
	fEditable(false)
{
}


AttributeItem::AttributeItem(const AttributeItem& other)
	: BStringItem(other.PublicName())
{
	*this = other;
}


AttributeItem::~AttributeItem()
{
}


void
AttributeItem::DrawItem(BView* owner, BRect frame, bool drawEverything)
{
	BStringItem::DrawItem(owner, frame, drawEverything);

	rgb_color highColor = owner->HighColor();
	rgb_color lowColor = owner->LowColor();

	if (IsSelected())
		owner->SetLowColor(tint_color(lowColor, B_DARKEN_2_TINT));

	rgb_color black = {0, 0, 0, 255};

	if (!IsEnabled())
		owner->SetHighColor(tint_color(black, B_LIGHTEN_2_TINT));
	else
		owner->SetHighColor(black);

	owner->MovePenTo(frame.left + frame.Width() / 2.0f + 5.0f, owner->PenLocation().y);

	BString type;
	name_for_type(type, fType);
	owner->DrawString(type.String());

	owner->SetHighColor(tint_color(owner->ViewColor(), B_DARKEN_1_TINT));

	float middle = frame.left + frame.Width() / 2.0f;
	owner->StrokeLine(BPoint(middle, 0.0f), BPoint(middle, frame.bottom));

	owner->SetHighColor(highColor);
	owner->SetLowColor(lowColor);
}


AttributeItem&
AttributeItem::operator=(const AttributeItem& other)
{
	SetText(other.PublicName());
	fName = other.Name();
	fType = other.Type();
	fDisplayAs = other.DisplayAs();
	fAlignment = other.Alignment();
	fWidth = other.Width();
	fVisible = other.Visible();
	fEditable = other.Editable();

	return *this;
}


bool
AttributeItem::operator==(const AttributeItem& other) const
{
	return !strcmp(Name(), other.Name())
		&& !strcmp(PublicName(), other.PublicName())
		&& !strcmp(DisplayAs(), other.DisplayAs())
		&& Type() == other.Type()
		&& Alignment() == other.Alignment()
		&& Width() == other.Width()
		&& Visible() == other.Visible()
		&& Editable() == other.Editable();
}


bool
AttributeItem::operator!=(const AttributeItem& other) const
{
	return !(*this == other);
}


//	#pragma mark -


AttributeListView::AttributeListView(BRect frame, const char* name,
		uint32 resizingMode)
	: BListView(frame, name, B_SINGLE_SELECTION_LIST, resizingMode,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE)
{
}


AttributeListView::~AttributeListView()
{
	_DeleteItems();
}


void
AttributeListView::_DeleteItems()
{
	for (int32 i = CountItems(); i-- > 0;) {
		delete ItemAt(i);
	}
	MakeEmpty();
}


void
AttributeListView::SetTo(BMimeType* type)
{
	_DeleteItems();

	// fill it again
	
	if (type == NULL)
		return;

	BMessage attributes;
	if (type->GetAttrInfo(&attributes) != B_OK)
		return;

	AttributeItem* item;
	int32 i = 0;
	while ((item = create_attribute_item(attributes, i++)) != NULL) {
		AddItem(item);
	}
}


void
AttributeListView::Draw(BRect updateRect)
{
	BListView::Draw(updateRect);

	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));

	float middle = Bounds().Width() / 2.0f;
	StrokeLine(BPoint(middle, 0.0f), BPoint(middle, Bounds().bottom));
}

