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

#include "Device.h"


PropertyListPlain::PropertyListPlain(const char* name)
	:
	BView(name, 0, NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLayout(new BGroupLayout(B_VERTICAL));
}


PropertyListPlain::~PropertyListPlain()
{
	
}


void
PropertyListPlain::AddAttributes(const Attributes& attributes)
{
	RemoveAll();
	BGridLayoutBuilder builder(5, 5);
	unsigned int i;
	for (i = 0; i < attributes.size(); i++) {
		const char* name = attributes[i].fName;
		const char* value = attributes[i].fValue;
		
		BString tempViewName(name);
		BStringView *nameView = new BStringView(tempViewName.String(), name);
		tempViewName.Append("Value");
		BStringView *valueView = new BStringView(tempViewName.String(), value);

		nameView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
			B_ALIGN_VERTICAL_UNSET));
		valueView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
			B_ALIGN_VERTICAL_UNSET));

		builder
			.Add(nameView, 0, i)
			.Add(valueView, 1, i);
	}

	// make sure that all content is left and top aligned
	builder.Add(BSpaceLayoutItem::CreateGlue(), 2, i);

	AddChild(BGroupLayoutBuilder(B_VERTICAL,5)
		.Add(builder)
		.SetInsets(5, 5, 5, 5)
	);
}


void
PropertyListPlain::RemoveAll()
{
	BView *child;
	while((child = ChildAt(0))) {
		RemoveChild(child);
		delete child;
	}
}


void
PropertyListPlain::MessageReceived(BMessage* message)
{
	switch (message->what) {
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
