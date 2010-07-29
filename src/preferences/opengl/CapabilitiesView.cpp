/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include "CapabilitiesView.h"

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


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Capabilities"


void
AddStringView(BView* view, const char* viewName, const char* name,
	const char* value, bool spacer)
{
	if (!view)
		return;

	BRect dummyRect(0, 0, 0, 0);
	const float kSpacer = 5;

	BString tempViewName(viewName);
	BStringView *nameView = new BStringView(dummyRect, tempViewName.String(),
		name, B_FOLLOW_NONE);
	tempViewName.Append("Value");
	BStringView *valueView = new BStringView(dummyRect, tempViewName.String(),
		value, B_FOLLOW_NONE);

	if (spacer) {
		view->AddChild(BGroupLayoutBuilder(B_VERTICAL)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL)
				.Add(nameView)
				.Add(BSpaceLayoutItem::CreateGlue())
				.Add(valueView)
			)
			.Add(BSpaceLayoutItem::CreateVerticalStrut(kSpacer))
		);
	} else {
		view->AddChild(BGroupLayoutBuilder(B_VERTICAL)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL)
				.Add(nameView)
				.Add(BSpaceLayoutItem::CreateGlue())
				.Add(valueView)
			)
		);
	}
}


CapabilitiesView::CapabilitiesView()
	: BView(B_TRANSLATE("Capabilities"), 0, NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLayout(new BGroupLayout(B_VERTICAL));

	const float kInset = 10;

	int lights = 0;
	int clippingPlanes = 0;
	int modelStack = 0;
	int projectionStack = 0;
	int textureStack = 0;
	int maxTex3d = 0;
	int maxTex2d = 0;
	int nameStack = 0;
	int listStack = 0;
	int maxPoly = 0;
	int attribStack = 0;
	int buffers = 0;
	int convolutionWidth = 0;
	int convolutionHeight = 0;
	int maxIndex = 0;
	int maxVertex = 0;
	int textureUnits = 0;

	glGetIntegerv(GL_MAX_LIGHTS, &lights);
	glGetIntegerv(GL_MAX_CLIP_PLANES, &clippingPlanes);
	glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &modelStack);
	glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &projectionStack);
	glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH, &textureStack);
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxTex3d);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex2d);
	glGetIntegerv(GL_MAX_NAME_STACK_DEPTH, &nameStack);
	glGetIntegerv(GL_MAX_LIST_NESTING, &listStack);
	glGetIntegerv(GL_MAX_EVAL_ORDER, &maxPoly);
	glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH, &attribStack);
	glGetIntegerv(GL_AUX_BUFFERS, &buffers);
	glGetIntegerv(GL_MAX_CONVOLUTION_WIDTH, &convolutionWidth);
	glGetIntegerv(GL_MAX_CONVOLUTION_HEIGHT, &convolutionHeight);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &maxIndex);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &maxVertex);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &textureUnits);

	BString tempString;
	BView *rootView = new BView("root view", 0, NULL);
	rootView->SetLayout(new BGroupLayout(B_VERTICAL));

	tempString << (int32) buffers;
	AddStringView(rootView, "Buffers", B_TRANSLATE("Auxiliary buffer(s):"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) modelStack;
	AddStringView(rootView, "ModelStack", B_TRANSLATE("Model stack size:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) projectionStack;
	AddStringView(rootView, "ProjectionStack",
		B_TRANSLATE("Projection stack size:"), tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) textureStack;
	AddStringView(rootView, "TextureStack", B_TRANSLATE("Texture stack size:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) nameStack;
	AddStringView(rootView, "NameStack", B_TRANSLATE("Name stack size:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) listStack;
	AddStringView(rootView, "ListStack", B_TRANSLATE("List stack size:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) attribStack;
	AddStringView(rootView, "AttribStack", B_TRANSLATE("Attributes stack size:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) maxTex3d;
	AddStringView(rootView, "MaxTex3D", B_TRANSLATE("Max. 3D texture size:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) maxTex2d;
	AddStringView(rootView, "MaxTex2D", B_TRANSLATE("Max. 2D texture size:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) textureUnits;
	AddStringView(rootView, "MaxTexUnits", B_TRANSLATE("Max. texture units:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) lights;
	AddStringView(rootView, "MaxLights", B_TRANSLATE("Max. lights:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) clippingPlanes;
	AddStringView(rootView, "MaxClippingPlanes",
		B_TRANSLATE("Max. clipping planes:"), tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) maxPoly;
	AddStringView(rootView, "MaxPoly",
		B_TRANSLATE("Max. evaluators equation order:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) convolutionWidth << "x" << convolutionHeight;
	AddStringView(rootView, "MaxConvultion", B_TRANSLATE("Max. convolution:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) maxIndex;
	AddStringView(rootView, "MaxIndex",
		B_TRANSLATE("Max. recommended index elements:"),
		tempString.String(), true);

	tempString.SetTo("");
	tempString << (int32) maxVertex;
	AddStringView(rootView, "MaxVertex",
		B_TRANSLATE("Max. recommended vertex elements:"),
		tempString.String(), true);

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(rootView)
		.SetInsets(kInset, kInset, kInset, kInset)
	);
}


CapabilitiesView::~CapabilitiesView()
{

}


void
CapabilitiesView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
	}
}


void
CapabilitiesView::AttachedToWindow()
{

}


void
CapabilitiesView::DetachedFromWindow()
{

}
