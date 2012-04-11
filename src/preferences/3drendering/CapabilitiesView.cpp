/*
 * Copyright 2009-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alex Wilson <yourpalal2@gmail.com>
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include "CapabilitiesView.h"

#include <stdio.h>

#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <String.h>
#include <StringView.h>
#include <GL/glu.h>
#include <GL/glut.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Capabilities"


const BAlignment kNameAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET);
const BAlignment kValueAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET);


CapabilitiesView::CapabilitiesView()
	:
	BGridView(B_TRANSLATE("Capabilities"))
{
	_AddCapability(GL_AUX_BUFFERS, B_TRANSLATE("Auxiliary buffer(s):"));

	_AddCapability(GL_MAX_MODELVIEW_STACK_DEPTH,
		B_TRANSLATE("Model stack size:"));

	_AddCapability(GL_MAX_PROJECTION_STACK_DEPTH,
		B_TRANSLATE("Projection stack size:"));

	_AddCapability(GL_MAX_TEXTURE_STACK_DEPTH, 
		B_TRANSLATE("Texture stack size:"));

	_AddCapability(GL_MAX_NAME_STACK_DEPTH, B_TRANSLATE("Name stack size:"));

	_AddCapability(GL_MAX_LIST_NESTING, B_TRANSLATE("List stack size:"));

	_AddCapability(GL_MAX_ATTRIB_STACK_DEPTH,
		B_TRANSLATE("Attributes stack size:"));

	_AddCapability(GL_MAX_TEXTURE_SIZE, B_TRANSLATE("Max. 2D texture size:"));

	_AddCapability(GL_MAX_3D_TEXTURE_SIZE,
		B_TRANSLATE("Max. 3D texture size:"));

	_AddCapability(GL_MAX_TEXTURE_UNITS_ARB,
		B_TRANSLATE("Max. texture units:"));

	_AddCapability(GL_MAX_LIGHTS, B_TRANSLATE("Max. lights:"));

	_AddCapability(GL_MAX_CLIP_PLANES, B_TRANSLATE("Max. clipping planes:"));

	_AddCapability(GL_MAX_EVAL_ORDER,
		B_TRANSLATE("Max. evaluators equation order:"));

	_AddConvolutionCapability();

	_AddCapability(GL_MAX_ELEMENTS_INDICES,
		B_TRANSLATE("Max. recommended index elements:"));

	_AddCapability(GL_MAX_ELEMENTS_VERTICES,
		B_TRANSLATE("Max. recommended vertex elements:"));

	BGridLayout* layout = GridLayout();
	layout->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	layout->AddItem(BSpaceLayoutItem::CreateGlue(), 0, layout->CountRows(),
		layout->CountColumns(), 1);

	// Set horizontal spacing to 0, and use the middle column as
	// variable-width spacing (like layout 'glue').
	layout->SetHorizontalSpacing(0);
	layout->SetMinColumnWidth(1, be_control_look->DefaultLabelSpacing());
	layout->SetMaxColumnWidth(1, B_SIZE_UNLIMITED);
}


CapabilitiesView::~CapabilitiesView()
{
}


void
CapabilitiesView::_AddCapability(GLenum capability, const char* name)
{
	int value = 0;
	glGetIntegerv(capability, &value);
	_AddCapabilityView(name, BString() << (int32)value);
}


void
CapabilitiesView::_AddCapabilityView(const char* name, const char* value)
{
	BStringView *nameView = new BStringView(NULL, name);
	nameView->SetExplicitAlignment(kNameAlignment);

	BStringView *valueView = new BStringView(NULL, value);
	valueView->SetExplicitAlignment(kValueAlignment);

	// Add the items at the bottom of our grid with a column inbetween
	int32 row = GridLayout()->CountRows();
	BLayoutBuilder::Grid<>(this)
		.Add(nameView, 0, row)
		.Add(valueView, 2, row);
}


void
CapabilitiesView::_AddConvolutionCapability()
{
	int width = 0;
	glGetConvolutionParameteriv(GL_CONVOLUTION_2D,
		GL_MAX_CONVOLUTION_WIDTH, &width);

	int height = 0;
	glGetConvolutionParameteriv(GL_CONVOLUTION_2D,
		GL_MAX_CONVOLUTION_HEIGHT, &height);

	BString convolution;
	convolution << (int32) width << 'x' << (int32) height;
	_AddCapabilityView(B_TRANSLATE("Max. convolution:"), convolution);
}
