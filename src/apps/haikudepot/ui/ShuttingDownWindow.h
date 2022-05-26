/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SHUTTING_DOWN_WINDOW_H
#define SHUTTING_DOWN_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <Window.h>

#include "BarberPole.h"
#include "HaikuDepotConstants.h"
#include "UserDetail.h"
#include "UserUsageConditions.h"


class BButton;
class BCheckBox;
class Model;


class ShuttingDownWindow : public BWindow {
public:
								ShuttingDownWindow(BWindow* parent);
	virtual						~ShuttingDownWindow();
};


#endif // SHUTTING_DOWN_WINDOW_H
