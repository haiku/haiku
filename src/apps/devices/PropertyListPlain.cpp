/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 *		Modified by Pieter Panman
 */

#include "PropertyListPlain.h"

#include <GridLayout.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Message.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <StringView.h>
#include <TextView.h>

#include "Device.h"


PropertyListPlain::PropertyListPlain(const char* name)
	:
	BView(name, 0, NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLayout(new BGroupLayout(B_VERTICAL));
}


PropertyListPlain::~PropertyListPlain()
{

}


void
PropertyListPlain::AddAttributes(const Attributes& attributes)
{
	RemoveAll();
	BGroupLayoutBuilder layout(B_VERTICAL);

	BTextView* view = new BTextView(BRect(0, 0, 1000, 1000),
		"attribs", BRect(5, 5, 995, 995), B_FOLLOW_ALL_SIDES);

	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	view->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	view->MakeEditable(false);

	for (unsigned int i = 0; i < attributes.size(); i++) {
		BString attributeLine;
		attributeLine << attributes[i].fName
			<< "\t" << attributes[i].fValue << "\n";
		view->Insert(attributeLine);

		view->SetExplicitAlignment(
			BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	}
	layout.Add(view);

	fAttributeViews.AddItem(view);
	AddChild(layout);
}


void
PropertyListPlain::RemoveAll()
{
	BView *child;
	while ((child = ChildAt(0))) {
		RemoveChild(child);
		delete child;
	}
}


void
PropertyListPlain::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_COLORS_UPDATED:
		{
			rgb_color color;
			if (message->FindColor(ui_color_name(B_PANEL_TEXT_COLOR), &color)
					!= B_OK)
				break;

			BTextView* view = NULL;
			int32 count = fAttributeViews.CountItems();
			for (int32 index = 0; index < count; ++index) {
				view = fAttributeViews.ItemAt(index);
				view->SetFontAndColor(be_plain_font, 0, &color);
			}
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
PropertyListPlain::AttachedToWindow()
{
}


void
PropertyListPlain::DetachedFromWindow()
{
}
