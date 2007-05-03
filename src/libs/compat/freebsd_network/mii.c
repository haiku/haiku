/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include "device.h"

#include <compat/sys/bus.h>

#include <compat/net/if_media.h>
#include <compat/dev/mii/miivar.h>


int
mii_phy_probe(device_t dev, device_t *miiDev, ifm_change_cb_t change,
	ifm_stat_cb_t stat)
{
	return -1;
}


void
mii_tick(struct mii_data *data)
{
}

int
mii_mediachg(struct mii_data *data)
{
	return -1;
}


void
mii_pollstat(struct mii_data *data)
{
}
