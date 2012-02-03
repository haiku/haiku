/*
 * Copyright 2009-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Alex Wilson <yourpalal2@gmail.com>
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include "OpenGLView.h"

#include <stdio.h>

#include <Catalog.h>
#include <GLView.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>

#include "CapabilitiesView.h"
#include "ExtensionsView.h"
#include "InfoView.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "OpenGLView"


OpenGLView::OpenGLView()
	:
	BGroupView("OpenGLView", B_VERTICAL)
{

	BGLView* glView = new BGLView(BRect(0, 0, 1, 1), "gl info", B_FOLLOW_NONE, 0,
		BGL_RGB | BGL_DOUBLE);
	glView->Hide();
	AddChild(glView);

	glView->LockGL();

    BMenu* menu = new BMenu(B_TRANSLATE("Automatic"));
    menu->SetRadioMode(true);
    menu->SetLabelFromMarked(true);
    menu->AddItem(new BMenuItem(B_TRANSLATE("Automatic"),
        new BMessage(MENU_AUTO_MESSAGE)));
    menu->AddSeparatorItem();
    menu->AddItem(new BMenuItem(B_TRANSLATE("Software Rasterizer"),
        new BMessage(MENU_SWRAST_MESSAGE)));
    menu->AddItem(new BMenuItem(B_TRANSLATE("Gallium Software Pipe"),
        new BMessage(MENU_SWPIPE_MESSAGE)));
    menu->AddItem(new BMenuItem(B_TRANSLATE("Gallium LLVM Pipe"),
        new BMessage(MENU_SWLLVM_MESSAGE)));
    BMenuField* menuField = new BMenuField("renderer",
        B_TRANSLATE("3D Rendering Engine:"), menu);
	// TODO:  Set current Renderer
	menuField->SetEnabled(false);

	BTabView *tabView = new BTabView("tab view", B_WIDTH_FROM_LABEL);
	tabView->AddTab(new InfoView());
	tabView->AddTab(new CapabilitiesView());
	tabView->AddTab(new ExtensionsView());

	glView->UnlockGL();

	GroupLayout()->SetSpacing(0);
	BLayoutBuilder::Group<>(this)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(menuField)
		.AddGroup(B_HORIZONTAL)
			.Add(tabView)
			.SetInsets(0, B_USE_DEFAULT_SPACING, 0, 0);
}


OpenGLView::~OpenGLView()
{
}

