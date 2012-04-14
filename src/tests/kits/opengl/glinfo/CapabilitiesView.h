/*
 * Copyright 2009-2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CAPABILITIES_VIEW_H
#define CAPABILITIES_VIEW_H


#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <GL/gl.h>
#include <GroupView.h>


class CapabilitiesView : public BGroupView {
public:
								CapabilitiesView();
		virtual					~CapabilitiesView();

private:
				BRow*			_CreateCapabilitiesRow(GLenum capability,
									const char* name);
				BRow*			_CreateConvolutionCapabilitiesRow();

				BColumnListView* fCapabilitiesList;
				BStringColumn*	fCapabilityColumn;
				BStringColumn*	fValueColumn;
};


#endif	/* CAPABILITIES_VIEW_H */
