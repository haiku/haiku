/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "NormalEdit.h"
#include "ResourceRow.h"

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <TextControl.h>


NormalEdit::NormalEdit()
	:
	EditView("NormalEdit")
{

}


NormalEdit::~NormalEdit()
{

}


void
NormalEdit::AttachTo(BView* view)
{
	EditView::AttachTo(view);

	SetLayout(new BGroupLayout(B_VERTICAL));

	fValueText = new BTextControl(BRect(0, 0, 0, 0), "fValueText",
		"Value:", "", NULL);
	fValueText->SetDivider(50);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fValueText)
		.AddGlue()
	);

	view->AddChild(this);
}


void
NormalEdit::Edit(ResourceRow* row)
{
	EditView::Edit(row);

	fValueText->SetText(fRow->ResourceData());
}


void
NormalEdit::Commit()
{
	EditView::Commit();

	fRow->SetResourceData(fValueText->Text());
}
