/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ColorProperty.h"

#include <new>
#include <stdio.h>

#include <Message.h>

#include "support_ui.h"
#include "ui_defines.h"

using std::nothrow;

// constructor
ColorProperty::ColorProperty(uint32 identifier)
	: Property(identifier),
	  fValue(kBlack)
{
}

// constructor
ColorProperty::ColorProperty(uint32 identifier, rgb_color color)
	: Property(identifier),
	  fValue(color)
{
}

// constructor
ColorProperty::ColorProperty(const ColorProperty& other)
	: Property(other),
	  fValue(other.fValue)
{
}

// constructor
ColorProperty::ColorProperty(BMessage* archive)
	: Property(archive),
	  fValue(kBlack)
{
	if (!archive)
		return;

	if (archive->FindInt32("value", (int32*)&fValue) < B_OK)
		fValue = kBlack;
}

// destrucor
ColorProperty::~ColorProperty()
{
}

// Archive
status_t
ColorProperty::Archive(BMessage* into, bool deep) const
{
	status_t ret = Property::Archive(into, deep);

	if (ret >= B_OK)
		ret = into->AddInt32("value", (uint32&)fValue);

	// finish off
	if (ret >= B_OK)
		ret = into->AddString("class", "ColorProperty");

	return ret;
}

// Instantiate
BArchivable*
ColorProperty::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "ColorProperty"))
		return new ColorProperty(archive);

	return NULL;
}

// Clone
Property*
ColorProperty::Clone() const
{
	return new (nothrow) ColorProperty(*this);
}

// SetValue
bool
ColorProperty::SetValue(const char* str)
{
	rgb_color value = fValue;
	if (*str == '#') {
		str++;
		int32 length = strlen(str);
		unsigned scannedColor = 0;
		char expanded[7];
		if (length == 3) {
			// if there are only 3 bytes, than it means that we
			// need to expand the color (#f60 -> #ff6600)
			// TODO: There must be an easier way...
			expanded[0] = *str;
			expanded[1] = *str++;
			expanded[2] = *str;
			expanded[3] = *str++;
			expanded[4] = *str;
			expanded[5] = *str++;
			expanded[6] = 0;
			str = expanded;
		}
		if (sscanf(str, "%x", &scannedColor) == 1) {
			uint8* colorByte = (uint8*)&scannedColor;
			value.red	= colorByte[3];
			value.green	= colorByte[2];
			value.blue	= colorByte[1];
			value.alpha	= colorByte[0];
		}
	} else {
		// TODO: parse "named color"
	}
	return SetValue(value);
}

// SetValue
bool
ColorProperty::SetValue(const Property* other)
{
	const ColorProperty* c = dynamic_cast<const ColorProperty*>(other);
	if (c)
		return SetValue(c->Value());
	return false;
}

// GetValue
void
ColorProperty::GetValue(BString& string)
{
	char valueString[16];
	sprintf(valueString, "#%02x%02x%02x%02x",
			fValue.red, fValue.green, fValue.blue, fValue.alpha);
	string << valueString;
}

// InterpolateTo
bool
ColorProperty::InterpolateTo(const Property* other, float scale)
{
	const ColorProperty* c = dynamic_cast<const ColorProperty*>(other);
	if (c) {
		rgb_color a = fValue;
		const rgb_color& b = c->fValue;
		a.red = a.red + (uint8)floorf((b.red - a.red) * scale + 0.5);
		a.green = a.green + (uint8)floorf((b.green - a.green) * scale + 0.5);
		a.blue = a.blue + (uint8)floorf((b.blue - a.blue) * scale + 0.5);
		a.alpha = a.alpha + (uint8)floorf((b.alpha - a.alpha) * scale + 0.5);
		return SetValue(a);
	}
	return false;
}

// SetValue
bool
ColorProperty::SetValue(rgb_color color)
{
	if (fValue != color) {
		fValue = color;
		return true;
	}
	return false;
}

// Value
rgb_color
ColorProperty::Value() const
{
	return fValue;
}



