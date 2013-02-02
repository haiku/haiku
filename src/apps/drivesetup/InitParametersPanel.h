/*
 * Copyright 2008-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 */
#ifndef INIT_PARAMETERS_PANEL_H
#define INIT_PARAMETERS_PANEL_H


#include "AbstractParametersPanel.h"


class InitParametersPanel : public AbstractParametersPanel {
public:
								InitParametersPanel(BWindow* window,
									const BString& diskSystem,
									BPartition* partition);
	virtual						~InitParametersPanel();

			status_t			Go(BString& name, BString& parameters);
};


#endif // INIT_PARAMETERS_PANEL_H
