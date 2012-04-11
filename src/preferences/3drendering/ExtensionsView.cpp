/*
 * Copyright 2009-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alex Wilson <yourpalal2@gmail.com>
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include "ExtensionsView.h"

#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "ExtensionsList.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Extensions"


ExtensionsView::ExtensionsView()
	:
	BGroupView(B_TRANSLATE("Extensions"), B_VERTICAL)
{
	ExtensionsList *extList = new ExtensionsList();
	_AddExtensionsList(extList, (char*) glGetString(GL_EXTENSIONS));
	_AddExtensionsList(extList, (char*) gluGetString(GLU_EXTENSIONS));

	AddChild(extList);
	GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
}


ExtensionsView::~ExtensionsView()
{
}


//	#pragma mark -


void
ExtensionsView::_AddExtensionsList(ExtensionsList *extList, char* stringList)
{
	if (!stringList)
		// empty extentions string
		return;

	while (*stringList) {
		char extName[255];
		int n = strcspn(stringList, " ");
		strncpy(extName, stringList, n);
		extName[n] = 0;
		extList->AddRow(new ExtensionRow(extName));
		if (!stringList[n])
			break;
		stringList += (n + 1);
			// next !
	}
}
