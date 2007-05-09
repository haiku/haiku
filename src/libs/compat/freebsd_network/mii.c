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


driver_t miibus_driver = {
	"mii",
};


int
miibus_readreg(device_t dev, int phy, int reg)
{
	if (dev->methods.miibus_readreg == NULL) {
		if (dev->parent == NULL)
			panic("miibus_readreg, no support");
		return miibus_readreg(dev->parent, phy, reg);
	}

	return dev->methods.miibus_readreg(dev, phy, reg);
}


int
miibus_writereg(device_t dev, int phy, int reg, int data)
{
	if (dev->methods.miibus_writereg == NULL) {
		if (dev->parent == NULL)
			panic("miibus_writereg, no support");
		return miibus_writereg(dev->parent, phy, reg, data);
	}

	return dev->methods.miibus_writereg(dev, phy, reg, data);
}


int
mii_phy_probe(device_t dev, device_t *mii_dev, ifm_change_cb_t change,
	ifm_stat_cb_t stat)
{
	UNIMPLEMENTED();
	return -1;
}


void
mii_tick(struct mii_data *data)
{
	UNIMPLEMENTED();
}

int
mii_mediachg(struct mii_data *data)
{
	UNIMPLEMENTED();
	return -1;
}


void
mii_pollstat(struct mii_data *data)
{
	UNIMPLEMENTED();
}
