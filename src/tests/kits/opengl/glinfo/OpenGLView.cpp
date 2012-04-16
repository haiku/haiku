/*
 * Copyright 2009-2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
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
#include <PopUpMenu.h>
#include <Size.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>

#include "CapabilitiesView.h"
#include "ExtensionsView.h"
#include "InfoView.h"
#include "GearsView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "OpenGLView"


OpenGLView::OpenGLView()
	:
	BGroupView("OpenGLView", B_VERTICAL)
{

	BGLView* glView = new BGLView(BRect(0, 0, 1, 1), "gl info", B_FOLLOW_NONE, 0,
		BGL_RGB | BGL_DOUBLE);
	glView->Hide();
	AddChild(glView);

	glView->LockGL();

	BPopUpMenu* menu = new BPopUpMenu(B_TRANSLATE("Automatic"), true, true);
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
	menuField->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	// TODO:  Set current Renderer
	menuField->SetEnabled(false);

	float tabViewWidth = this->StringWidth("M") * 36;
	float tabViewHeight = this->StringWidth("M") * 16;

	BTabView *tabView = new BTabView("tab view", B_WIDTH_FROM_LABEL);
	tabView->SetExplicitMinSize(BSize(tabViewWidth, tabViewHeight));
	tabView->AddTab(new CapabilitiesView());
	tabView->AddTab(new ExtensionsView());

	glView->UnlockGL();

	GroupLayout()->SetSpacing(0);
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(new GearsView())
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
				.SetInsets(0, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
				.Add(menuField)
				.Add(new InfoView())
				.Add(tabView)
				.End()
			.AddGlue()
			.End();
}

OpenGLView::~OpenGLView()
{
}
