/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */


#include "SymbolButton.h"

#include <GradientLinear.h>
#include <LayoutUtils.h>
#include <Shape.h>


static const rgb_color kGreen = (rgb_color){ 116, 224, 0, 255 };


// constructor
SymbolButton::SymbolButton(const char* name, BShape* symbolShape,
		BMessage* message, uint32 borders)
	:
	BButton(name, NULL, message),
	fSymbol(symbolShape),
	fBorders(borders)
{
}


SymbolButton::~SymbolButton()
{
	delete fSymbol;
}


void
SymbolButton::Draw(BRect updateRect)
{
	uint32 flags = be_control_look->Flags(this);
	rgb_color base = LowColor();
	BRect bounds(Bounds());

	if (fBorders != 0) {
		be_control_look->DrawButtonFrame(this, bounds, updateRect, base,
			base, flags & ~BControlLook::B_DISABLED, fBorders);
		be_control_look->DrawButtonBackground(this, bounds, updateRect, base,
			flags);
	} else
		FillRect(updateRect, B_SOLID_LOW);

	if (fSymbol == NULL)
		return;

	if (IsEnabled()) {
		if (Value() == B_CONTROL_ON)
			base = tint_color(base, (B_DARKEN_4_TINT + B_DARKEN_MAX_TINT) / 2);
		else
			base = tint_color(base, B_DARKEN_4_TINT);
	} else {
		if (Value() == B_CONTROL_ON)
			base = tint_color(base, B_DARKEN_2_TINT);
		else
			base = tint_color(base, B_DARKEN_1_TINT);
	}

	BPoint offset;
	offset.x = (bounds.left + bounds.right) / 2;
	offset.y = (bounds.top + bounds.bottom) / 2;
	offset.x -= fSymbol->Bounds().Width() / 2;
	offset.y -= fSymbol->Bounds().Height() / 2;
	offset.x = floorf(offset.x - fSymbol->Bounds().left);
	offset.y = ceilf(offset.y - fSymbol->Bounds().top);

	MovePenTo(offset);
	BGradientLinear gradient;
	gradient.AddColor(tint_color(base, B_DARKEN_1_TINT), 0);
	gradient.AddColor(base, 255);
	gradient.SetStart(offset);
	offset.y += fSymbol->Bounds().Height();
	gradient.SetEnd(offset);
	FillShape(fSymbol, gradient);
}


BSize
SymbolButton::MinSize()
{
	if (fSymbol == NULL)
		return BButton::MinSize();

	float scale = fBorders != 0 ? 2.5f : 1.0f;

	BSize size;
	size.width = ceilf(fSymbol->Bounds().Width() * scale);
	size.height = ceilf(fSymbol->Bounds().Height() * scale);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
SymbolButton::MaxSize()
{
	BSize size(MinSize());
	if (fBorders != 0)
		size.width = ceilf(size.width * 1.5f);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


void
SymbolButton::SetSymbol(BShape* symbolShape)
{
	BSize oldSize = MinSize();

	delete fSymbol;
	fSymbol = symbolShape;

	if (MinSize() != oldSize)
		InvalidateLayout();

	Invalidate();
}

