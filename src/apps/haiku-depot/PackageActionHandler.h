/*
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef PACKAGE_ACTION_HANDLER_H
#define PACkAGE_ACTION_HANDLER_H


#include <SupportDefs.h>

#include "PackageAction.h"


class Model;


class PackageActionHandler {
public:
	virtual						~PackageActionHandler();

	virtual	status_t			SchedulePackageActions(
									PackageActionList& list) = 0;

	virtual	Model*				GetModel() = 0;
};

#endif // PACKAGE_ACTION_HANDLER_H
