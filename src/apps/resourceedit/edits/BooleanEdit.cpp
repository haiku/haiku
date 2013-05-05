/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "NormalEdit.h"
#include "ResourceRow.h"

#include <CheckBox.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>


BooleanEdit::BooleanEdit()
	:
	EditView("BooleanEdit")
{

}


BooleanEdit::~BooleanEdit()
{

}


void
BooleanEdit::AttachTo(BView* view)
{
	EditView::AttachTo(view);

	SetLayout(new BGroupLayout(B_VERTICAL));

	fValueCheck = new BCheckBox(BRect(0, 0, 0, 0), "fValueText",
		"", NULL);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fValueCheck)
		.AddGlue()
	);

	view->AddChild(this);
}


void
BooleanEdit::Edit(ResourceRow* row)
{
	EditView::Edit(row);

	BString data = fRow->ResourceData();

	if (data == "" || data == "0" || data == "false")
		fValueCheck->SetValue(B_CONTROL_OFF);
	else
		fValueCheck->SetValue(B_CONTROL_ON);
}


void
BooleanEdit::Commit()
{
	EditView::Commit();

	if (fValueCheck->Value() == B_CONTROL_ON)
		fRow->SetResourceData("true");
	else
		fRow->SetResourceData("false");
}
