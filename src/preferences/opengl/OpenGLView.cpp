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

#include <GLView.h>
#include <LayoutBuilder.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>

#include "CapabilitiesView.h"
#include "ExtensionsView.h"
#include "InfoView.h"
#include "LogoView.h"


OpenGLView::OpenGLView()
	:
	BGroupView("OpenGLView", B_VERTICAL)
{

	BGLView* glView = new BGLView(BRect(0, 0, 1, 1), "gl info", B_FOLLOW_NONE, 0,
		BGL_RGB | BGL_DOUBLE);
	glView->Hide();
	AddChild(glView);

	glView->LockGL();

	LogoView *logoView = new LogoView();

	BTabView *tabView = new BTabView("tab view", B_WIDTH_FROM_LABEL);
	tabView->AddTab(new InfoView());
	tabView->AddTab(new CapabilitiesView());
	tabView->AddTab(new ExtensionsView());

	glView->UnlockGL();

	GroupLayout()->SetSpacing(0);
	BLayoutBuilder::Group<>(this)
		.SetInsets(0, 0, 0, 0)
		.Add(logoView)
		.AddGroup(B_HORIZONTAL)
			.Add(tabView)
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
}


OpenGLView::~OpenGLView()
{
}

