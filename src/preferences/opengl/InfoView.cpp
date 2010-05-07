/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include "InfoView.h"

#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <String.h>
#include <StringView.h>
#include <SpaceLayoutItem.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>


#define TR_CONTEXT "InfoView"


InfoView::InfoView()
	: BView(B_TRANSLATE("Information"), 0, NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLayout(new BGroupLayout(B_VERTICAL));

	BRect dummyRect(0, 0, 0, 0);

	BStringView *version = new BStringView(dummyRect, "Version",
		B_TRANSLATE("OpenGL version:"), B_FOLLOW_NONE);
	BStringView *versionValue = new BStringView(dummyRect, "VersionVal",
		(const char*)glGetString(GL_VERSION), B_FOLLOW_NONE);
	BStringView *vendor = new BStringView(dummyRect, "Vendor",
		B_TRANSLATE("Vendor name:"), B_FOLLOW_NONE);
	BStringView *vendorValue = new BStringView(dummyRect, "VendorVal",
		(const char*)glGetString(GL_VENDOR), B_FOLLOW_NONE);
	BStringView *renderer = new BStringView(dummyRect, "Renderer",
		B_TRANSLATE("Renderer name:"), B_FOLLOW_NONE);
	BStringView *rendererValue = new BStringView(dummyRect, "RendererVal",
		(const char*)glGetString(GL_RENDERER), B_FOLLOW_NONE);
	BStringView *gluVersion = new BStringView(dummyRect, "GLUVersion",
		B_TRANSLATE("GLU version:"), B_FOLLOW_NONE);
	BStringView *gluVersionValue = new BStringView(dummyRect, "GLUVersionVal",
		(const char*)gluGetString(GLU_VERSION), B_FOLLOW_NONE);
	BStringView *glutVersion = new BStringView(dummyRect, "GLUTVersion",
		B_TRANSLATE("GLUT API version:"), B_FOLLOW_NONE);
	BString glutApiVer;
	glutApiVer << (int32)GLUT_API_VERSION;
	BStringView *glutVersionValue = new BStringView(dummyRect, "GLUTVersionVal",
		glutApiVer.String(), B_FOLLOW_NONE);

	const float kInset = 10;
	const float kSpacer = 5;

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(version)
			.Add(BSpaceLayoutItem::CreateGlue())
			.Add(versionValue)
		)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(kSpacer))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(vendor)
			.Add(BSpaceLayoutItem::CreateGlue())
			.Add(vendorValue)
		)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(kSpacer))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(renderer)
			.Add(BSpaceLayoutItem::CreateGlue())
			.Add(rendererValue)
		)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(kSpacer))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(gluVersion)
			.Add(BSpaceLayoutItem::CreateGlue())
			.Add(gluVersionValue)
		)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(kSpacer))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(glutVersion)
			.Add(BSpaceLayoutItem::CreateGlue())
			.Add(glutVersionValue)
		)
		.Add(BSpaceLayoutItem::CreateGlue())
		.SetInsets(kInset, kInset, kInset, kInset)
	);
}


InfoView::~InfoView()
{

}


void
InfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
	}
}


void
InfoView::AttachedToWindow()
{

}


void
InfoView::DetachedFromWindow()
{

}
