/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Siarzhuk Zharski <imker@gmx.li>
 */


#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <machine/bus.h>
#include <shared.h>

#include "if_dcreg.h"

HAIKU_FBSD_DRIVERS_GLUE(dec21xxx);

HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_FAST_TASKQUEUE | FBSD_SWI_TASKQUEUE);

extern driver_t *DRIVER_MODULE_NAME(dc, pci);
extern driver_t *DRIVER_MODULE_NAME(de, pci);

status_t __haiku_handle_fbsd_drivers_list(status_t (*handler)(driver_t *[]))
{
	driver_t *drivers[] = { 
		DRIVER_MODULE_NAME(dc, pci),
		DRIVER_MODULE_NAME(de, pci),
		NULL
	};
	return (*handler)(drivers);
}

extern driver_t *DRIVER_MODULE_NAME(acphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(amphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(dcphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(pnphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);

driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(acphy, miibus),
		DRIVER_MODULE_NAME(amphy, miibus),
		DRIVER_MODULE_NAME(dcphy, miibus),
		DRIVER_MODULE_NAME(pnphy, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}


int check_disable_interrupts_dc(device_t dev);
void reenable_interrupts_dc(device_t dev);

extern int check_disable_interrupts_de(device_t dev);
extern void reenable_interrupts_de(device_t dev);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	uint16 name = *(uint16*)dev->device_name;
	switch(name) {
		case 'cd':
			return check_disable_interrupts_dc(dev);
		case 'ed':
			return check_disable_interrupts_de(dev);
		default:
			break;
	}

	panic("Unsupported device: %#x (%s)!", name, dev->device_name);
	return 0;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	uint16 name = *(uint16*)dev->device_name;
	switch(name) {
		case 'cd':
			reenable_interrupts_dc(dev);
			break;
		case 'ed':
			reenable_interrupts_de(dev);
			break;
		default:
			panic("Unsupported device: %#x (%s)!", name, dev->device_name);
			break;
	}
}


int check_disable_interrupts_dc(device_t dev)
{
	struct dc_softc *sc = device_get_softc(dev);
	uint16_t status;
	HAIKU_INTR_REGISTER_STATE;

	HAIKU_INTR_REGISTER_ENTER();

	status = CSR_READ_4(sc, DC_ISR);
	if (status == 0xffff) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	if (status != 0 && (status & DC_INTRS) == 0) {
		CSR_WRITE_4(sc, DC_ISR, status);
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	if ((status & DC_INTRS) == 0) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	CSR_WRITE_4(sc, DC_IMR, 0);

	HAIKU_INTR_REGISTER_LEAVE();
	
	return 1;
}


void reenable_interrupts_dc(device_t dev)
{
	struct dc_softc *sc = device_get_softc(dev);
	DC_LOCK(sc);
	CSR_WRITE_4(sc, DC_IMR, DC_INTRS);
	DC_UNLOCK(sc);
}

