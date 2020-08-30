/*
 * Copyright (C) 2019-2020 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#include "AttributesView.h"

#include <Catalog.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <DateTimeFormat.h>
#include <FormattingConventions.h>
#include <fs_attr.h>
#include <Node.h>
#include <StringFormat.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AttributesView"

int kValueColumn = 1;
int kTypeColumn = 2;


AttributesView::AttributesView(Model* model)
	:
	BGroupView(B_VERTICAL, 0),
	fListView(new BColumnListView("attrs", 0, B_PLAIN_BORDER, false))
{
	SetName(B_TRANSLATE("Attributes"));
	AddChild(fListView);
	GroupLayout()->SetInsets(-1, -1, -1, -1);

	float nameWidth = StringWidth("SYS:PACKAGE_FILE") + 16;
	float typeMaxWidth = StringWidth(B_TRANSLATE(
		"Double-precision floating point number")) + 16;
	float typeWidth = StringWidth(B_TRANSLATE("64-bit unsigned integer")) + 16;
	float valueMaxWidth = StringWidth("W") * 64 + 16;
	float valueWidth = StringWidth("(94.00, 95.00) (1920, 1080)") + 16;
	BStringColumn* nameColumn = new BStringColumn(B_TRANSLATE("Name"),
		nameWidth, nameWidth, nameWidth, 0);
	BStringColumn* typeColumn = new BStringColumn(B_TRANSLATE("Type"),
		typeWidth, typeWidth, typeMaxWidth, 0);
	BStringColumn* valueColumn = new BStringColumn(B_TRANSLATE("Value"),
		valueWidth, valueWidth, valueMaxWidth, 0);

	fListView->AddColumn(nameColumn, 0);
	fListView->AddColumn(valueColumn, 1);
	fListView->AddColumn(typeColumn, 2);

	SetExplicitMinSize(BSize(typeWidth + valueWidth + nameWidth + 40,
		B_SIZE_UNSET));

	BNode* node = model->Node();

	node->RewindAttrs();
	char name[B_ATTR_NAME_LENGTH];

	// Initialize formatters only once for all attributes
	BDateTimeFormat dateTimeFormatter;
	BStringFormat multiValueFormat(B_TRANSLATE(
		"{0, plural, other{<# values>}}"));

	while (node->GetNextAttrName(name) == B_OK) {
		// Skip well-known attributes already shown elsewhere in the window
		if (strcmp(name, "BEOS:TYPE") == 0)
			continue;

		attr_info info;
		node->GetAttrInfo(name, &info);
		BRow* row = new BRow;
		row->SetField(new BStringField(name), 0);

		BString representation;
		switch(info.type) {
			case B_STRING_TYPE:
			case B_MIME_STRING_TYPE:
			{
				// Use a small buffer, long strings will be truncated
				char buffer[64];
				ssize_t size = node->ReadAttr(name, info.type, 0, buffer,
					sizeof(buffer));
				if (size > 0)
					representation.SetTo(buffer, size);
				break;
			}
			case B_BOOL_TYPE:
			{
				if (info.size == sizeof(bool)) {
					bool value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation = value ? B_TRANSLATE("yes")
						: B_TRANSLATE("no");
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(bool));
				}
				break;
			}
			case B_INT16_TYPE:
			{
				if (info.size == sizeof(int16)) {
					int16 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%" B_PRId16, value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(int16));
				}
				break;
			}
			case B_INT32_TYPE:
			{
				if (info.size == sizeof(int32)) {
					int32 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%" B_PRId32, value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(int32));
				}
				break;
			}
			case B_INT64_TYPE:
			{
				if (info.size == sizeof(int64)) {
					int64 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%" B_PRId64, value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(int64));
				}
				break;
			}
			case B_INT8_TYPE:
			{
				if (info.size == sizeof(int8)) {
					int8 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%" B_PRId8, value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(int8));
				}
				break;
			}
			case B_RECT_TYPE:
			{
				if (info.size == sizeof(BRect)) {
					BRect value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("(%g,%g) (%g,%g)", value.left,
						value.top, value.right, value.bottom);
				} else {
					BStringFormat multiRectFormat(B_TRANSLATE(
						"{0, plural, other{<# rectangles>}}"));
					multiRectFormat.Format(representation,
						info.size / sizeof(BRect));
				}
				break;
			}
			case B_DOUBLE_TYPE:
			{
				if (info.size == sizeof(double)) {
					double value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%f", value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(double));
				}
				break;
			}
			case B_FLOAT_TYPE:
			{
				if (info.size == sizeof(float)) {
					float value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%f", value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(float));
				}
				break;
			}
			case B_TIME_TYPE:
			{
				// Try to handle attributes written on both 32 and 64bit systems
				if (info.size == sizeof(int32)) {
					int32 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					dateTimeFormatter.Format(representation, value,
						B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT);
				} else if (info.size == sizeof(int64)) {
					int64 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					dateTimeFormatter.Format(representation, value,
						B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT);
				} else {
					BStringFormat multiDateFormat(B_TRANSLATE(
						"{0, plural, other{<# dates>}}"));
					multiDateFormat.Format(representation,
						info.size / sizeof(time_t));
				}
				break;
			}
			case B_UINT16_TYPE:
			{
				if (info.size == sizeof(uint16)) {
					uint16 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%" B_PRIu16, value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(uint16));
				}
				break;
			}
			case B_UINT32_TYPE:
			{
				if (info.size == sizeof(uint32)) {
					uint32 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%" B_PRIu32, value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(uint32));
				}
				break;
			}
			case B_UINT64_TYPE:
			{
				if (info.size == sizeof(uint64)) {
					uint64 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%" B_PRIu64, value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(uint64));
				}
				break;
			}
			case B_UINT8_TYPE:
			{
				if (info.size == sizeof(uint8)) {
					uint8 value;
					node->ReadAttr(name, info.type, 0, &value, sizeof(value));
					representation.SetToFormat("%" B_PRIu8, value);
				} else {
					multiValueFormat.Format(representation,
						info.size / sizeof(uint8));
				}
				break;
			}
			default:
			{
				BStringFormat sizeFormat(B_TRANSLATE(
					"{0, plural, one{<# data byte>} other{<# bytes of data>}}"));
				sizeFormat.Format(representation, info.size);
				break;
			}
		}
		row->SetField(new BStringField(representation), kValueColumn);

		switch(info.type) {
			case B_AFFINE_TRANSFORM_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Affine transform")),
					kTypeColumn);
				break;
			case B_ALIGNMENT_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Alignment")),
					kTypeColumn);
				break;
			case B_ANY_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Any")),
					kTypeColumn);
				break;
			case B_ATOM_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Atom")),
					kTypeColumn);
				break;
			case B_ATOMREF_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Atom reference")),
					kTypeColumn);
				break;
			case B_BOOL_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Boolean")),
					kTypeColumn);
				break;
			case B_CHAR_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Character")),
					kTypeColumn);
				break;
			case B_COLOR_8_BIT_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Palette-indexed picture")), kTypeColumn);
				break;
			case B_DOUBLE_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Double-precision floating point number")), kTypeColumn);
				break;
			case B_FLOAT_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Floating point number")), kTypeColumn);
				break;
			case B_GRAYSCALE_8_BIT_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Grayscale picture")), kTypeColumn);
				break;
			case B_INT16_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("16-bit integer")),
					kTypeColumn);
				break;
			case B_INT32_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("32-bit integer")),
					kTypeColumn);
				break;
			case B_INT64_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("64-bit integer")),
					kTypeColumn);
				break;
			case B_INT8_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("8-bit integer")),
					kTypeColumn);
				break;
			case B_LARGE_ICON_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Bitmap icon")),
					kTypeColumn);
				break;
			case B_MEDIA_PARAMETER_GROUP_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Media parameter group")), kTypeColumn);
				break;
			case B_MEDIA_PARAMETER_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Media parameter")),
					kTypeColumn);
				break;
			case B_MEDIA_PARAMETER_WEB_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Media parameter web")), kTypeColumn);
				break;
			case B_MESSAGE_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Message")),
					kTypeColumn);
				break;
			case B_MESSENGER_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Messenger")),
					kTypeColumn);
				break;
			case B_MIME_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("MIME Type")),
					kTypeColumn);
				break;
			case B_MINI_ICON_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Small bitmap icon")), kTypeColumn);
				break;
			case B_MONOCHROME_1_BIT_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Monochrome picture")), kTypeColumn);
				break;
			case B_OBJECT_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Object")),
					kTypeColumn);
				break;
			case B_OFF_T_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("File offset")),
					kTypeColumn);
				break;
			case B_PATTERN_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Drawing pattern")),
					kTypeColumn);
				break;
			case B_POINTER_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Memory pointer")),
					kTypeColumn);
				break;
			case B_POINT_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Point")),
					kTypeColumn);
				break;
			case B_PROPERTY_INFO_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Property info")),
					kTypeColumn);
				break;
			case B_RAW_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Raw data")),
					kTypeColumn);
				break;
			case B_RECT_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Rectangle")),
					kTypeColumn);
				break;
			case B_REF_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Entry ref")),
					kTypeColumn);
				break;
			case B_NODE_REF_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Node ref")),
					kTypeColumn);
				break;
			case B_RGB_32_BIT_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"True-color picture")),	kTypeColumn);
				break;
			case B_RGB_COLOR_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Color")),
					kTypeColumn);
				break;
			case B_SIZE_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Geometric size")),
					kTypeColumn);
				break;
			case B_SIZE_T_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Memory size")),
					kTypeColumn);
				break;
			case B_SSIZE_T_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Signed memory size")), kTypeColumn);
				break;
			case B_STRING_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Plain text")),
					kTypeColumn);
				break;
			case B_STRING_LIST_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Text list")),
					kTypeColumn);
				break;
			case B_TIME_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Time")),
					kTypeColumn);
				break;
			case B_UINT16_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"16-bit unsigned integer")), kTypeColumn);
				break;
			case B_UINT32_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"32-bit unsigned integer")), kTypeColumn);
				break;
			case B_UINT64_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"64-bit unsigned integer")), kTypeColumn);
				break;
			case B_UINT8_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"8-bit unsigned integer")), kTypeColumn);
				break;
			case B_VECTOR_ICON_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Icon")),
					kTypeColumn);
				break;
			case B_XATTR_TYPE:
				row->SetField(new BStringField(B_TRANSLATE(
					"Extended attribute")), kTypeColumn);
				break;
			case B_NETWORK_ADDRESS_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("Network address")),
					kTypeColumn);
				break;
			case B_MIME_STRING_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("MIME String")),
					kTypeColumn);
				break;
			case B_ASCII_TYPE:
				row->SetField(new BStringField(B_TRANSLATE("ASCII Text")),
					kTypeColumn);
				break;
			default:
				row->SetField(new BStringField(B_TRANSLATE("(unknown)")),
					kTypeColumn);
				break;
		}
		fListView->AddRow(row);
	}

	int32 rows = fListView->CountRows(NULL);
	if (rows < 5)
		rows = 5;
	BRow* first = fListView->RowAt(0, NULL);
	if (first != NULL) {
		float height = first->Height() * (rows + 2);
		SetExplicitMaxSize(BSize(B_SIZE_UNSET, height));
	}

}
