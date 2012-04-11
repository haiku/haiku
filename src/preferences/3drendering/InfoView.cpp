/*
 * Copyright 2009-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alex Wilson <yourpalal2@gmail.com>
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include "InfoView.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <String.h>
#include <StringView.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "InfoView"


const BAlignment kLabelAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET);
const BAlignment kValueAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET);


InfoView::InfoView()
	:
	BGridView(B_TRANSLATE("Information"))
{
	_AddString(B_TRANSLATE("GL version:"),
		(const char*)glGetString(GL_VERSION));
	_AddString(B_TRANSLATE("Vendor name:"),
		(const char*)glGetString(GL_VENDOR));
	_AddString(B_TRANSLATE("Renderer name:"),
		(const char*)glGetString(GL_RENDERER));
	_AddString(B_TRANSLATE("GLU version:"),
		(const char*)gluGetString(GLU_VERSION));
	_AddString(B_TRANSLATE("GLUT API version:"),
		BString() << (int32)GLUT_API_VERSION);

	BGridLayout* layout = GridLayout();
	layout->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	layout->AddItem(BSpaceLayoutItem::CreateGlue(), 0, layout->CountRows(),
		layout->CountColumns(), 1);

	// Set horizontal spacing to 0, and use the middle column as
	// variable-width spacing (like layout 'glue').
	layout->SetHorizontalSpacing(0);
	layout->SetMinColumnWidth(1, be_control_look->DefaultLabelSpacing());
	layout->SetMaxColumnWidth(1, B_SIZE_UNLIMITED);
}


InfoView::~InfoView()
{
}


void
InfoView::_AddString(const char* label, const char* value)
{
	BView* labelView = new BStringView(NULL, label);
	labelView->SetExplicitAlignment(kLabelAlignment);

	BView* valueView = new BStringView(NULL, value);
	valueView->SetExplicitAlignment(kValueAlignment);

	int32 rows = GridLayout()->CountRows();
	BLayoutBuilder::Grid<>(this)
		.Add(labelView, 0, rows)
		.Add(valueView, 2, rows);
}
