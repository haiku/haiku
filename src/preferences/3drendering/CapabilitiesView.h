/*
 * Copyright 2009-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CAPABILITIES_VIEW_H
#define CAPABILITIES_VIEW_H


#include <GridView.h>

#include <GL/gl.h>


class CapabilitiesView : public BGridView {
public:
								CapabilitiesView();
		virtual					~CapabilitiesView();

private:
				void			_AddCapability(GLenum capability,
									const char* name);
				void			_AddCapabilityView(const char* name,
									const char* value);
				void			_AddConvolutionCapability();
};


#endif	/* CAPABILITIES_VIEW_H */
