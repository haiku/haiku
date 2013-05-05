
/*-
 * Copyright (c) 2009-2010 Alexander Egorenkov <egorenar@gmail.com>
 * Copyright (c) 2009 Damien Bergamini <damien.bergamini@free.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <dev/rt2860/rt2860_softc.h>
#include <dev/rt2860/rt2860_reg.h>
#include <dev/rt2860/rt2860_eeprom.h>
#include <dev/rt2860/rt2860_ucode.h>
#include <dev/rt2860/rt2860_txwi.h>
#include <dev/rt2860/rt2860_rxwi.h>
#include <dev/rt2860/rt2860_io.h>
#include <dev/rt2860/rt2860_read_eeprom.h>
#include <dev/rt2860/rt2860_led.h>
#include <dev/rt2860/rt2860_rf.h>
#include <dev/rt2860/rt2860_debug.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

/*
 * Defines and macros
 */

#define PCI_VENDOR_RALINK						0x1814
#define PCI_PRODUCT_RALINK_RT2860_PCI			0x0601
#define PCI_PRODUCT_RALINK_RT2860_PCIe			0x0681
#define PCI_PRODUCT_RALINK_RT2760_PCI			0x0701
#define PCI_PRODUCT_RALINK_RT2790_PCIe			0x0781
#define PCI_PRODUCT_RALINK_RT3090_PCIe			0x3090

/*
 * Data structures and types
 */

struct rt2860_pci_ident
{
	uint16_t vendor;
	uint16_t device;
	const char *name;
};

/*
 * Static function prototypes
 */

static int rt2860_pci_probe(device_t dev);

static int rt2860_pci_attach(device_t dev);

/*
 * Bus independed methods
 */

int rt2860_attach(device_t dev);

int rt2860_detach(device_t dev);

int rt2860_shutdown(device_t dev);

int rt2860_suspend(device_t dev);

int rt2860_resume(device_t dev);


/*
 * Static variables
 */

static const struct rt2860_pci_ident rt2860_pci_ids[] =
{
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2860_PCI, "Ralink RT2860 PCI" },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2860_PCIe, "Ralink RT2860 PCIe" },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2760_PCI, "Ralink RT2760 PCI" },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2790_PCIe, "Ralink RT2790 PCIe" },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT2790_PCIe, "Ralink RT2790 PCIe" },
	{ PCI_VENDOR_RALINK, PCI_PRODUCT_RALINK_RT3090_PCIe, "Ralink RT3090 PCIe" },
	{ 0, 0, NULL }
};


/*
 * rt2860_pci_probe
 */
static int rt2860_pci_probe(device_t dev)
{
	const struct rt2860_pci_ident *ident;

	for (ident = rt2860_pci_ids; ident->name != NULL; ident++)
	{
		if (pci_get_vendor(dev) == ident->vendor &&
			pci_get_device(dev) == ident->device)
		{
			device_set_desc(dev, ident->name);
			return 0;
		}
	}

	return ENXIO;
}

/*
 * rt2860_pci_attach
 */
static int rt2860_pci_attach(device_t dev)
{
	struct rt2860_softc *sc;

	sc = device_get_softc(dev);

	if (pci_get_powerstate(dev) != PCI_POWERSTATE_D0)
	{
		printf("%s: chip is in D%d power mode, setting to D0\n",
			device_get_nameunit(dev), pci_get_powerstate(dev));
		pci_set_powerstate(dev, PCI_POWERSTATE_D0);
	}

	/* enable bus-mastering */
	pci_enable_busmaster(dev);
	sc->mem_rid = PCIR_BAR(0);

	return (rt2860_attach(dev));
}

static device_method_t rt2860_pci_dev_methods[] =
{
	/* PCI only */
	DEVMETHOD(device_probe, rt2860_pci_probe),
	DEVMETHOD(device_attach, rt2860_pci_attach),

	/* Any bus */
	DEVMETHOD(device_detach, rt2860_detach),
	DEVMETHOD(device_shutdown, rt2860_shutdown),
	DEVMETHOD(device_suspend, rt2860_suspend),
	DEVMETHOD(device_resume, rt2860_resume),
	{ 0, 0 }
};

static driver_t rt2860_pci_driver =
{
	"rt2860",
	rt2860_pci_dev_methods,
	sizeof(struct rt2860_softc)
};

static devclass_t rt2860_pci_dev_class;

DRIVER_MODULE(rt2860, pci, rt2860_pci_driver, rt2860_pci_dev_class, 0, 0);
MODULE_DEPEND(rt2860, pci, 1, 1, 1);
MODULE_DEPEND(rt2860, wlan, 1, 1, 1);

