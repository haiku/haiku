/*
 * Copyright 2009-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione <jscipione@gmail.com>
 *		Alex Wilson <yourpalal2@gmail.com>
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include "InfoView.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GridLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <String.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InfoView"


const BAlignment kLabelAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET);
const BAlignment kValueAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET);


// <bold>Render name</bold>
// Vendor Name           GL Version
// GLU version           GLUT API version
//
// example:
// Software rasterizer for X86/MMX/SSE2
// Mesa Project          2.1 Mesa 8.1-devel (git-2402c0)
// GLU 1.3               GLUT API 5

InfoView::InfoView()
	:
	BGroupView(B_TRANSLATE("Information"), B_HORIZONTAL)
{
	BStringView* rendererView = new BStringView(NULL,
		(const char*)glGetString(GL_RENDERER));
	rendererView->SetExplicitAlignment(kLabelAlignment);
	rendererView->SetFont(be_bold_font);

	BStringView* vendorNameView = new BStringView(NULL,
		(const char*)glGetString(GL_VENDOR));
	vendorNameView->SetExplicitAlignment(kLabelAlignment);

	BStringView* glVersionView = new BStringView(NULL,
		(const char*)glGetString(GL_VERSION));
	glVersionView->SetExplicitAlignment(kLabelAlignment);

	BString gluString("GLU ");
	gluString << (const char*)gluGetString(GLU_VERSION);
	BStringView* gluVersionView = new BStringView(NULL, gluString.String());
	gluVersionView->SetExplicitAlignment(kLabelAlignment);

	BString glutAPIString("GLUT API ");
	glutAPIString << (int32)GLUT_API_VERSION;
	BStringView* glutVersionView = new BStringView(NULL,
		glutAPIString.String());
	glutVersionView->SetExplicitAlignment(kLabelAlignment);

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 0)
			.Add(rendererView)
			.Add(BGridLayoutBuilder(0, 0)
				.Add(vendorNameView, 0, 0)
				.Add(glVersionView, 1, 0)
				.Add(gluVersionView, 0, 1)
				.Add(glutVersionView, 1, 1)
			)
			.End();
}


InfoView::~InfoView()
{
}
