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
__haiku_miibus_readreg(device_t dev, int phy, int reg)
{
	if (dev->methods.miibus_readreg == NULL)
		panic("miibus_readreg, no support");

	return dev->methods.miibus_readreg(dev, phy, reg);
}


int
__haiku_miibus_writereg(device_t dev, int phy, int reg, int data)
{
	if (dev->methods.miibus_writereg == NULL)
		panic("miibus_writereg, no support");

	return dev->methods.miibus_writereg(dev, phy, reg, data);
}


void
__haiku_miibus_statchg(device_t dev)
{
	if (dev->methods.miibus_statchg)
		dev->methods.miibus_statchg(dev);
}


void
__haiku_miibus_linkchg(device_t dev)
{
	if (dev->methods.miibus_linkchg)
		dev->methods.miibus_linkchg(dev);
}


void
__haiku_miibus_mediainit(device_t dev)
{
	if (dev->methods.miibus_mediainit)
		dev->methods.miibus_mediainit(dev);
}
