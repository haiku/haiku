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

#include <Box.h>
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


InfoView::InfoView()
	:
	BGroupView(B_TRANSLATE("Information"), B_HORIZONTAL)
{
	const char* glRenderer = (const char*)glGetString(GL_RENDERER);
	if (glRenderer == NULL)
		glRenderer = B_TRANSLATE("Unknown or missing OpenGL renderer.");
	BStringView* rendererView = new BStringView(NULL, glRenderer);
	rendererView->SetExplicitAlignment(kLabelAlignment);
	rendererView->SetFont(be_bold_font);

	const char* glVendor = (const char*)glGetString(GL_VENDOR);
	if (glVendor == NULL)
		glVendor = B_TRANSLATE("Unknown vendor");
	BStringView* vendorNameView = new BStringView(NULL, glVendor);
	vendorNameView->SetExplicitAlignment(kLabelAlignment);

	const char* glVersion = (const char*)glGetString(GL_VERSION);
	if (glVersion == NULL)
		glVersion = B_TRANSLATE("Unknown version");
	BStringView* glVersionView = new BStringView(NULL, glVersion);
	glVersionView->SetExplicitAlignment(kLabelAlignment);

	BString apiString("GLU ");
	apiString << (const char*)gluGetString(GLU_VERSION);
	apiString << ", GLUT ";
	apiString << (int32)GLUT_API_VERSION;
	BStringView* apiVersionView = new BStringView(NULL, apiString.String());
	apiVersionView->SetExplicitAlignment(kLabelAlignment);

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 0)
			.Add(rendererView)
			.Add(vendorNameView)
			.Add(glVersionView)
			.Add(apiVersionView)
			.End();
}


InfoView::~InfoView()
{
}
