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
	BTextView(name)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	//SetLayout(new BGroupLayout(B_VERTICAL));
	//	SetText("xx");
	MakeEditable(false);
}


PropertyListPlain::~PropertyListPlain()
{
	
}


void
PropertyListPlain::AddAttributes(const Attributes& attributes)
{
	RemoveAll();
	//BGridLayoutBuilder builder(5, 5);
	unsigned int i;
	string txt="";
	for (i = 0; i < attributes.size(); i++) {
		/*const char* name = attributes[i].fName;
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
			.Add(valueView, 1, i);*/
		//SetText("aa");
		BString tempViewName(attributes[i].fName);
		const char* value = attributes[i].fValue;
		//txt+=attributes[i].fName;
		txt=txt+tempViewName.String()+": "+value;
	}

	SetText(txt.c_str());
	// make sure that all content is left and top aligned
	//builder.Add(BSpaceLayoutItem::CreateGlue(), 2, i);

	/*AddChild(BGroupLayoutBuilder(B_VERTICAL,5)
		.Add(builder)
		.SetInsets(5, 5, 5, 5)
	);*/
}


void
PropertyListPlain::RemoveAll()
{
	SetText("");
}


void
PropertyListPlain::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BTextView::MessageReceived(message);
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


