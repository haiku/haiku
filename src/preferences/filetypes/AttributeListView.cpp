/*
 * Copyright 2006-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeListView.h"

#include <stdio.h>

#include <ObjectList.h>


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

// TODO: in the future, have a (private) Tracker API that exports these
//	as well as a nice GUI for them.
const struct display_as_map kDisplayAsMap[] = {
	{"Default",		NULL,		{}},
	{"Checkbox",	"checkbox",
		{B_BOOL_TYPE, B_INT8_TYPE, B_INT16_TYPE, B_INT32_TYPE}},
	{"Duration",	"duration",
		{B_TIME_TYPE, B_INT8_TYPE, B_INT16_TYPE, B_INT32_TYPE, B_INT64_TYPE}},
	{"Rating",		"rating",	{B_INT8_TYPE, B_INT16_TYPE, B_INT32_TYPE}},
	{NULL,			NULL,		{}}
};


static void
add_display_as(BString& string, const char* displayAs)
{
	if (displayAs == NULL || !displayAs[0])
		return;

	BString base(displayAs);
	int32 end = base.FindFirst(':');
	if (end > 0)
		base.Truncate(end);

	for (int32 i = 0; kDisplayAsMap[i].name != NULL; i++) {
		if (base.ICompare(kDisplayAsMap[i].identifier) == 0) {
			string += ", ";
			string += base;
			return;
		}
	}
}


static void
name_for_type(BString& string, type_code type, const char* displayAs)
{
	for (int32 i = 0; kTypeMap[i].name != NULL; i++) {
		if (kTypeMap[i].type == type) {
			string = kTypeMap[i].name;
			add_display_as(string, displayAs);
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
	for (int16 i = 0; i < 4; i++) {
		if (buffer[i] < ' ')
			buffer[i] = '.';
	}

	snprintf(buffer + 6, sizeof(buffer), " (0x%" B_PRIx32 ")", type);
	string = buffer;
}


AttributeItem*
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
	:
	BStringItem(publicName),
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
	:
	BStringItem(""),
	fType(B_STRING_TYPE),
	fAlignment(B_ALIGN_LEFT),
	fWidth(60),
	fVisible(true),
	fEditable(false)
{
}


AttributeItem::AttributeItem(const AttributeItem& other)
	:
	BStringItem(other.PublicName())
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

	owner->MovePenTo(frame.left + frame.Width() / 2.0f + 5.0f,
		owner->PenLocation().y);

	BString type;
	name_for_type(type, fType, fDisplayAs.String());
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


AttributeListView::AttributeListView(const char* name)
	: BListView(name, B_SINGLE_SELECTION_LIST,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS)
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
	AttributeItem selectedItem;
	if (CurrentSelection(0) >= 0)
		selectedItem = *(AttributeItem*)ItemAt(CurrentSelection(0));

	// Remove the current items but remember them for now. Also remember
	// the currently selected item.
	BObjectList<AttributeItem> previousItems(CountItems(), true);
	while (AttributeItem* item = (AttributeItem*)RemoveItem((int32)0))
		previousItems.AddItem(item);

	// fill it again

	if (type == NULL)
		return;

	BMessage attributes;
	if (type->GetAttrInfo(&attributes) != B_OK)
		return;

	AttributeItem* item;
	int32 i = 0;
	while ((item = create_attribute_item(attributes, i++)) != NULL)
		AddItem(item);

	// Maybe all the items are the same, except for one item. That
	// attribute probably just got added. We should select it so the user
	// can better follow what's going on. The problem we are solving by
	// doing it this way is that updates to the MIME database are very
	// asynchronous. Most likely we have created the new attribute ourselves,
	// but the notification comes so late, we can't know for sure.
	if (CountItems() == previousItems.CountItems() + 1) {
		// First try to make sure that every previous item is there again.
		bool allPreviousItemsFound = true;
		for (i = previousItems.CountItems() - 1; i >= 0; i--) {
			bool previousItemFound = false;
			for (int32 j = CountItems() - 1; j >= 0; j--) {
				item = (AttributeItem*)ItemAt(j);
				if (*item == *previousItems.ItemAt(i)) {
					previousItemFound = true;
					break;
				}
			}
			if (!previousItemFound) {
				allPreviousItemsFound = false;
				break;
			}
		}
		if (allPreviousItemsFound) {
			for (i = CountItems() - 1; i >= 0; i--) {
				item = (AttributeItem*)ItemAt(i);
				bool foundNewItem = false;
				for (int32 j = previousItems.CountItems() - 1; j >= 0; j--) {
					if (*item != *previousItems.ItemAt(j)) {
						foundNewItem = true;
						break;
					}
				}
				if (foundNewItem) {
					Select(i);
					ScrollToSelection();
					break;
				}
			}
		}
	} else {
		// Try to re-selected a previously selected item, if it's the exact
		// same attribute. This helps not loosing the selection, since changes
		// to the model are followed by completely rebuilding the list all the
		// time.
		for (i = CountItems() - 1; i >= 0; i--) {
			item = (AttributeItem*)ItemAt(i);
			if (*item == selectedItem) {
				Select(i);
				ScrollToSelection();
				break;
			}
		}
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

