/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include "OpenGLView.h"

#include <stdio.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <GroupLayout.h>
#include <TabView.h>
#include <SpaceLayoutItem.h>
#include "CapabilitiesView.h"
#include "ExtensionsView.h"
#include "InfoView.h"
#include "LogoView.h"


OpenGLView::OpenGLView()
	: BView("OpenGLView", 0, NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLayout(new BGroupLayout(B_VERTICAL));

	const float kInset = 10;
	BRect dummy(0, 0, 2, 2);

	fGLView = new BGLView(dummy, "gl info", B_FOLLOW_NONE, 0,
		BGL_RGB | BGL_DOUBLE);
	fGLView->Hide();
	AddChild(fGLView);

	fGLView->LockGL();

	LogoView *logoView = new LogoView(dummy);

	BTabView *tabView = new BTabView("tab view", B_WIDTH_FROM_LABEL);

	InfoView *infoView = new InfoView();
	tabView->AddTab(infoView);

	CapabilitiesView *capabilitiesView = new CapabilitiesView();
	tabView->AddTab(capabilitiesView);

	ExtensionsView *extensionsView = new ExtensionsView();
	tabView->AddTab(extensionsView);

	fGLView->UnlockGL();

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(logoView)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(tabView)
			.SetInsets(kInset, kInset, kInset, kInset)
		)
	);
}


OpenGLView::~OpenGLView()
{
}


void
OpenGLView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
	}	
}


void
OpenGLView::AttachedToWindow()
{
	
}


void
OpenGLView::DetachedFromWindow()
{
		
}
