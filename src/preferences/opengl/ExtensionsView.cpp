/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */
#include "ExtensionsView.h"
#include "ExtensionsList.h"

#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <GL/gl.h>
#include <GL/glu.h>


#define TR_CONTEXT "Extensions"


ExtensionsView::ExtensionsView()
	: BView(B_TRANSLATE("Extensions"), 0, NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLayout(new BGroupLayout(B_VERTICAL));

	const float kInset = 10;

	ExtensionsList *extList = new ExtensionsList();

	_AddExtensionsList(extList, (char*) glGetString(GL_EXTENSIONS));
	_AddExtensionsList(extList, (char*) gluGetString(GLU_EXTENSIONS));

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(extList)
		.SetInsets(kInset, kInset, kInset, kInset)
	);
}


ExtensionsView::~ExtensionsView()
{

}


void
ExtensionsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
	}
}


void
ExtensionsView::AttachedToWindow()
{

}


void
ExtensionsView::DetachedFromWindow()
{

}

// #pragma mark

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
		stringList += (n + 1); // next !
	}
}

