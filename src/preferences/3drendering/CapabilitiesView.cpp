/*
 * Copyright 2009-2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione <jscipione@gmail.com>
 *		Alex Wilson <yourpalal2@gmail.com>
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include "CapabilitiesView.h"

#include <stdio.h>

#include <Catalog.h>
#include <ControlLook.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <String.h>
#include <StringView.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Capabilities"


CapabilitiesView::CapabilitiesView()
	:
	BGroupView(B_TRANSLATE("Capabilities"), B_VERTICAL),
	fCapabilitiesList(new BColumnListView("CapabilitiesList", 0))
{
	// add the columns

	fCapabilityColumn = new BStringColumn(B_TRANSLATE("Capability"),
		240, 240, 240, B_TRUNCATE_MIDDLE);
	fCapabilitiesList->AddColumn(fCapabilityColumn, 0);
	fCapabilitiesList->SetSortingEnabled(true);
	fCapabilitiesList->SetSortColumn(fCapabilityColumn, true, true);

	fValueColumn = new BStringColumn(B_TRANSLATE("Value"), 60, 60, 60,
			B_TRUNCATE_MIDDLE);
	fCapabilitiesList->AddColumn(fValueColumn, 1);

	// add the rows

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_AUX_BUFFERS,
		"Auxiliary buffer(s)"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(
		GL_MAX_MODELVIEW_STACK_DEPTH, "Model stack size"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(
		GL_MAX_PROJECTION_STACK_DEPTH, "Projection stack size"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(
		GL_MAX_TEXTURE_STACK_DEPTH, "Texture stack size"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(
		GL_MAX_NAME_STACK_DEPTH, "Name stack size"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_MAX_LIST_NESTING,
		"List stack size"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(
		GL_MAX_ATTRIB_STACK_DEPTH, "Attributes stack size"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_MAX_TEXTURE_SIZE,
		"Maximum 2D texture size"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_MAX_3D_TEXTURE_SIZE,
		"Maximum 3D texture size"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_MAX_TEXTURE_UNITS_ARB,
		"Maximum texture units"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_MAX_LIGHTS,
		"Maximum lights"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_MAX_CLIP_PLANES,
		"Maximum clipping planes"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_MAX_EVAL_ORDER,
		"Maximum evaluators equation order"));

	fCapabilitiesList->AddRow(_CreateConvolutionCapabilitiesRow());

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_MAX_ELEMENTS_INDICES,
		"Max. recommended index elements"));

	fCapabilitiesList->AddRow(_CreateCapabilitiesRow(GL_MAX_ELEMENTS_VERTICES,
		"Max. recommended vertex elements"));

	// add the list

	AddChild(fCapabilitiesList);
	GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
}


CapabilitiesView::~CapabilitiesView()
{
	BRow *row;
	while ((row = fCapabilitiesList->RowAt((int32)0, NULL)) != NULL) {
		fCapabilitiesList->RemoveRow(row);
		delete row;
	}
	delete fCapabilityColumn;
	delete fValueColumn;
	delete fCapabilitiesList;
}


//	#pragma mark -


BRow*
CapabilitiesView::_CreateCapabilitiesRow(GLenum capability, const char* name)
{
	BRow* row = new BRow();
	row->SetField(new BStringField(B_TRANSLATE(name)), 0);

	int value = 0;
	glGetIntegerv(capability, &value);
	row->SetField(new BStringField(BString() << (int32)value), 1);

	return row;
}


BRow*
CapabilitiesView::_CreateConvolutionCapabilitiesRow()
{
	BRow* row = new BRow();
	row->SetField(new BStringField(B_TRANSLATE("Maximum convolution")), 0);

	int width = 0;
	glGetConvolutionParameteriv(GL_CONVOLUTION_2D,
		GL_MAX_CONVOLUTION_WIDTH, &width);

	int height = 0;
	glGetConvolutionParameteriv(GL_CONVOLUTION_2D,
		GL_MAX_CONVOLUTION_HEIGHT, &height);

	BString convolution;
	convolution << (int32)width << 'x' << (int32)height;
	row->SetField(new BStringField(convolution), 1);

	return row;
}
