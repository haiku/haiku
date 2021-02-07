/*
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef PACKAGE_ACTION_HANDLER_H
#define PACKAGE_ACTION_HANDLER_H


#include <SupportDefs.h>

#include "PackageAction.h"


class Model;


class PackageActionHandler {
public:
	virtual						~PackageActionHandler();

	virtual	status_t			SchedulePackageAction(
									PackageActionRef action) = 0;

	virtual	Model*				GetModel() = 0;
};

#endif // PACKAGE_ACTION_HANDLER_H
