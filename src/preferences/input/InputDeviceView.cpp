/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#include "InputDeviceView.h"


#include <Catalog.h>
#include <DateFormat.h>
#include <Input.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <ScrollView.h>
#include <String.h>
#include <StringItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceList"


InputIcons* DeviceListItemView::sIcons = NULL;


DeviceListItemView::DeviceListItemView(BString title, input_type type)
	:
	BListItem((uint32)0),
	fTitle(title),
	fInputType(type)
{
}

struct DeviceListItemView::Renderer {
	Renderer()
		:
		fTitle(NULL),
		fPrimaryIcon(NULL),
		fSelected(false)
	{
	}

	void AddIcon(BBitmap* icon)
	{
		if (!fPrimaryIcon)
			fPrimaryIcon = icon;
	}

	void SetTitle(const char* title)
	{
		fTitle = title;
	}

	void SetSelected(bool selected)
	{
		fSelected = selected;
	}

	void Render(BView* onto, BRect frame, bool complete = false)
	{
		const rgb_color lowColor = onto->LowColor();
		const rgb_color highColor = onto->HighColor();

		if (fSelected || complete) {
			if (fSelected)
				onto->SetLowColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
			onto->FillRect(frame, B_SOLID_LOW);
		}

		BPoint point(frame.left + 4.0f,
			frame.top + (frame.Height() - InputIcons::sBounds.Height()) / 2.0f);

		BRect iconFrame(InputIcons::IconRectAt(point + BPoint(1, 0)));

		onto->SetDrawingMode(B_OP_OVER);
		if (fPrimaryIcon) {
			onto->DrawBitmap(fPrimaryIcon, iconFrame);
			point.x = iconFrame.right + 1;
		}

		onto->SetDrawingMode(B_OP_COPY);

		BFont font = be_plain_font;
		font_height	fontInfo;
		font.GetHeight(&fontInfo);

		onto->SetFont(&font);
		onto->MovePenTo(point.x + 8, frame.top
			+ fontInfo.ascent + (frame.Height()
			- ceilf(fontInfo.ascent + fontInfo.descent)) / 2.0f);
		onto->DrawString(fTitle);

		onto->SetHighColor(highColor);
		onto->SetLowColor(lowColor);
	}

	float ItemWidth()
	{
		float width = 4.0f;
		width += be_plain_font->StringWidth(fTitle) + 16.0f;
		return width;
	}
private:

	BString		fTitle;
	BBitmap*	fPrimaryIcon;
	bool		fSelected;
};


void
DeviceListItemView::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner, font);

	float iconHeight = InputIcons::sBounds.Height() + 1;
	if ((Height() < iconHeight + kITEM_MARGIN * 2)) {
		SetHeight(iconHeight + kITEM_MARGIN * 2);
	}

	Renderer renderer;
	renderer.SetTitle(Label());
	renderer.SetTitle(fTitle);
	SetRenderParameters(renderer);
	SetWidth(renderer.ItemWidth());
};


void
DeviceListItemView::DrawItem(BView* owner, BRect frame, bool complete)
{
	Renderer renderer;
	renderer.SetSelected(IsSelected());
	renderer.SetTitle(Label());
	SetRenderParameters(renderer);
	renderer.Render(owner, frame, complete);
};


void
DeviceListItemView::SetRenderParameters(Renderer& renderer)
{
	if (fInputType == MOUSE_TYPE)
		renderer.AddIcon(&Icons()->mouseIcon);

	else if (fInputType == TOUCHPAD_TYPE)
		renderer.AddIcon(&Icons()->touchpadIcon);

	else if (fInputType == KEYBOARD_TYPE)
		renderer.AddIcon(&Icons()->keyboardIcon);
}
