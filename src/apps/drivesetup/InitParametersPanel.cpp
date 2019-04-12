/*
 * Copyright 2008-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Karsten Heimrich. <host.haiku@gmx.de>
 */


#include "InitParametersPanel.h"

#include <driver_settings.h>
#include <stdio.h>

#include <Button.h>
#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InitializeParametersPanel"


InitParametersPanel::InitParametersPanel(BWindow* window,
	const BString& diskSystem, BPartition* partition)
	:
	AbstractParametersPanel(window)
{
	Init(B_INITIALIZE_PARAMETER_EDITOR, diskSystem, partition);

	fOkButton->SetLabel(B_TRANSLATE("Format"));
}


InitParametersPanel::~InitParametersPanel()
{
}


status_t
InitParametersPanel::Go(BString& name, BString& parameters)
{
	status_t status = AbstractParametersPanel::Go(parameters);
	if (status == B_OK) {
		void* handle = parse_driver_settings_string(parameters.String());
		if (handle != NULL) {
			const char* string = get_driver_parameter(handle, "name",
				NULL, NULL);
			name.SetTo(string);
			delete_driver_settings(handle);
		}
	}

	return status;
}
