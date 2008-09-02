/*-
 * Copyright (c) 2008 Stanislav Sedov <stas@FreeBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Driver for Attansic Technology Corp. L2 FastEthernet adapter.
 *
 * This driver is heavily based on age(4) Attansic L1 driver by Pyun YongHyeon.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/rman.h>
#include <sys/module.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/taskqueue.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/if_vlan_var.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/bus.h>
#include <machine/in_cksum.h>

#ifdef __HAIKU__
#include <machine/resource.h>
#endif

#include "miibus_if.h"

#include "if_aereg.h"
#include "if_aevar.h"

MODULE_DEPEND(ae, pci, 1, 1, 1);
MODULE_DEPEND(ae, ether, 1, 1, 1);
MODULE_DEPEND(ae, miibus, 1, 1, 1);

/*
 * Devices supported by this driver.
 */
static struct ae_dev {
	uint16_t	vendorid;
	uint16_t	deviceid;
	const char	*name;
} ae_devs[] = {
	{ VENDORID_ATTANSIC, DEVICEID_ATTANSIC_L2,
		"Attansic Technology Corp, L2 FastEthernet" },
};

#define AE_DEVS_COUNT (sizeof(ae_devs) / sizeof(*ae_devs))

static int ae_probe(device_t dev);
static int ae_attach(device_t dev);
static void ae_pcie_init(ae_softc_t *sc);
static void ae_phy_reset(ae_softc_t *sc);
static int ae_reset(ae_softc_t *sc);
static void ae_init(void *arg);
static int ae_init_locked(ae_softc_t *sc);
static unsigned int ae_detach(device_t dev);
static int ae_miibus_readreg(device_t dev, int phy, int reg);
static int ae_miibus_writereg(device_t dev, int phy, int reg, int val);
static void ae_miibus_statchg(device_t dev);
static void ae_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr);
static int ae_mediachange(struct ifnet *ifp);
static void ae_retrieve_address(ae_softc_t *sc);
static void ae_dmamap_cb(void *arg, bus_dma_segment_t *segs, int nsegs, int error);
static int ae_alloc_rings(ae_softc_t *sc);
static void ae_dma_free(ae_softc_t *sc);
static int ae_shutdown(device_t dev);
static int ae_suspend(device_t dev);
static int ae_resume(device_t dev);
static unsigned int ae_tx_avail_size(ae_softc_t *sc);
static int ae_encap(ae_softc_t *sc, struct mbuf **m_head);
static void ae_start(struct ifnet *ifp);
static void ae_link_task(void *arg, int pending);
static void ae_stop_rxmac(ae_softc_t *sc);
static void ae_stop_txmac(ae_softc_t *sc);
static void ae_tx_task(void *arg, int pending);
static void ae_mac_config(ae_softc_t *sc);
static int ae_intr(void *arg);
static void ae_int_task(void *arg, int pending);
static void ae_tx_intr(ae_softc_t *sc);
static int ae_rxeof(ae_softc_t *sc, ae_rxd_t *rxd);
static void ae_rx_intr(ae_softc_t *sc);
static void ae_watchdog(ae_softc_t *sc);
static void ae_tick(void *arg);
static void ae_rxfilter(ae_softc_t *sc);
static int ae_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data);
static void ae_stop(ae_softc_t *sc);
static int ae_check_eeprom_present(ae_softc_t *sc, int *vpdc);
static int ae_vpd_read_word(ae_softc_t *sc, int reg, uint32_t *word);
static int ae_get_vpd_eaddr(ae_softc_t *sc, uint32_t *eaddr);
static int ae_get_reg_eaddr(ae_softc_t *sc, uint32_t *eaddr);

static device_method_t ae_methods[] = {
	/* Device interface. */
	DEVMETHOD(device_probe,		ae_probe),
	DEVMETHOD(device_attach,	ae_attach),
	DEVMETHOD(device_detach,	ae_detach),
	DEVMETHOD(device_shutdown,	ae_shutdown),
	DEVMETHOD(device_suspend,	ae_suspend),
	DEVMETHOD(device_resume,	ae_resume),

	/* MII interface. */
	DEVMETHOD(miibus_readreg,	ae_miibus_readreg),
	DEVMETHOD(miibus_writereg,	ae_miibus_writereg),
	DEVMETHOD(miibus_statchg,	ae_miibus_statchg),

	{ NULL, NULL }
};

static driver_t ae_driver = {
        "ae",
        ae_methods,
        sizeof(ae_softc_t)
};

static devclass_t ae_devclass;

DRIVER_MODULE(ae, pci, ae_driver, ae_devclass, 0, 0);
DRIVER_MODULE(miibus, ae, miibus_driver, miibus_devclass, 0, 0);

static struct resource_spec ae_res_spec_mem[] = {
	{ SYS_RES_MEMORY,       PCIR_BAR(0),    RF_ACTIVE },
	{ -1,			0,		0 }
};

static struct resource_spec ae_res_spec_irq[] = {
	{ SYS_RES_IRQ,		0,		RF_ACTIVE | RF_SHAREABLE },
	{ -1,			0,		0 }
};

#define	AE_READ_4(sc, reg) \
	bus_read_4((sc)->mem[0], (reg))

#define	AE_READ_2(sc, reg) \
	bus_read_2((sc)->mem[0], (reg))

#define	AE_READ_1(sc, reg) \
	bus_read_1((sc)->mem[0], (reg))

#define	AE_WRITE_4(sc, reg, val) \
	bus_write_4((sc)->mem[0], (reg), (val))

#define	AE_WRITE_2(sc, reg, val) \
	bus_write_2((sc)->mem[0], (reg), (val))

#define AE_WRITE_1(sc, reg, val) \
	bus_write_1((sc)->mem[0], (reg), (val))

#define	AE_CHECK_EADDR_VALID(eaddr) \
	((eaddr[0] == 0 && eaddr[1] == 0) || \
	(eaddr[0] == 0xffffffff && eaddr[1] == 0xffff))

static int
ae_probe(device_t dev)
{
	uint16_t vendorid, deviceid;
	int i;

	vendorid = pci_get_vendor(dev);
	deviceid = pci_get_device(dev);

	for (i = 0; i < AE_DEVS_COUNT; i++) {
		if (vendorid == ae_devs[i].vendorid &&
		    deviceid == ae_devs[i].deviceid) {
			device_set_desc(dev, ae_devs[i].name);
			return (BUS_PROBE_DEFAULT);
		}
	}

	return (ENXIO);
}

static int
ae_attach(device_t dev)
{
	ae_softc_t *sc;
	struct ifnet *ifp;
	int error;
	uint8_t chiprev;
	uint32_t pcirev;

	sc = device_get_softc(dev); /* Automatically allocated and zeroed
				       on attach. */

	KASSERT(sc != NULL, ("[ae, %d]: sc is null", __LINE__));
	sc->dev = dev;

	/* Initialize per-device mutex. */
	mtx_init(&sc->mtx, device_get_nameunit(dev), MTX_NETWORK_LOCK, MTX_DEF);

	callout_init_mtx(&sc->tick_ch, &sc->mtx, 0);
	TASK_INIT(&sc->int_task, 0, ae_int_task, sc);
        TASK_INIT(&sc->link_task, 0, ae_link_task, sc);

	/* Enable bus mastering. */
	pci_enable_busmaster(dev);

	sc->spec_mem = ae_res_spec_mem;
	sc->spec_irq = ae_res_spec_irq;

	/* Allocate memory registers. */
	error = bus_alloc_resources(dev, sc->spec_mem, sc->mem);
	if (error != 0) {
		device_printf(dev, "could not allocate memory resources.\n");
		goto fail;
	}

	/* Retrieve PCI and chip revisions. */
	pcirev = pci_get_revid(dev);
	chiprev = (AE_READ_4(sc, AE_MASTER_REG) >> AE_MASTER_REVNUM_SHIFT) &
	    AE_MASTER_REVNUM_MASK;
	if (bootverbose || 1) {
		device_printf(dev, "pci device revision: %#04x\n", pcirev);
		device_printf(dev, "chip id: %#02x\n", chiprev);
	}

	/* Allocate IRQ resources. */
	error = bus_alloc_resources(dev, sc->spec_irq, sc->irq);
	if (error != 0) {
		device_printf(dev, "could not allocate IRQ resources.\n");
		goto fail;
	}

	/* Reset PHY */
	AE_LOCK(sc);
	ae_phy_reset(sc);
	AE_UNLOCK(sc);

	/* Reset the controller. */
	error = ae_reset(sc);
	if (error != 0)
		goto fail;

	/* Initialize PCIE controller. */
	ae_pcie_init(sc);

	/* Load MAC address. */
	ae_retrieve_address(sc);

	/* Set default PHY address. */
	sc->phyaddr = AE_PHYADDR_DEFAULT;

	ifp = sc->ifp = if_alloc(IFT_ETHER);
	if (ifp == NULL) {
		device_printf(dev, "could not allocate ifnet structure.\n");
		error = ENXIO;
	}

	ifp->if_softc = sc;
	if_initname(ifp, device_get_name(dev), device_get_unit(dev));
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = ae_ioctl;
	ifp->if_start = ae_start;
	ifp->if_init = ae_init;
	ifp->if_capabilities = IFCAP_VLAN_MTU;
	ifp->if_capenable = ifp->if_capabilities;
	ifp->if_hwassist = 0;
	ifp->if_snd.ifq_drv_maxlen = IFQ_MAXLEN;
	IFQ_SET_MAXLEN(&ifp->if_snd, ifp->if_snd.ifq_drv_maxlen);
	IFQ_SET_READY(&ifp->if_snd);

	/* Configure and attach MII bus. */
	error = mii_phy_probe(dev, &sc->miibus, ae_mediachange,
	    ae_mediastatus);
	if (error != 0) {
		device_printf(dev, "no PHY found.\n");
		goto fail;
	}

	ether_ifattach(ifp, sc->eaddr);

	TASK_INIT(&sc->tx_task, 1, ae_tx_task, ifp);
	sc->tq = taskqueue_create_fast("ae_taskq", M_WAITOK,
            taskqueue_thread_enqueue, &sc->tq);
	if (sc->tq == NULL) {
		device_printf(dev, "could not create taskqueue.\n");
		ether_ifdetach(ifp);
		error = ENXIO;
		goto fail;
	}

	taskqueue_start_threads(&sc->tq, 1, PI_NET, "%s taskq",
	    device_get_nameunit(sc->dev));

	error = bus_setup_intr(dev, sc->irq[0], INTR_TYPE_NET | INTR_MPSAFE,
	    ae_intr, NULL, sc, &sc->intrhand);
	if (error != 0) {
		device_printf(dev, "could not set up interrupt handler.\n");
		taskqueue_free(sc->tq);
		sc->tq = NULL;
		ether_ifdetach(ifp);
		goto fail;
	}

fail:
	if (error != 0)
		ae_detach(dev);
	
	return (error);
}

static void
ae_pcie_init(ae_softc_t *sc)
{
	AE_WRITE_4(sc, AE_LTSSM_TESTMODE_REG, AE_LTSSM_TESTMODE_DEFAULT);
	AE_WRITE_4(sc, AE_DLL_TX_REG, AE_DLL_TX_DEFAULT);
}

static void
ae_phy_reset(ae_softc_t *sc)
{
	AE_WRITE_4(sc, AE_PHY_ENABLE_REG, AE_PHY_ENABLE);
	DELAY(1000);	/* XXX: pause(9) ? */
}

static int
ae_reset(ae_softc_t *sc)
{
	int i;

	/* Issue a soft reset. */
	AE_WRITE_4(sc, AE_MASTER_REG, AE_MASTER_SOFT_RESET);
	bus_barrier(sc->mem[0], AE_MASTER_REG, 4,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);
	
	for (i = 0; i < AE_RESET_TIMEOUT; i++) {
		if ((AE_READ_4(sc, AE_MASTER_REG) & AE_MASTER_SOFT_RESET) == 0)
			break;
		DELAY(10);
	}

	if (i == AE_RESET_TIMEOUT) {
		device_printf(sc->dev, "reset timeout.\n");
		return (ENXIO);
	}

	/* Wait for everything to enter idle state */
	for (i = 0; i < AE_IDLE_TIMEOUT; i++) {
		if (AE_READ_4(sc, AE_IDLE_REG) == 0)
			break;
		DELAY(100);
	}

	if (i == AE_IDLE_TIMEOUT) {
		device_printf(sc->dev, "could not enter idle state.\n");
		return (ENXIO);
	}

	return (0);
}

static void
ae_init(void *arg)
{
	ae_softc_t *sc;

	sc = (ae_softc_t *)arg;
	AE_LOCK(sc);
	ae_init_locked(sc);
	AE_UNLOCK(sc);
}

static int
ae_init_locked(ae_softc_t *sc)
{
	struct ifnet *ifp;
	struct mii_data *mii;
	uint8_t eaddr[ETHER_ADDR_LEN];
	uint32_t val;
	bus_addr_t addr;

	AE_LOCK_ASSERT(sc);

	ifp = sc->ifp;
	mii = device_get_softc(sc->miibus);

	/* Stop the device. */
	ae_stop(sc);
	
	/* Reset the adapter. */
	ae_reset(sc);
	
	/* Initialize PCIE module. */
	ae_pcie_init(sc);

	/* Initialize ring buffers. */
	ae_alloc_rings(sc);

	/* Clear and disable interrupts. */
	AE_WRITE_4(sc, AE_ISR_REG, 0xffffffff);

	/* Set the MAC address. */
	bcopy(IF_LLADDR(ifp), eaddr, ETHER_ADDR_LEN);
	val = eaddr[2] << 24 | eaddr[3] << 16 | eaddr[4] << 8 | eaddr[5];
	AE_WRITE_4(sc, AE_EADDR0_REG, val);
	val = eaddr[0] << 8 | eaddr[1];
	AE_WRITE_4(sc, AE_EADDR1_REG, val);

	/* Set ring buffers base addresses. */
	addr = sc->dma_rxd_busaddr;
	AE_WRITE_4(sc, AE_DESC_ADDR_HI_REG, BUS_ADDR_HI(addr));
	AE_WRITE_4(sc, AE_RXD_ADDR_LO_REG, BUS_ADDR_LO(addr));
	addr = sc->dma_txd_busaddr;
	AE_WRITE_4(sc, AE_TXD_ADDR_LO_REG, BUS_ADDR_LO(addr));
	addr = sc->dma_txs_busaddr;
	AE_WRITE_4(sc, AE_TXS_ADDR_LO_REG, BUS_ADDR_LO(addr));

	/* Configure ring buffers sizes. */
	AE_WRITE_2(sc, AE_RXD_COUNT_REG, AE_RXD_COUNT_DEFAULT);
	AE_WRITE_2(sc, AE_TXD_BUFSIZE_REG, AE_TXD_BUFSIZE_DEFAULT / 4);
	AE_WRITE_2(sc, AE_TXS_COUNT_REG, AE_TXS_COUNT_DEFAULT);

	/* Configure interframe gap parameters. */
	val = ((AE_IFG_TXIPG_DEFAULT << AE_IFG_TXIPG_SHIFT) &
	    AE_IFG_TXIPG_MASK) |
	    ((AE_IFG_RXIPG_DEFAULT << AE_IFG_RXIPG_SHIFT) &
	    AE_IFG_RXIPG_MASK) |
	    ((AE_IFG_IPGR1_DEFAULT << AE_IFG_IPGR1_SHIFT) &
	    AE_IFG_IPGR1_MASK) |
	    ((AE_IFG_IPGR2_DEFAULT << AE_IFG_IPGR2_SHIFT) &
	    AE_IFG_IPGR2_MASK);
	AE_WRITE_4(sc, AE_IFG_REG, val);

	/* Configure half-duplex operation. */
	val = ((AE_HDPX_LCOL_DEFAULT << AE_HDPX_LCOL_SHIFT) &
	    AE_HDPX_LCOL_MASK) |
	    ((AE_HDPX_RETRY_DEFAULT << AE_HDPX_RETRY_SHIFT) &
	    AE_HDPX_RETRY_MASK) |
	    ((AE_HDPX_ABEBT_DEFAULT << AE_HDPX_ABEBT_SHIFT) &
	    AE_HDPX_ABEBT_MASK) |
	    ((AE_HDPX_JAMIPG_DEFAULT << AE_HDPX_JAMIPG_SHIFT) &
	    AE_HDPX_JAMIPG_MASK) | AE_HDPX_EXC_EN;
	AE_WRITE_4(sc, AE_HDPX_REG, val);

	/* Configure interrupt moderate timer. */
	AE_WRITE_2(sc, AE_IMT_REG, AE_IMT_DEFAULT);
	val = AE_READ_4(sc, AE_MASTER_REG);
	val |= AE_MASTER_IMT_EN;
	AE_WRITE_4(sc, AE_MASTER_REG, val);

	/* Configure interrupt clearing timer. */
	AE_WRITE_2(sc, AE_ICT_REG, AE_ICT_DEFAULT);

	/* Configure MTU. */
	val = ifp->if_mtu + ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN +
	    ETHER_CRC_LEN;
	AE_WRITE_2(sc, AE_MTU_REG, val);

	/* Configure cut-through threshold. */
	AE_WRITE_4(sc, AE_CUT_THRESH_REG, AE_CUT_THRESH_DEFAULT);

	/* Configure flow control. */
	AE_WRITE_2(sc, AE_FLOW_THRESH_HI_REG, (AE_RXD_COUNT_DEFAULT / 8) * 7);
	AE_WRITE_2(sc, AE_FLOW_THRESH_LO_REG, (AE_RXD_COUNT_MIN / 8) >
	    (AE_RXD_COUNT_DEFAULT / 12) ? (AE_RXD_COUNT_MIN / 8) :
	    (AE_RXD_COUNT_DEFAULT / 12));

	/* Init mailboxes. */
	sc->txd_cur = sc->rxd_cur = 0;
	sc->txs_ack = sc->txd_ack = 0;
	sc->rxd_cur = 0;
	AE_WRITE_2(sc, AE_MB_TXD_IDX_REG, sc->txd_cur);
	AE_WRITE_2(sc, AE_MB_RXD_IDX_REG, sc->rxd_cur);

	sc->tx_inproc = 0;	/* Number of packets the chip processes now. */

	/* We have free Txs available */
	sc->flags |= AE_FLAG_TXAVAIL;

	/* Enable DMA. */
	AE_WRITE_1(sc, AE_DMAREAD_REG, AE_DMAREAD_EN);
	AE_WRITE_1(sc, AE_DMAWRITE_REG, AE_DMAWRITE_EN);

	/* Check if everything is OK. */
	val = AE_READ_4(sc, AE_ISR_REG);
	if ((val & AE_ISR_PHY_LINKDOWN) != 0) {
		printf("Initialization failed.\n");
		return (ENXIO);
	}

	/* Clear interrupt status. */
	AE_WRITE_4(sc, AE_ISR_REG, 0x3fffffff);
	AE_WRITE_4(sc, AE_ISR_REG, 0x0);

	/* Enable interrupts. */
	val = AE_READ_4(sc, AE_MASTER_REG);
	AE_WRITE_4(sc, AE_MASTER_REG, val | AE_MASTER_MANUAL_INT);
	AE_WRITE_4(sc, AE_IMR_REG, AE_IMR_DEFAULT);

	/* Configure MAC. */
	val = AE_MAC_TX_CRC_EN | AE_MAC_TX_AUTOPAD |
	    AE_MAC_FULL_DUPLEX | AE_MAC_CLK_PHY |
	    AE_MAC_TX_FLOW_EN | AE_MAC_RX_FLOW_EN |
	    ((AE_HALFBUF_DEFAULT << AE_HALFBUF_SHIFT) & AE_HALFBUF_MASK) |
	    ((AE_MAC_PREAMBLE_DEFAULT << AE_MAC_PREAMBLE_SHIFT) &
	    AE_MAC_PREAMBLE_MASK);
	AE_WRITE_4(sc, AE_MAC_REG, val);

	/* Set up the receive filter. */
	ae_rxfilter(sc);
/*	ae_rxvlan(sc); XXX: */

	/* Enable Tx/Rx. */
	val = AE_READ_4(sc, AE_MAC_REG);
	AE_WRITE_4(sc, AE_MAC_REG, val | AE_MAC_TX_EN | AE_MAC_RX_EN);

	sc->flags &= ~AE_FLAG_LINK;

	/* Switch to the current media */
	mii_mediachg(mii);

	callout_reset(&sc->tick_ch, hz, ae_tick, sc);

	ifp->if_drv_flags |= IFF_DRV_RUNNING;
	ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;

#ifdef AE_DEBUG
	device_printf(sc->dev, "Initialization complete.\n");
#endif

	return (0);
}

static unsigned int
ae_detach(device_t dev)
{
	struct ae_softc *sc;
	struct ifnet *ifp;

	sc = device_get_softc(dev);
	KASSERT(sc != NULL, ("[ae: %d]: sc is null", __LINE__));
	ifp = sc->ifp;

	if (device_is_attached(dev)) {
		AE_LOCK(sc);
		sc->flags |= AE_FLAG_DETACH;
		ae_stop(sc);
		AE_UNLOCK(sc);
		callout_drain(&sc->tick_ch);
		taskqueue_drain(sc->tq, &sc->int_task);
		taskqueue_drain(sc->tq, &sc->tx_task);
		taskqueue_drain(taskqueue_swi, &sc->link_task);
		ether_ifdetach(ifp);
	}

	if (sc->tq != NULL) {
		taskqueue_drain(sc->tq, &sc->int_task);
		taskqueue_free(sc->tq);
		sc->tq = NULL;
	}

	if (sc->miibus != NULL) {
		device_delete_child(dev, sc->miibus);
		sc->miibus = NULL;
	}

	bus_generic_detach(sc->dev);
	ae_dma_free(sc);

	if (sc->intrhand != NULL) {
		bus_teardown_intr(dev, sc->irq[0], sc->intrhand);
		sc->intrhand = NULL;
	}

	if (ifp != NULL) {
		if_free(ifp);
		sc->ifp = NULL;
	}

	if (sc->spec_irq != NULL)
		bus_release_resources(dev, sc->spec_irq, sc->irq);
	if (sc->spec_mem != NULL)
		bus_release_resources(dev, sc->spec_mem, sc->mem);

	mtx_destroy(&sc->mtx);

	return (0);
}

static int
ae_miibus_readreg(device_t dev, int phy, int reg)
{
	ae_softc_t *sc;
	uint32_t val;
	int i;

	sc = device_get_softc(dev);
	KASSERT(sc != NULL, ("[ae, %d]: sc is NULL", __LINE__));

	/* Locking is done in upper layers. */

#ifdef notdef
	device_printf(sc->dev, "MII read reg%d\n", reg);
#endif

	if (phy != sc->phyaddr)
		return (0);

	val = ((reg << AE_MDIO_REGADDR_SHIFT) & AE_MDIO_REGADDR_MASK) |
	    AE_MDIO_START | AE_MDIO_READ | AE_MDIO_SUP_PREAMBLE |
	    ((AE_MDIO_CLK_25_4 << AE_MDIO_CLK_SHIFT) & AE_MDIO_CLK_MASK);
	AE_WRITE_4(sc, AE_MDIO_REG, val);

	for (i = 0; i < AE_MDIO_TIMEOUT; i++) {
		DELAY(2);
		val = AE_READ_4(sc, AE_MDIO_REG);
		if ((val & (AE_MDIO_START | AE_MDIO_BUSY)) == 0)
			break;
	}

	if (i == AE_MDIO_TIMEOUT) {
		device_printf(sc->dev, "phy read timeout: %d\n", reg);
		return (0);
	}

	return ((val << AE_MDIO_DATA_SHIFT) & AE_MDIO_DATA_MASK);
}

static int
ae_miibus_writereg(device_t dev, int phy, int reg, int val)
{
	ae_softc_t *sc;
	uint32_t aereg;
	int i;

	sc = device_get_softc(dev);
	KASSERT(sc != NULL, ("[ae, %d]: sc is NULL", __LINE__));

	/* Locking is done in upper layers. */

	if (phy != sc->phyaddr)
		return (0);

	aereg = ((reg << AE_MDIO_REGADDR_SHIFT) & AE_MDIO_REGADDR_MASK) |
	    AE_MDIO_START | AE_MDIO_SUP_PREAMBLE |
	    ((AE_MDIO_CLK_25_4 << AE_MDIO_CLK_SHIFT) & AE_MDIO_CLK_MASK) |
	    ((val << AE_MDIO_DATA_SHIFT) & AE_MDIO_DATA_MASK);
	AE_WRITE_4(sc, AE_MDIO_REG, aereg);

	for (i = 0; i < AE_MDIO_TIMEOUT; i++) {
		DELAY(2);
		aereg = AE_READ_4(sc, AE_MDIO_REG);
		if ((aereg & (AE_MDIO_START | AE_MDIO_BUSY)) == 0)
			break;
	}

	if (i == AE_MDIO_TIMEOUT) {
		device_printf(sc->dev, "phy read timeout: %d\n", reg);
	}

	return (0);
}

/* MII bus callback when media changes */
static void
ae_miibus_statchg(device_t dev)
{
	ae_softc_t *sc;

	sc = device_get_softc(dev);
	taskqueue_enqueue(taskqueue_swi, &sc->link_task);
}

/* Get current media status */
static void
ae_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	ae_softc_t *sc;
	struct mii_data *mii;

	sc = ifp->if_softc;
	KASSERT(sc != NULL, ("[ae, %d]: sc is NULL", __LINE__));

	AE_LOCK(sc);
	mii = device_get_softc(sc->miibus);
	mii_pollstat(mii);
	ifmr->ifm_status = mii->mii_media_status;
	ifmr->ifm_active = mii->mii_media_active;
	AE_UNLOCK(sc);
}

/* Switch to new media. */
static int
ae_mediachange(struct ifnet *ifp)
{
	ae_softc_t *sc;
	struct mii_data *mii;
	struct mii_softc *mii_sc;
	int error;

	/* XXX: check IFF_UP ?? */

	sc = ifp->if_softc;
	KASSERT(sc != NULL, ("[ae, %d]: sc is NULL", __LINE__));
	AE_LOCK(sc);
	mii = device_get_softc(sc->miibus);
	if (mii->mii_instance != 0) {
		LIST_FOREACH(mii_sc, &mii->mii_phys, mii_list)
			mii_phy_reset(mii_sc);
	}

	error = mii_mediachg(mii);
	AE_UNLOCK(sc);

	return (error);
}

static int
ae_check_eeprom_present(ae_softc_t *sc, int *vpdc)
{
	int error;
	uint32_t val;

	KASSERT(vpdc != NULL, ("[ae, %d]: vpdc is NULL!\n", __LINE__));

	/*
	 * Not sure why, but Linux does this.
	 */
	val = AE_READ_4(sc, AE_SPICTL_REG);
	if ((val & AE_SPICTL_VPD_EN) != 0) {
		val &= ~AE_SPICTL_VPD_EN;
		AE_WRITE_4(sc, AE_SPICTL_REG, val);
	}

	error = pci_find_extcap(sc->dev, PCIY_VPD, vpdc);

	return (error);
}

static int
ae_vpd_read_word(ae_softc_t *sc, int reg, uint32_t *word)
{
	uint32_t val;
	int i;

	AE_WRITE_4(sc, AE_VPD_DATA_REG, 0);	/* Clear register value. */

	/* VPD registers start at offset 0x100. */
	val = 0x100 + reg * 4;
	AE_WRITE_4(sc, AE_VPD_CAP_REG, (val << AE_VPD_CAP_ADDR_SHIFT) &
	    AE_VPD_CAP_ADDR_MASK);

	for (i = 0; i < AE_VPD_TIMEOUT; i++) {
		DELAY(2000);
		val = AE_READ_4(sc, AE_VPD_CAP_REG);
		if ((val & AE_VPD_CAP_DONE) != 0)
			break;
	}
	
	if (i == AE_VPD_TIMEOUT) {
		device_printf(sc->dev, "timeout reading VPD register %d.\n",
		    reg);
		return (ETIMEDOUT);
	}

	*word = AE_READ_4(sc, AE_VPD_DATA_REG);

	return (0);
}

static int
ae_get_vpd_eaddr(ae_softc_t *sc, uint32_t *eaddr)
{
	uint32_t word, reg, val;
	int error;
	int found;
	int vpdc;
	int i;

	KASSERT(sc != NULL, ("[ae, %d]: sc is NULL", __LINE__));
	KASSERT(eaddr != NULL, ("[ae, %d]: eaddr is NULL", __LINE__));

	/* Check for EEPROM. */
	error = ae_check_eeprom_present(sc, &vpdc);
	if (error != 0)
		return (error);

	/*
	 * Read the VPD configuration space.
	 * Each register is prefixed with signature,
	 * so we can check if it is valid.
	 */

	for (i = 0, found = 0; i < AE_VPD_NREGS; i++) {
		error = ae_vpd_read_word(sc, i, &word);
		if (error != 0)
			break;
		
		/* Check signature. */
		if ((word & AE_VPD_SIG_MASK) != AE_VPD_SIG)
			break;

		/* Ok, we've found a valid block. */
		reg = word >> AE_VPD_REG_SHIFT;

		/* Move to the next word. */
		i++;

		if (reg != AE_EADDR0_REG && reg != AE_EADDR1_REG)
			continue;

		error = ae_vpd_read_word(sc, i, &val);
		if (error != 0)
			break;

		if (reg == AE_EADDR0_REG)
			eaddr[0] = val;
		else
			eaddr[1] = val;

		found++;
	}

	if (found < 2)
		return (ENOENT);
	
	/* Only last 2 bytes are used. */
	eaddr[1] &= 0xffff;

	if (AE_CHECK_EADDR_VALID(eaddr) != 0) {
		if (bootverbose || 1)
			device_printf(sc->dev,
			    "VPD ethernet address registers are invalid.\n");

		return (EINVAL);
	}

	return (0);
}

static int
ae_get_reg_eaddr(ae_softc_t *sc, uint32_t *eaddr)
{
	/*
	 * BIOS is supposed to set this.
	 */
	eaddr[0] = AE_READ_4(sc, AE_EADDR0_REG);
	eaddr[1] = AE_READ_4(sc, AE_EADDR1_REG);

	/* Only last 2 bytes are used. */
	eaddr[1] &= 0xffff;

	if (AE_CHECK_EADDR_VALID(eaddr) != 0) {
		if (bootverbose || 1)
			device_printf(sc->dev,
			    "Ethetnet address registers are invalid.\n");

		return (EINVAL);
	}

	return (0);
}

static void
ae_retrieve_address(ae_softc_t *sc)
{
	uint32_t eaddr[2] = {0, 0};
	int error;

	/* Check for EEPROM. */
	error = ae_get_vpd_eaddr(sc, eaddr);
	if (error != 0)
		error = ae_get_reg_eaddr(sc, eaddr);

	if (error != 0) {
		if (bootverbose || 1)
			device_printf(sc->dev,
			    "Generating fake ethernet address.\n");

#ifdef __HAIKU__
		eaddr[0] = rand();
#else
		eaddr[0] = arc4random();
#endif
		/* Set OUI to ASUSTek COMPUTER INC. */
		sc->eaddr[0] = 0x00;
		sc->eaddr[1] = 0x1f;
		sc->eaddr[2] = 0xc6;
		sc->eaddr[3] = (eaddr[0] >> 16) & 0xff;
		sc->eaddr[4] = (eaddr[0] >> 8) & 0xff;
		sc->eaddr[5] = (eaddr[0] >> 0) & 0xff;
	} else {
		sc->eaddr[0] = (eaddr[1] >> 8) & 0xff;
		sc->eaddr[1] = (eaddr[1] >> 0) & 0xff;
		sc->eaddr[2] = (eaddr[0] >> 24) & 0xff;
		sc->eaddr[3] = (eaddr[0] >> 16) & 0xff;
		sc->eaddr[4] = (eaddr[0] >> 8) & 0xff;
		sc->eaddr[5] = (eaddr[0] >> 0) & 0xff;
	}
}

static void
ae_dmamap_cb(void *arg, bus_dma_segment_t *segs, int nsegs, int error)
{
	bus_addr_t *addr = arg;

	if (error != 0)
		return;

	KASSERT(nsegs == 1, ("[ae, %d]: %d segments instead of 1!", __LINE__,
	    nsegs));

	*addr = segs[0].ds_addr;
}

static int
ae_alloc_rings(ae_softc_t *sc)
{
	bus_dma_tag_t bustag;
	bus_addr_t busaddr;
	int error;

	bustag = bus_get_dma_tag(sc->dev);

	/* Create parent DMA tag. */
	error = bus_dma_tag_create(bus_get_dma_tag(sc->dev),
	    1, 0, BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR,
	    NULL, NULL, BUS_SPACE_MAXSIZE_32BIT, 0,
	    BUS_SPACE_MAXSIZE_32BIT, 0, NULL, NULL,
	    &sc->dma_parent_tag);
	if (error != 0) {
		device_printf(sc->dev, "could not creare parent DMA tag.\n");
		return (error);
	}

	/* Create DMA tag for TxD. */
	error = bus_dma_tag_create(sc->dma_parent_tag,
	    4, 0, BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR,
	    NULL, NULL, AE_TXD_BUFSIZE_DEFAULT, 1,
	    AE_TXD_BUFSIZE_DEFAULT, 0, NULL, NULL,
	    &sc->dma_txd_tag);
	if (error != 0) {
		device_printf(sc->dev, "could not creare TxD DMA tag.\n");
		return (error);
	}

	/* Create DMA tag for TxS. */
	error = bus_dma_tag_create(sc->dma_parent_tag,
	    4, 0, BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR,
	    NULL, NULL, AE_TXS_COUNT_DEFAULT * 4, 1,
	    AE_TXS_COUNT_DEFAULT * 4, 0, NULL, NULL,
	    &sc->dma_txs_tag);
	if (error != 0) {
		device_printf(sc->dev, "could not creare TxS DMA tag.\n");
		return (error);
	}

	/* Create DMA tag for RxD. */
	error = bus_dma_tag_create(sc->dma_parent_tag,
	    128, 0, BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR,
	    NULL, NULL, AE_RXD_COUNT_DEFAULT * 1536 + 120, 1,
	    AE_RXD_COUNT_DEFAULT * 1536 + 120, 0, NULL, NULL,
	    &sc->dma_rxd_tag);
	if (error != 0) {
		device_printf(sc->dev, "could not creare TxS DMA tag.\n");
		return (error);
	}

	/* Allocate DMA memory. */
	error = bus_dmamem_alloc(sc->dma_txd_tag, (void **)&sc->txd_base,
	    BUS_DMA_WAITOK | BUS_DMA_ZERO | BUS_DMA_COHERENT,
	    &sc->dma_txd_map);
	if (error != 0) {
		device_printf(sc->dev,
		    "could not allocate DMA memory for TxD ring.\n");
		return (error);
	}

	error = bus_dmamap_load(sc->dma_txd_tag, sc->dma_txd_map, sc->txd_base,
	    AE_TXD_BUFSIZE_DEFAULT, ae_dmamap_cb, &busaddr, BUS_DMA_NOWAIT);
	if (error != 0 || busaddr == 0) {
		device_printf(sc->dev,
		    "could not load DMA map for TxD ring.\n");
		return (error);
	}

	sc->dma_txd_busaddr = busaddr;

	error = bus_dmamem_alloc(sc->dma_txs_tag, (void **)&sc->txs_base,
	    BUS_DMA_WAITOK | BUS_DMA_ZERO | BUS_DMA_COHERENT,
	    &sc->dma_txs_map);
	if (error != 0) {
		device_printf(sc->dev,
		    "could not allocate DMA memory for TxS ring.\n");
		return (error);
	}

	error = bus_dmamap_load(sc->dma_txs_tag, sc->dma_txs_map, sc->txs_base,
	    AE_TXS_COUNT_DEFAULT * 4, ae_dmamap_cb, &busaddr, BUS_DMA_NOWAIT);
	if (error != 0 || busaddr == 0) {
		device_printf(sc->dev,
		    "could not load DMA map for TxS ring.\n");
		return (error);
	}
	sc->dma_txs_busaddr = busaddr;

	error = bus_dmamem_alloc(sc->dma_rxd_tag, (void **)&sc->rxd_base_dma,
	    BUS_DMA_WAITOK | BUS_DMA_ZERO | BUS_DMA_COHERENT,
	    &sc->dma_rxd_map);
	if (error != 0) {
		device_printf(sc->dev,
		    "could not allocate DMA memory for RxD ring.\n");
		return (error);
	}

	error = bus_dmamap_load(sc->dma_rxd_tag, sc->dma_rxd_map, sc->rxd_base_dma,
	    AE_RXD_COUNT_DEFAULT * 1536 + 120, ae_dmamap_cb, &busaddr, BUS_DMA_NOWAIT);
	if (error != 0 || busaddr == 0) {
		device_printf(sc->dev,
		    "could not load DMA map for RxD ring.\n");
		return (error);
	}

	sc->dma_rxd_busaddr = busaddr + 120;
	sc->rxd_base = (ae_rxd_t *)(sc->rxd_base_dma + 120);

	return (0);
}

static void
ae_dma_free(ae_softc_t *sc)
{
	if (sc->dma_txd_tag != NULL) {
		if (sc->dma_txd_map != NULL) {
			bus_dmamap_unload(sc->dma_txd_tag, sc->dma_txd_map);

			if (sc->txd_base != NULL)
				bus_dmamem_free(sc->dma_txd_tag, sc->txd_base,
				    sc->dma_txd_map);

		}

		bus_dma_tag_destroy(sc->dma_txd_tag);
		sc->dma_txd_map = NULL;
		sc->dma_txd_tag = NULL;
		sc->txd_base = NULL;
	}

	if (sc->dma_txs_tag != NULL) {
		if (sc->dma_txs_map != NULL) {
			bus_dmamap_unload(sc->dma_txs_tag, sc->dma_txs_map);

			if (sc->txs_base != NULL)
				bus_dmamem_free(sc->dma_txs_tag, sc->txs_base,
				    sc->dma_txs_map);

		}

		bus_dma_tag_destroy(sc->dma_txs_tag);
		sc->dma_txs_map = NULL;
		sc->dma_txs_tag = NULL;
		sc->txs_base = NULL;
	}

	if (sc->dma_rxd_tag != NULL) {
		if (sc->dma_rxd_map != NULL) {
			bus_dmamap_unload(sc->dma_rxd_tag, sc->dma_rxd_map);

			if (sc->rxd_base_dma != NULL)
				bus_dmamem_free(sc->dma_rxd_tag, sc->rxd_base_dma,
				    sc->dma_rxd_map);

		}

		bus_dma_tag_destroy(sc->dma_rxd_tag);
		sc->dma_rxd_map = NULL;
		sc->dma_rxd_tag = NULL;
		sc->rxd_base_dma = NULL;
	}

	if (sc->dma_parent_tag != NULL) {
		bus_dma_tag_destroy(sc->dma_parent_tag);
		sc->dma_parent_tag = NULL;
	}
}

static int
ae_shutdown(device_t dev)
{
	int error;

	error = ae_suspend(dev);

	return (error);
}

static int
ae_suspend(device_t dev)
{
	ae_softc_t *sc;

	sc = device_get_softc(dev);

	AE_LOCK(sc);
	ae_stop(sc);
	AE_UNLOCK(sc);

	return (0);
}

static int
ae_resume(device_t dev)
{
	ae_softc_t *sc;

	sc = device_get_softc(dev);
	KASSERT(sc != NULL, ("[ae, %d]: sc is null", __LINE__));

	AE_LOCK(sc);
	if ((sc->ifp->if_flags & IFF_UP) != 0)
		ae_init_locked(sc);
	AE_UNLOCK(sc);

	return (0);
}

static unsigned int
ae_tx_avail_size(ae_softc_t *sc)
{
	unsigned int avail;
	
	if (sc->txd_cur >= sc->txd_ack)
		avail = AE_TXD_BUFSIZE_DEFAULT - (sc->txd_cur - sc->txd_ack);
	else
		avail = sc->txd_ack - sc->txd_cur;

	return (avail - 4);	/* 4-byte header. */
}

static int
ae_encap(ae_softc_t *sc, struct mbuf **m_head)
{
	struct mbuf *m0;
	ae_txd_t *hdr;
	unsigned int to_end;

	AE_LOCK_ASSERT(sc);

	m0 = *m_head;
	
	if ((sc->flags & AE_FLAG_TXAVAIL) == 0 ||
	    ae_tx_avail_size(sc) < m0->m_pkthdr.len) {
#ifdef AE_DEBUG
		if_printf(sc->ifp, "No free Tx available.\n");
#endif
		return ENOBUFS;
	}

	hdr = (ae_txd_t *)(sc->txd_base + sc->txd_cur);
	bzero(hdr, sizeof(*hdr));

	hdr->len = htole32(m0->m_pkthdr.len);
	sc->txd_cur = (sc->txd_cur + 4) % AE_TXD_BUFSIZE_DEFAULT; /* Header
								     size. */

	to_end = AE_TXD_BUFSIZE_DEFAULT - sc->txd_cur; /* Space available to
							* the end of the ring
							*/

#ifdef AE_DEBUG
	if_printf(sc->ifp, "Writing data.\n");
#endif

	if (to_end >= m0->m_pkthdr.len) {
		m_copydata(m0, 0, m0->m_pkthdr.len,
		    (caddr_t)(sc->txd_base + sc->txd_cur));
	} else {
		m_copydata(m0, 0, to_end, (caddr_t)(sc->txd_base +
		    sc->txd_cur));
		m_copydata(m0, to_end, m0->m_pkthdr.len - to_end,
		    (caddr_t)sc->txd_base);
	}

#ifdef AE_DEBUG
	if_printf(sc->ifp, "Written data.\n");
#endif

	/* Set current TxD position and round up to a 4-byte boundary. */
	sc->txd_cur = ((sc->txd_cur + m0->m_pkthdr.len + 3) & ~3) %
	    AE_TXD_BUFSIZE_DEFAULT;

	if (sc->txd_cur == sc->txd_ack)
		sc->flags &= ~AE_FLAG_TXAVAIL;

#ifdef AE_DEBUG
	if_printf(sc->ifp, "New txd_cur = %d.\n", sc->txd_cur);
#endif

	/* Update TxS position and check if there are empty TxS available */
	sc->txs_base[sc->txs_cur].update = 0;
	sc->txs_cur = (sc->txs_cur + 1) % AE_TXS_COUNT_DEFAULT;
	if (sc->txs_cur == sc->txs_ack)
		sc->flags &= ~AE_FLAG_TXAVAIL;

	bus_dmamap_sync(sc->dma_txd_tag, sc->dma_txd_map, BUS_DMASYNC_PREREAD |
	    BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->dma_txs_tag, sc->dma_txs_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return (0);
}

static void
ae_start(struct ifnet *ifp)
{
	ae_softc_t *sc;
	unsigned int count = 0;
	struct mbuf *m0;
	int error;

	sc = ifp->if_softc;
	KASSERT(sc != NULL, ("[ae, %d]: sc is null", __LINE__));

	AE_LOCK(sc);
	
	if ((ifp->if_drv_flags & (IFF_DRV_RUNNING | IFF_DRV_OACTIVE)) !=
	    IFF_DRV_RUNNING || (sc->flags & AE_FLAG_LINK) == 0) {
		AE_UNLOCK(sc);
		return;
	}

#ifdef AE_DEBUG
	if_printf(ifp, "Start called.\n");
#endif

	while (!IFQ_DRV_IS_EMPTY(&ifp->if_snd)) {
		IFQ_DRV_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;	/* Nothing to do. */

		error = ae_encap(sc, &m0);
		if (error != 0) {
			if (m0 != NULL) {
				IFQ_DRV_PREPEND(&ifp->if_snd, m0);
				ifp->if_drv_flags |= IFF_DRV_OACTIVE;
#ifdef AE_DEBUG
				if_printf(ifp, "Setting OACTIVE.\n");
#endif
			}
			break;
		}

		count++;
		sc->tx_inproc++;

		/* Bounce a copy of the frame to BPF. */
		ETHER_BPF_MTAP(ifp, m0);

		m_freem(m0);
	}

	if (count > 0) {	/* Something was dequeued. */
		AE_WRITE_2(sc, AE_MB_TXD_IDX_REG, sc->txd_cur / 4);
		sc->wd_timer = AE_TX_TIMEOUT;	/* Load watchdog. */
#ifdef AE_DEBUG
		if_printf(ifp, "%d packets dequeued.\n", count);
		if_printf(ifp, "Tx pos now is %d.\n", sc->txd_cur);
#endif
	}

	AE_UNLOCK(sc);
}

static void
ae_link_task(void *arg, int pending)
{
	ae_softc_t *sc;
	struct mii_data *mii;
	struct ifnet *ifp;
	uint32_t val;

	sc = (ae_softc_t *)arg;
	KASSERT(sc != NULL, ("[ae, %d]: sc is null", __LINE__));

	AE_LOCK(sc);

	ifp = sc->ifp;
	mii = device_get_softc(sc->miibus);

	if (mii == NULL || ifp == NULL ||
	    (ifp->if_drv_flags & IFF_DRV_RUNNING) == 0) {
		AE_UNLOCK(sc);	/* XXX: could happen? */
		return;
	}
	
	sc->flags &= ~AE_FLAG_LINK;
	if ((mii->mii_media_status & IFM_AVALID) != 0) {
		switch(IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_10_T:
		case IFM_100_TX:
			sc->flags |= AE_FLAG_LINK;
			break;
		default:
			break;
		}
	}

	/* Stop Rx/Tx MACs. */
	ae_stop_rxmac(sc);
	ae_stop_txmac(sc);

	if ((sc->flags & AE_FLAG_LINK) != 0) {
		ae_mac_config(sc);

		/* Restart DMA engines. */
		AE_WRITE_1(sc, AE_DMAREAD_REG, AE_DMAREAD_EN);
		AE_WRITE_1(sc, AE_DMAWRITE_REG, AE_DMAWRITE_EN);

		/* Enable Rx and Tx MACs. */
		val = AE_READ_4(sc, AE_MAC_REG);
		val |= AE_MAC_TX_EN | AE_MAC_RX_EN;
		AE_WRITE_4(sc, AE_MAC_REG, val);
	}

	AE_UNLOCK(sc);
}

static void
ae_stop_rxmac(ae_softc_t *sc)
{
	uint32_t val;
	int i;

	AE_LOCK_ASSERT(sc);

	/* Stop Rx MAC engine. */
	val = AE_READ_4(sc, AE_MAC_REG);
	if ((val & AE_MAC_RX_EN) != 0) {
		val &= ~AE_MAC_RX_EN;
		AE_WRITE_4(sc, AE_MAC_REG, val);
	}

	/* Stop Rx DMA engine. */
	if (AE_READ_1(sc, AE_DMAWRITE_REG) == AE_DMAWRITE_EN)
		AE_WRITE_1(sc, AE_DMAWRITE_REG, 0);

	/* Wait for IDLE state. */
	for (i = 0; i < AE_IDLE_TIMEOUT; i--) {
		val = AE_READ_4(sc, AE_IDLE_REG);
		if ((val & (AE_IDLE_RXMAC | AE_IDLE_DMAWRITE)) == 0)
			break;
		DELAY(100);
	}

	if (i == AE_IDLE_TIMEOUT)
		device_printf(sc->dev, "timed out while stopping Rx MAC.\n");
}

static void
ae_stop_txmac(ae_softc_t *sc)
{
	uint32_t val;
	int i;

	AE_LOCK_ASSERT(sc);

	/* Stop Tx MAC engine. */
	val = AE_READ_4(sc, AE_MAC_REG);
	if ((val & AE_MAC_TX_EN) != 0) {
		val &= ~AE_MAC_TX_EN;
		AE_WRITE_4(sc, AE_MAC_REG, val);
	}

	/* Stop Tx DMA engine. */
	if (AE_READ_1(sc, AE_DMAREAD_REG) == AE_DMAREAD_EN)
		AE_WRITE_1(sc, AE_DMAREAD_REG, 0);

	/* Wait for IDLE state. */
	for (i = 0; i < AE_IDLE_TIMEOUT; i--) {
		val = AE_READ_4(sc, AE_IDLE_REG);
		if ((val & (AE_IDLE_TXMAC | AE_IDLE_DMAREAD)) == 0)
			break;
		DELAY(100);
	}

	if (i == AE_IDLE_TIMEOUT)
		device_printf(sc->dev, "timed out while stopping Tx MAC.\n");
}

static void
ae_tx_task(void *arg, int pending)
{
	struct ifnet *ifp;

	ifp = (struct ifnet *)arg;
	ae_start(ifp);
}

static void
ae_mac_config(ae_softc_t *sc)
{
	struct mii_data *mii;
	uint32_t val;

	AE_LOCK_ASSERT(sc);

	mii = device_get_softc(sc->miibus);
	val = AE_READ_4(sc, AE_MAC_REG);
	val &= ~AE_MAC_FULL_DUPLEX;

	if ((IFM_OPTIONS(mii->mii_media_active) & IFM_FDX) != 0)
		val |= AE_MAC_FULL_DUPLEX;

	AE_WRITE_4(sc, AE_MAC_REG, val);
}

static int
ae_intr(void *arg)
{
	ae_softc_t *sc;
	uint32_t val;

	sc = (ae_softc_t *)arg;
	KASSERT(sc != NULL, ("[ae, %d]: sc is null", __LINE__));

	val = AE_READ_4(sc, AE_ISR_REG);
	if (val == 0 || (val & AE_IMR_DEFAULT) == 0)
		return FILTER_STRAY;

	/* Disable interrupts. */
	AE_WRITE_4(sc, AE_ISR_REG, AE_ISR_DISABLE);

	/* Schedule interrupt processing. */
	taskqueue_enqueue(sc->tq, &sc->int_task);

	return (FILTER_HANDLED);
}

static void
ae_int_task(void *arg, int pending)
{
	ae_softc_t *sc;
	struct ifnet *ifp;
	struct mii_data *mii;
	uint32_t val;

	sc = (ae_softc_t *)arg;

	AE_LOCK(sc);

	ifp = sc->ifp;

	/* Read interrupt status. */
	val = AE_READ_4(sc, AE_ISR_REG);

	if ((val & AE_ISR_PHY) != 0) {
		/*
		 * Clear PHY interrupt. Not sure if it needed. From Linux.
		 */
		ae_miibus_readreg(sc->miibus, 1, 19);
	}

	/* Clear interrupts and disable them. */
	AE_WRITE_4(sc, AE_ISR_REG, val | AE_ISR_DISABLE);

#ifdef AE_DEBUG
	if_printf(ifp, "Interrupt received: 0x%08x\n", val);
#endif

	if ((ifp->if_drv_flags & IFF_DRV_RUNNING) != 0) {
		if ((val & (AE_ISR_PHY | AE_ISR_MANUAL)) != 0) {
			mii = device_get_softc(sc->miibus);
			mii_mediachg(mii);
		}

		if ((val & (AE_ISR_DMAR_TIMEOUT | AE_ISR_DMAW_TIMEOUT |
		    AE_ISR_PHY_LINKDOWN)) != 0) {
			ae_init_locked(sc);
		}

		if ((val & AE_ISR_TX_EVENT) != 0)
			ae_tx_intr(sc);

		if ((val & AE_ISR_RX_EVENT) != 0)
			ae_rx_intr(sc);
	}

	/* Re-enable interrupts. */
	AE_WRITE_4(sc, AE_ISR_REG, 0);

	AE_UNLOCK(sc);
}

static void
ae_tx_intr(ae_softc_t *sc)
{
	struct ifnet *ifp;
	ae_txd_t *txd;
	ae_txs_t *txs;

	AE_LOCK_ASSERT(sc);

	ifp = sc->ifp;

#ifdef AE_DEBUG
	if_printf(ifp, "Tx interrupt occuried.\n");
#endif

	/* Syncronize DMA buffers. */
	bus_dmamap_sync(sc->dma_txd_tag, sc->dma_txd_map,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	bus_dmamap_sync(sc->dma_txs_tag, sc->dma_txs_map,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	for (;;) {
		txs = sc->txs_base + sc->txs_ack;
		if (txs->update == 0)
			break;

		/* Update TxS position. */
		sc->txs_ack = (sc->txs_ack + 1) % AE_TXS_COUNT_DEFAULT;
		sc->flags |= AE_FLAG_TXAVAIL;

		txs->update = 0;	/* Clear status. */

		txd = (ae_txd_t *)(sc->txd_base + sc->txd_ack);
		if (txs->len != txd->len)
			device_printf(sc->dev, "Size mismatch: TxS:%d TxD:%d\n",
			    le32toh(txs->len), le32toh(txd->len));

		/* Move txd ack and align on 4-byte boundary. */
		sc->txd_ack = ((sc->txd_ack + le32toh(txd->len) + 4 + 3) & ~3) %
		    AE_TXD_BUFSIZE_DEFAULT;

		if (txs->flags & AE_TXF_SUCCESS) {	/* XXX: endianess. */
			ifp->if_opackets++;
			/* if_printf(ifp, "Packet successfully sent.\n"); */
		}
		else
			ifp->if_oerrors++;

		sc->tx_inproc--;

		ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;
	}

	if (sc->tx_inproc < 0) {
		if_printf(ifp, "Received stray Tx interrupt(s).\n");
		sc->tx_inproc = 0;
	}

	if (sc->tx_inproc == 0)
		sc->wd_timer = 0;	/* Unarm watchdog. */
	
	if ((sc->flags & AE_FLAG_TXAVAIL) != 0) {
		if (!IFQ_DRV_IS_EMPTY(&ifp->if_snd))
			taskqueue_enqueue(sc->tq, &sc->tx_task);
	}

	/* Syncronize DMA buffers. */
	bus_dmamap_sync(sc->dma_txd_tag, sc->dma_txd_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	bus_dmamap_sync(sc->dma_txs_tag, sc->dma_txs_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
}

static int
ae_rxeof(ae_softc_t *sc, ae_rxd_t *rxd)
{
	struct ifnet *ifp;
	struct mbuf *m;
	unsigned int size;

	AE_LOCK_ASSERT(sc);

	ifp = sc->ifp;

#ifdef AE_DEBUG
	if_printf(ifp, "Rx interrupt occuried.\n");
#endif

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (ENOBUFS);

	size = le32toh(rxd->len) - ETHER_CRC_LEN;
	if (size < 0) {
		if_printf(ifp, "Negative length packet received.");
		return (EIO);
	}

	if (size > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return (ENOBUFS);
		}
	}

	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len = size;

	memcpy(mtod(m, void *), &rxd->data[0], size);

	/* Pass it on. */
	AE_UNLOCK(sc);
	(*ifp->if_input)(ifp, m);
	AE_LOCK(sc);

	return (0);
}

static void
ae_rx_intr(ae_softc_t *sc)
{
	ae_rxd_t *rxd;
	struct ifnet *ifp;
	int error;

	KASSERT(sc != NULL, ("[ae, %d]: sc is NULL!", __LINE__));

	AE_LOCK_ASSERT(sc);

	ifp = sc->ifp;

	/* Syncronize DMA buffers. */
	bus_dmamap_sync(sc->dma_rxd_tag, sc->dma_rxd_map,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	for (;;) {
		rxd = (ae_rxd_t *)(sc->rxd_base + sc->rxd_cur);
		if (rxd->update == 0)
			break;

		/* Update position index. */
		sc->rxd_cur = (sc->rxd_cur + 1) % AE_RXD_COUNT_DEFAULT;
		rxd->update = 0;

		if ((rxd->flags & AE_RXF_SUCCESS) == 0) {
			ifp->if_ierrors++;
			continue;
		}
		error = ae_rxeof(sc, rxd);
		if (error != 0) {
			ifp->if_ierrors++;
			continue;
		} else {
			ifp->if_ipackets++;
		}
	}

	/* Update Rx index. */
	AE_WRITE_2(sc, AE_MB_RXD_IDX_REG, sc->rxd_cur);
}

static void
ae_watchdog(ae_softc_t *sc)
{
	struct ifnet *ifp;

	KASSERT(sc != NULL, ("[ae, %d]: sc is NULL!", __LINE__));

	AE_LOCK_ASSERT(sc);

	if (sc->wd_timer == 0 || --sc->wd_timer != 0)
		return;		/* Noting to do. */

	ifp = sc->ifp;

	if ((sc->flags & AE_FLAG_LINK) == 0)
		if_printf(ifp, "watchdog timeout (missed link).\n");
	else
		if_printf(ifp, "watchdog timeout - resetting.\n");

	ifp->if_oerrors++;
	ae_init_locked(sc);
	if (!IFQ_DRV_IS_EMPTY(&ifp->if_snd))
		taskqueue_enqueue(sc->tq, &sc->tx_task);
}

static void
ae_tick(void *arg)
{
	ae_softc_t *sc;
	struct mii_data *mii;

	sc = (ae_softc_t *)arg;

	KASSERT(sc != NULL, ("[ae, %d]: sc is NULL!", __LINE__));

	AE_LOCK_ASSERT(sc);

	mii = device_get_softc(sc->miibus);
	mii_tick(mii);

	ae_watchdog(sc);	/* Watchdog check. */

	callout_reset(&sc->tick_ch, hz, ae_tick, sc);
}

static void
ae_rxfilter(ae_softc_t *sc)
{
	struct ifnet *ifp;
	struct ifmultiaddr *ifma;
	uint32_t crc;
	uint32_t mchash[2];
	uint32_t rxcfg;

	KASSERT(sc != NULL, ("[ae, %d]: sc is NULL!", __LINE__));

	AE_LOCK_ASSERT(sc);

	ifp = sc->ifp;

	rxcfg = AE_READ_4(sc, AE_MAC_REG);
	rxcfg &= ~(AE_MAC_MCAST_EN | AE_MAC_BCAST_EN | AE_MAC_PROMISC_EN);

	if ((ifp->if_flags & IFF_BROADCAST) != 0)
		rxcfg |= AE_MAC_BCAST_EN;

	if ((ifp->if_flags & IFF_PROMISC) != 0)
		rxcfg |= AE_MAC_PROMISC_EN;

	if ((ifp->if_flags & IFF_ALLMULTI) != 0)
		rxcfg |= AE_MAC_MCAST_EN;

	/* Wipe old settings. */
	AE_WRITE_4(sc, AE_REG_MHT0, 0);
	AE_WRITE_4(sc, AE_REG_MHT1, 0);

	if ((ifp->if_flags & (IFF_PROMISC | IFF_ALLMULTI)) != 0) {
		AE_WRITE_4(sc, AE_REG_MHT0, 0xffffffff);
		AE_WRITE_4(sc, AE_REG_MHT1, 0xffffffff);
		AE_WRITE_4(sc, AE_MAC_REG, rxcfg);
		return;
	}

	bzero(mchash, sizeof(mchash));

	IF_ADDR_LOCK(ifp);
	TAILQ_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link) {
		if (ifma->ifma_addr->sa_family != AF_LINK)
			continue;
		crc = ether_crc32_le(LLADDR((struct sockaddr_dl *)
			ifma->ifma_addr), ETHER_ADDR_LEN);
		mchash[crc >> 31] |= 1 << ((crc >> 26) & 0x1f);
	}
	IF_ADDR_UNLOCK(ifp);

	AE_WRITE_4(sc, AE_REG_MHT0, mchash[0]);
	AE_WRITE_4(sc, AE_REG_MHT1, mchash[1]);
	AE_WRITE_4(sc, AE_MAC_REG, rxcfg);
}

static int
ae_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ae_softc *sc;
	struct ifreq *ifr;
	struct mii_data *mii;
	int error;

	sc = ifp->if_softc;
	ifr = (struct ifreq *)data;

	error = 0;

	switch (cmd) {
	case SIOCSIFMTU:
		if (ifr->ifr_mtu < ETHERMIN || ifr->ifr_mtu > ETHERMTU)
			error = EINVAL;
		else if (ifp->if_mtu != ifr->ifr_mtu) {
			AE_LOCK(sc);
			ifp->if_mtu = ifr->ifr_mtu;
			if ((ifp->if_drv_flags & IFF_DRV_RUNNING) != 0)
				ae_init_locked(sc);
			AE_UNLOCK(sc);
		}
		break;
	case SIOCSIFFLAGS:
		AE_LOCK(sc);
		if ((ifp->if_flags & IFF_UP) != 0) {
			if ((ifp->if_drv_flags & IFF_DRV_RUNNING) != 0) {
				if (((ifp->if_flags ^ sc->if_flags)
				    & (IFF_PROMISC | IFF_ALLMULTI)) != 0)
					ae_rxfilter(sc);
			} else {
				if ((sc->flags & AE_FLAG_DETACH) == 0)
					ae_init_locked(sc);
			}
		} else {
			if ((ifp->if_drv_flags & IFF_DRV_RUNNING) != 0)
				ae_stop(sc);
		}
		sc->if_flags = ifp->if_flags;
		AE_UNLOCK(sc);
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		AE_LOCK(sc);
		if ((ifp->if_drv_flags & IFF_DRV_RUNNING) != 0)
			ae_rxfilter(sc);
		AE_UNLOCK(sc);
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		mii = device_get_softc(sc->miibus);
		error = ifmedia_ioctl(ifp, ifr, &mii->mii_media, cmd);
		break;
	case SIOCSIFCAP:
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}

	return (error);
}

static void
ae_stop(ae_softc_t *sc)
{
	struct ifnet *ifp;
	int i;

	AE_LOCK_ASSERT(sc);

	ifp = sc->ifp;
	ifp->if_drv_flags &= ~(IFF_DRV_RUNNING | IFF_DRV_OACTIVE);
	sc->flags &= ~AE_FLAG_LINK;
	sc->wd_timer = 0;	/* Cancel watchdog. */
	callout_stop(&sc->tick_ch);

	/* Clear and disable interrupts. */
	AE_WRITE_4(sc, AE_IMR_REG, 0);
	AE_WRITE_4(sc, AE_ISR_REG, 0xffffffff);

	/* Stop Rx/Tx MACs. */
	ae_stop_txmac(sc);
	ae_stop_rxmac(sc);

	/* Stop DMA engines. */
	AE_WRITE_1(sc, AE_DMAREAD_REG, ~AE_DMAREAD_EN);
	AE_WRITE_1(sc, AE_DMAWRITE_REG, ~AE_DMAWRITE_EN);

	/* Wait for everything to enter idle state */
	for (i = 0; i < AE_IDLE_TIMEOUT; i++) {
		if (AE_READ_4(sc, AE_IDLE_REG) == 0)
			break;
		DELAY(100);
	}

	if (i == AE_IDLE_TIMEOUT)
		device_printf(sc->dev, "could not enter idle state in stop.\n");
}
