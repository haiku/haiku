/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include "ExtensionsView.h"

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Message.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <GL/gl.h>
#include "ExtensionsList.h"


ExtensionsView::ExtensionsView()
	: BView("Extensions", 0, NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLayout(new BGroupLayout(B_VERTICAL));

	const float kInset = 10;

	ExtensionsList *extList = new ExtensionsList();
	
	char *list = (char*)glGetString(GL_EXTENSIONS);
	if (list) {
		while (*list) {
			char extName[255];
			int n = strcspn(list, " ");
			strncpy(extName, list, n);
			extName[n] = 0;
			extList->AddRow(new ExtensionRow(extName));
			if (!list[n])
				break;
			list += (n + 1); // next !
		}
	}

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
