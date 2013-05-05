
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

/*
 * Defines and macros
 */

#define RT2860_MAX_AGG_SIZE						3840

#define RT2860_TX_DATA_SEG0_SIZE				\
	(sizeof(struct rt2860_txwi) + sizeof(struct ieee80211_qosframe_addr4))

#define RT2860_NOISE_FLOOR						-95

#define IEEE80211_HAS_ADDR4(wh)					\
	(((wh)->i_fc[1] & IEEE80211_FC1_DIR_MASK) == IEEE80211_FC1_DIR_DSTODS)

#define	RT2860_MS(_v, _f)						(((_v) & _f) >> _f##_S)
#define	RT2860_SM(_v, _f)						(((_v) << _f##_S) & _f)

#define RT2860_TX_WATCHDOG_TIMEOUT				5

#define RT2860_WCID_RESERVED					0xff
#define RT2860_WCID_MCAST						0xf7

/*
 * Global function prototypes, used in bus depended interfaces
 */

int rt2860_attach(device_t dev);

int rt2860_detach(device_t dev);

int rt2860_shutdown(device_t dev);

int rt2860_suspend(device_t dev);

int rt2860_resume(device_t dev);

/*
 * Static function prototypes
 */

static void rt2860_init_channels(struct rt2860_softc *sc);

static void rt2860_init_channels_ht40(struct rt2860_softc *sc);

static void rt2860_init_locked(void *priv);

static void rt2860_init(void *priv);

static int rt2860_init_bbp(struct rt2860_softc *sc);

static void rt2860_stop_locked(void *priv);

static void rt2860_stop(void *priv);

static void rt2860_start(struct ifnet *ifp);

static int rt2860_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data);

static struct ieee80211vap *rt2860_vap_create(struct ieee80211com *ic,
	const char name[IFNAMSIZ], int unit, int opmode, int flags,
	const uint8_t bssid[IEEE80211_ADDR_LEN],
	const uint8_t mac[IEEE80211_ADDR_LEN]);

static void rt2860_vap_delete(struct ieee80211vap *vap);

static int rt2860_vap_reset(struct ieee80211vap *vap, u_long cmd);

static int rt2860_vap_newstate(struct ieee80211vap *vap,
	enum ieee80211_state nstate, int arg);

static void rt2860_vap_key_update_begin(struct ieee80211vap *vap);

static void rt2860_vap_key_update_end(struct ieee80211vap *vap);

static int rt2860_vap_key_set(struct ieee80211vap *vap,
	const struct ieee80211_key *k, const uint8_t mac[IEEE80211_ADDR_LEN]);

static int rt2860_vap_key_delete(struct ieee80211vap *vap,
	const struct ieee80211_key *k);

static void rt2860_vap_update_beacon(struct ieee80211vap *vap, int what);

static int rt2860_media_change(struct ifnet *ifp);

static struct ieee80211_node *rt2860_node_alloc(struct ieee80211vap *vap,
	const uint8_t mac[IEEE80211_ADDR_LEN]);

static void rt2860_node_cleanup(struct ieee80211_node *ni);

static void rt2860_node_getmimoinfo(const struct ieee80211_node *ni,
	struct ieee80211_mimo_info *mi);

static int rt2860_setregdomain(struct ieee80211com *ic,
	struct ieee80211_regdomain *reg,
	int nchans, struct ieee80211_channel chans[]);

static void rt2860_getradiocaps(struct ieee80211com *ic,
	int maxchans, int *nchans, struct ieee80211_channel chans[]);

static void rt2860_scan_start(struct ieee80211com *ic);

static void rt2860_scan_end(struct ieee80211com *ic);

static void rt2860_set_channel(struct ieee80211com *ic);

static void rt2860_newassoc(struct ieee80211_node *ni, int isnew);

static void rt2860_updateslot(struct ifnet *ifp);

static void rt2860_update_promisc(struct ifnet *ifp);

static void rt2860_update_mcast(struct ifnet *ifp);

static int rt2860_wme_update(struct ieee80211com *ic);

static int rt2860_raw_xmit(struct ieee80211_node *ni, struct mbuf *m,
	const struct ieee80211_bpf_params *params);

static int rt2860_recv_action(struct ieee80211_node *ni,
	const struct ieee80211_frame *wh,
	const uint8_t *frm, const uint8_t *efrm);

static int rt2860_send_action(struct ieee80211_node *ni,
	int cat, int act, void *sa);

static int rt2860_addba_response(struct ieee80211_node *ni,
	struct ieee80211_tx_ampdu *tap,
	int status, int baparamset, int batimeout);

static void rt2860_addba_stop(struct ieee80211_node *ni,
	struct ieee80211_tx_ampdu *tap);

static int rt2860_ampdu_rx_start(struct ieee80211_node *ni,
	struct ieee80211_rx_ampdu *rap,
	int baparamset, int batimeout, int baseqctl);

static void rt2860_ampdu_rx_stop(struct ieee80211_node *ni,
	struct ieee80211_rx_ampdu *rap);

static int rt2860_send_bar(struct ieee80211_node *ni,
	struct ieee80211_tx_ampdu *tap, ieee80211_seq seqno);

static void rt2860_amrr_update_iter_func(void *arg, struct ieee80211_node *ni);

static void rt2860_periodic(void *arg);

static void rt2860_tx_watchdog(void *arg);

static int rt2860_staid_alloc(struct rt2860_softc *sc, int aid);

static void rt2860_staid_delete(struct rt2860_softc *sc, int staid);

static void rt2860_asic_set_bssid(struct rt2860_softc *sc,
	const uint8_t *bssid);

static void rt2860_asic_set_macaddr(struct rt2860_softc *sc,
	const uint8_t *addr);

static void rt2860_asic_enable_tsf_sync(struct rt2860_softc *sc);

static void rt2860_asic_disable_tsf_sync(struct rt2860_softc *sc);

static void rt2860_asic_enable_mrr(struct rt2860_softc *sc);

static void rt2860_asic_set_txpreamble(struct rt2860_softc *sc);

static void rt2860_asic_set_basicrates(struct rt2860_softc *sc);

static void rt2860_asic_update_rtsthreshold(struct rt2860_softc *sc);

static void rt2860_asic_update_txpower(struct rt2860_softc *sc);

static void rt2860_asic_update_promisc(struct rt2860_softc *sc);

static void rt2860_asic_updateprot(struct rt2860_softc *sc);

static void rt2860_asic_updateslot(struct rt2860_softc *sc);

static void rt2860_asic_wme_update(struct rt2860_softc *sc);

static void rt2860_asic_update_beacon(struct rt2860_softc *sc,
	struct ieee80211vap *vap);

static void rt2860_asic_clear_keytables(struct rt2860_softc *sc);

static void rt2860_asic_add_ba_session(struct rt2860_softc *sc,
	uint8_t wcid, int tid);

static void rt2860_asic_del_ba_session(struct rt2860_softc *sc,
	uint8_t wcid, int tid);

static int rt2860_beacon_alloc(struct rt2860_softc *sc,
	struct ieee80211vap *vap);

static uint8_t rt2860_rxrate(struct rt2860_rxwi *rxwi);

static uint8_t rt2860_maxrssi_rxpath(struct rt2860_softc *sc,
	const struct rt2860_rxwi *rxwi);

static int8_t rt2860_rssi2dbm(struct rt2860_softc *sc,
	uint8_t rssi, uint8_t rxpath);

static uint8_t rt2860_rate2mcs(uint8_t rate);

static int rt2860_tx_mgmt(struct rt2860_softc *sc,
	struct mbuf *m, struct ieee80211_node *ni, int qid);

static int rt2860_tx_data(struct rt2860_softc *sc,
	struct mbuf *m, struct ieee80211_node *ni, int qid);

/*
static int rt2860_tx_raw(struct rt2860_softc *sc,
	struct mbuf *m, struct ieee80211_node *ni,
	const struct ieee80211_bpf_params *params);
*/

static void rt2860_intr(void *arg);

static void rt2860_tx_coherent_intr(struct rt2860_softc *sc);

static void rt2860_rx_coherent_intr(struct rt2860_softc *sc);

static void rt2860_txrx_coherent_intr(struct rt2860_softc *sc);

static void rt2860_fifo_sta_full_intr(struct rt2860_softc *sc);

static void rt2860_rx_intr(struct rt2860_softc *sc);

static void rt2860_rx_delay_intr(struct rt2860_softc *sc);

static void rt2860_tx_intr(struct rt2860_softc *sc, int qid);

static void rt2860_tx_delay_intr(struct rt2860_softc *sc);

static void rt2860_pre_tbtt_intr(struct rt2860_softc *sc);

static void rt2860_tbtt_intr(struct rt2860_softc *sc);

static void rt2860_mcu_cmd_intr(struct rt2860_softc *sc);

static void rt2860_auto_wakeup_intr(struct rt2860_softc *sc);

static void rt2860_gp_timer_intr(struct rt2860_softc *sc);

static void rt2860_rx_done_task(void *context, int pending);

static void rt2860_tx_done_task(void *context, int pending);

static void rt2860_fifo_sta_full_task(void *context, int pending);

static void rt2860_periodic_task(void *context, int pending);

static int rt2860_rx_eof(struct rt2860_softc *sc, int limit);

static void rt2860_tx_eof(struct rt2860_softc *sc,
	struct rt2860_softc_tx_ring *ring);

static void rt2860_update_stats(struct rt2860_softc *sc);

static void rt2860_bbp_tuning(struct rt2860_softc *sc);

static void rt2860_watchdog(struct rt2860_softc *sc);

static void rt2860_drain_fifo_stats(struct rt2860_softc *sc);

static void rt2860_update_raw_counters(struct rt2860_softc *sc);

static void rt2860_intr_enable(struct rt2860_softc *sc, uint32_t intr_mask);

static void rt2860_intr_disable(struct rt2860_softc *sc, uint32_t intr_mask);

static int rt2860_txrx_enable(struct rt2860_softc *sc);

static int rt2860_alloc_rx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_rx_ring *ring);

static void rt2860_reset_rx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_rx_ring *ring);

static void rt2860_free_rx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_rx_ring *ring);

static int rt2860_alloc_tx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_tx_ring *ring, int qid);

static void rt2860_reset_tx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_tx_ring *ring);

static void rt2860_free_tx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_tx_ring *ring);

static void rt2860_dma_map_addr(void *arg, bus_dma_segment_t *segs,
	int nseg, int error);

static void rt2860_sysctl_attach(struct rt2860_softc *sc);

/*
 * Static variables
 */

static const struct
{
	uint32_t reg;
	uint32_t val;
} rt2860_def_mac[] =
{
	{ RT2860_REG_PBF_BCN_OFFSET0, 			0xf8f0e8e0 },
	{ RT2860_REG_PBF_BCN_OFFSET1, 			0x6f77d0c8 },
	{ RT2860_REG_LEGACY_BASIC_RATE, 		0x0000013f },
	{ RT2860_REG_HT_BASIC_RATE, 			0x00008003 },
	{ RT2860_REG_SYS_CTRL, 					0x00000000 },
	{ RT2860_REG_RX_FILTER_CFG, 			0x00017f97 },
	{ RT2860_REG_BKOFF_SLOT_CFG, 			0x00000209 },
	{ RT2860_REG_TX_SW_CFG0, 				0x00000000 },
	{ RT2860_REG_TX_SW_CFG1, 				0x00080606 },
	{ RT2860_REG_TX_LINK_CFG, 				0x00001020 },
	{ RT2860_REG_TX_TIMEOUT_CFG, 			0x000a2090 },
	{ RT2860_REG_MAX_LEN_CFG, 				(1 << 12) | RT2860_MAX_AGG_SIZE },
	{ RT2860_REG_LED_CFG, 					0x7f031e46 },
	{ RT2860_REG_PBF_MAX_PCNT, 				0x1f3fbf9f },
	{ RT2860_REG_TX_RTY_CFG, 				0x47d01f0f },
	{ RT2860_REG_AUTO_RSP_CFG, 				0x00000013 },
	{ RT2860_REG_TX_CCK_PROT_CFG, 			0x05740003 },
	{ RT2860_REG_TX_OFDM_PROT_CFG, 			0x05740003 },
	{ RT2860_REG_TX_GF20_PROT_CFG, 			0x01744004 },
	{ RT2860_REG_TX_GF40_PROT_CFG, 			0x03f44084 },
	{ RT2860_REG_TX_MM20_PROT_CFG, 			0x01744004 },
	{ RT2860_REG_TX_MM40_PROT_CFG,			0x03f54084 },
	{ RT2860_REG_TX_TXOP_CTRL_CFG, 			0x0000583f },
	{ RT2860_REG_TX_RTS_CFG, 				0x00092b20 },
	{ RT2860_REG_TX_EXP_ACK_TIME, 			0x002400ca },
	{ RT2860_REG_HCCAPSMP_TXOP_HLDR_ET, 	0x00000002 },
	{ RT2860_REG_XIFS_TIME_CFG, 			0x33a41010 },
	{ RT2860_REG_PWR_PIN_CFG, 				0x00000003 },
	{ RT2860_REG_SCHDMA_WMM_AIFSN_CFG,		0x00002273 },
	{ RT2860_REG_SCHDMA_WMM_CWMIN_CFG,		0x00002344 },
	{ RT2860_REG_SCHDMA_WMM_CWMAX_CFG,		0x000034aa },
};

#define RT2860_DEF_MAC_SIZE		(sizeof(rt2860_def_mac) / sizeof(rt2860_def_mac[0]))

static const struct
{
	uint8_t	reg;
	uint8_t	val;
} rt2860_def_bbp[] =
{
	{ 65,	0x2c },
	{ 66,	0x38 },
	{ 69,	0x12 },
	{ 70,	0x0a },
	{ 73,	0x10 },
	{ 81,	0x37 },
	{ 82,	0x62 },
	{ 83,	0x6a },
	{ 84,	0x99 },
	{ 86,	0x00 },
	{ 91,	0x04 },
	{ 92,	0x00 },
	{ 103,	0x00 },
	{ 105,	0x05 },
	{ 106,	0x35 },
};

#define RT2860_DEF_BBP_SIZE		(sizeof(rt2860_def_bbp) / sizeof(rt2860_def_bbp[0]))

SYSCTL_NODE(_hw, OID_AUTO, rt2860, CTLFLAG_RD, 0, "RT2860 driver parameters");

static int rt2860_tx_stbc = 1;
SYSCTL_INT(_hw_rt2860, OID_AUTO, tx_stbc, CTLFLAG_RW, &rt2860_tx_stbc, 0, "RT2860 Tx STBC");
TUNABLE_INT("hw.rt2860.tx_stbc", &rt2860_tx_stbc);

#ifdef RT2860_DEBUG
//static int rt2860_debug = 0xffffffff;
static int rt2860_debug = 0;
SYSCTL_INT(_hw_rt2860, OID_AUTO, debug, CTLFLAG_RW, &rt2860_debug, 0, "RT2860 debug level");
TUNABLE_INT("hw.rt2860.debug", &rt2860_debug);
#endif

/*
 * rt2860_attach
 */
int rt2860_attach(device_t dev)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;
	struct ieee80211com *ic;
	int error, ntries, i;

	sc = device_get_softc(dev);

	sc->dev = dev;

	mtx_init(&sc->lock, device_get_nameunit(dev),
		MTX_NETWORK_LOCK, MTX_DEF | MTX_RECURSE);

	sc->mem = bus_alloc_resource_any(dev, SYS_RES_MEMORY,
		&sc->mem_rid, RF_ACTIVE);
	if (sc->mem == NULL)
	{
		printf("%s: could not allocate memory resource\n",
			device_get_nameunit(dev));
		error = ENXIO;
		goto fail;
	}


	sc->bst = rman_get_bustag(sc->mem);
	sc->bsh = rman_get_bushandle(sc->mem);

	sc->irq_rid = 0;
	sc->irq = bus_alloc_resource_any(dev, SYS_RES_IRQ,
		&sc->irq_rid, RF_ACTIVE | RF_SHAREABLE);
	if (sc->irq == NULL)
	{
		printf("%s: could not allocate interrupt resource\n",
			device_get_nameunit(dev));
		error = ENXIO;
		goto fail;
	}

	sc->tx_stbc = rt2860_tx_stbc;

#ifdef RT2860_DEBUG
	sc->debug = rt2860_debug;

	SYSCTL_ADD_INT(device_get_sysctl_ctx(dev),
		SYSCTL_CHILDREN(device_get_sysctl_tree(dev)), OID_AUTO,
		"debug", CTLFLAG_RW, &sc->debug, 0, "rt2860 debug level");
#endif

	RT2860_DPRINTF(sc, RT2860_DEBUG_ANY,
		"%s: attaching\n",
		device_get_nameunit(sc->dev));

	/* wait for NIC to initialize */

	for (ntries = 0; ntries < 100; ntries++)
	{
		sc->mac_rev = rt2860_io_mac_read(sc, RT2860_REG_MAC_CSR0);
		if (sc->mac_rev != 0x00000000 && sc->mac_rev != 0xffffffff)
			break;

		DELAY(10);
	}

	if (ntries == 100)
	{
		printf("%s: timeout waiting for NIC to initialize\n",
			device_get_nameunit(dev));
		error = EIO;
		goto fail;
	}

	rt2860_read_eeprom(sc);

	printf("%s: MAC/BBP RT2860 (rev 0x%08x), RF %s\n",
	    device_get_nameunit(sc->dev), sc->mac_rev,
		rt2860_rf_name(sc->rf_rev));

	/* clear key tables */

	rt2860_asic_clear_keytables(sc);

	/* allocate Tx and Rx rings */

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
	{
		error = rt2860_alloc_tx_ring(sc, &sc->tx_ring[i], i);
		if (error != 0)
		{
			printf("%s: could not allocate Tx ring #%d\n",
				device_get_nameunit(sc->dev), i);
			goto fail;
		}
	}

	sc->tx_ring_mgtqid = 5;

	error = rt2860_alloc_rx_ring(sc, &sc->rx_ring);
	if (error != 0)
	{
		printf("%s: could not allocate Rx ring\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	callout_init(&sc->periodic_ch, 0);
	callout_init_mtx(&sc->tx_watchdog_ch, &sc->lock, 0);

	ifp = sc->ifp = if_alloc(IFT_IEEE80211);
	if (ifp == NULL)
	{
		printf("%s: could not if_alloc()\n",
			device_get_nameunit(sc->dev));
		error = ENOMEM;
		goto fail;
	}

	ifp->if_softc = sc;

	if_initname(ifp, "rt2860", device_get_unit(sc->dev));

	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;

	ifp->if_init = rt2860_init;
	ifp->if_ioctl = rt2860_ioctl;
	ifp->if_start = rt2860_start;

	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	ifp->if_snd.ifq_drv_maxlen = IFQ_MAXLEN;
	IFQ_SET_READY(&ifp->if_snd);

	ic = ifp->if_l2com;

	ic->ic_ifp = ifp;

	ic->ic_phytype = IEEE80211_T_HT;
	ic->ic_opmode = IEEE80211_M_STA;

	ic->ic_caps = IEEE80211_C_MONITOR |
		IEEE80211_C_IBSS |
		IEEE80211_C_STA |
		IEEE80211_C_AHDEMO |
		IEEE80211_C_HOSTAP |
		IEEE80211_C_WDS |
		IEEE80211_C_MBSS |
		IEEE80211_C_BGSCAN |
		IEEE80211_C_TXPMGT |
	    IEEE80211_C_SHPREAMBLE |
	    IEEE80211_C_SHSLOT |
		IEEE80211_C_TXFRAG |
		IEEE80211_C_BURST |
		IEEE80211_C_WME |
		IEEE80211_C_WPA;

	ic->ic_cryptocaps |= IEEE80211_CRYPTO_WEP |
		IEEE80211_CRYPTO_TKIP |
		IEEE80211_CRYPTO_TKIPMIC |
		IEEE80211_CRYPTO_AES_CCM;

	ic->ic_htcaps = IEEE80211_HTC_HT |
		IEEE80211_HTC_AMSDU |					/* A-MSDU Tx */
		IEEE80211_HTC_AMPDU |					/* A-MPDU Tx */
		IEEE80211_HTC_SMPS |					/* MIMO power save */
		IEEE80211_HTCAP_MAXAMSDU_3839 |			/* max. A-MSDU Rx length */
		IEEE80211_HTCAP_CHWIDTH40 |				/* HT 40MHz channel width */
		IEEE80211_HTCAP_GREENFIELD |			/* HT greenfield */
		IEEE80211_HTCAP_SHORTGI20 |				/* HT 20MHz short GI */
		IEEE80211_HTCAP_SHORTGI40 |				/* HT 40MHz short GI */
		IEEE80211_HTCAP_SMPS_OFF;				/* MIMO power save disabled */

	/* spatial streams */

	if (sc->nrxpath == 2)
		ic->ic_htcaps |= IEEE80211_HTCAP_RXSTBC_2STREAM;
	else if (sc->nrxpath == 3)
		ic->ic_htcaps |= IEEE80211_HTCAP_RXSTBC_3STREAM;
	else
		ic->ic_htcaps |= IEEE80211_HTCAP_RXSTBC_1STREAM;

	if (sc->ntxpath > 1)
		ic->ic_htcaps |= IEEE80211_HTCAP_TXSTBC;

	/* delayed BA */

	if (sc->mac_rev != 0x28600100)
		ic->ic_htcaps |= IEEE80211_HTCAP_DELBA;

	/* init channels */

	ic->ic_nchans = 0;

	rt2860_init_channels(sc);

	rt2860_init_channels_ht40(sc);

	ieee80211_ifattach(ic, sc->mac_addr);

	ic->ic_vap_create = rt2860_vap_create;
	ic->ic_vap_delete = rt2860_vap_delete;

	ic->ic_node_alloc = rt2860_node_alloc;

	sc->node_cleanup = ic->ic_node_cleanup;
	ic->ic_node_cleanup = rt2860_node_cleanup;

	ic->ic_node_getmimoinfo = rt2860_node_getmimoinfo;
	ic->ic_setregdomain = rt2860_setregdomain;
	ic->ic_getradiocaps = rt2860_getradiocaps;
	ic->ic_scan_start = rt2860_scan_start;
	ic->ic_scan_end = rt2860_scan_end;
	ic->ic_set_channel = rt2860_set_channel;
	ic->ic_newassoc = rt2860_newassoc;
	ic->ic_updateslot = rt2860_updateslot;
	ic->ic_update_promisc = rt2860_update_promisc;
	ic->ic_update_mcast = rt2860_update_mcast;
	ic->ic_wme.wme_update = rt2860_wme_update;
	ic->ic_raw_xmit = rt2860_raw_xmit;

	sc->recv_action = ic->ic_recv_action;
	ic->ic_recv_action = rt2860_recv_action;

	sc->send_action = ic->ic_send_action;
	ic->ic_send_action = rt2860_send_action;

	sc->addba_response = ic->ic_addba_response;
	ic->ic_addba_response = rt2860_addba_response;

	sc->addba_stop = ic->ic_addba_stop;
	ic->ic_addba_stop = rt2860_addba_stop;

	sc->ampdu_rx_start = ic->ic_ampdu_rx_start;
	ic->ic_ampdu_rx_start = rt2860_ampdu_rx_start;

	sc->ampdu_rx_stop = ic->ic_ampdu_rx_stop;
	ic->ic_ampdu_rx_stop = rt2860_ampdu_rx_stop;

	/* hardware requires padding between 802.11 frame header and body */

	ic->ic_flags |= IEEE80211_F_DATAPAD | IEEE80211_F_DOTH;

	ic->ic_flags_ext |= IEEE80211_FEXT_SWBMISS;

	ieee80211_radiotap_attach(ic,
	    &sc->txtap.ihdr, sizeof(sc->txtap),
		RT2860_SOFTC_TX_RADIOTAP_PRESENT,
	    &sc->rxtap.ihdr, sizeof(sc->rxtap),
		RT2860_SOFTC_RX_RADIOTAP_PRESENT);

	/* init task queue */

	TASK_INIT(&sc->rx_done_task, 0, rt2860_rx_done_task, sc);
	TASK_INIT(&sc->tx_done_task, 0, rt2860_tx_done_task, sc);
	TASK_INIT(&sc->fifo_sta_full_task, 0, rt2860_fifo_sta_full_task, sc);
	TASK_INIT(&sc->periodic_task, 0, rt2860_periodic_task, sc);

	sc->rx_process_limit = 100;

	sc->taskqueue = taskqueue_create("rt2860_taskq", M_NOWAIT,
	    taskqueue_thread_enqueue, &sc->taskqueue);

	taskqueue_start_threads(&sc->taskqueue, 1, PI_NET, "%s taskq",
	    device_get_nameunit(sc->dev));

	rt2860_sysctl_attach(sc);

	if (bootverbose)
		ieee80211_announce(ic);

	/* set up interrupt */

	error = bus_setup_intr(dev, sc->irq, INTR_TYPE_NET | INTR_MPSAFE,
	    NULL, rt2860_intr, sc, &sc->irqh);
	if (error != 0)
	{
		printf("%s: could not set up interrupt\n",
			device_get_nameunit(dev));
		goto fail;
	}

	return 0;

fail:

	/* free Tx and Rx rings */

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
		rt2860_free_tx_ring(sc, &sc->tx_ring[i]);

	rt2860_free_rx_ring(sc, &sc->rx_ring);

	mtx_destroy(&sc->lock);

	if (sc->mem != NULL)
		bus_release_resource(dev, SYS_RES_MEMORY, sc->mem_rid, sc->mem);

	if (sc->irq != NULL)
		bus_release_resource(dev, SYS_RES_IRQ, sc->irq_rid, sc->irq);

	return error;
}

/*
 * rt2860_detach
 */
int rt2860_detach(device_t dev)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;
	struct ieee80211com *ic;
	int i;

	sc = device_get_softc(dev);
	ifp = sc->ifp;
	ic = ifp->if_l2com;

	RT2860_DPRINTF(sc, RT2860_DEBUG_ANY,
		"%s: detaching\n",
		device_get_nameunit(sc->dev));

	RT2860_SOFTC_LOCK(sc);

	ifp->if_drv_flags &= ~(IFF_DRV_RUNNING | IFF_DRV_OACTIVE);

	callout_stop(&sc->periodic_ch);
	callout_stop(&sc->tx_watchdog_ch);

	taskqueue_drain(sc->taskqueue, &sc->rx_done_task);
	taskqueue_drain(sc->taskqueue, &sc->tx_done_task);
	taskqueue_drain(sc->taskqueue, &sc->fifo_sta_full_task);
	taskqueue_drain(sc->taskqueue, &sc->periodic_task);

	/* free Tx and Rx rings */

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
		rt2860_free_tx_ring(sc, &sc->tx_ring[i]);

	rt2860_free_rx_ring(sc, &sc->rx_ring);

	RT2860_SOFTC_UNLOCK(sc);

	ieee80211_ifdetach(ic);

	if_free(ifp);

	taskqueue_free(sc->taskqueue);

	mtx_destroy(&sc->lock);

	bus_generic_detach(dev);

	bus_teardown_intr(dev, sc->irq, sc->irqh);

	bus_release_resource(dev, SYS_RES_IRQ, sc->irq_rid, sc->irq);

	bus_release_resource(dev, SYS_RES_MEMORY, sc->mem_rid, sc->mem);

	return 0;
}

/*
 * rt2860_shutdown
 */
int rt2860_shutdown(device_t dev)
{
	struct rt2860_softc *sc;

	sc = device_get_softc(dev);

	RT2860_DPRINTF(sc, RT2860_DEBUG_ANY,
		"%s: shutting down\n",
		device_get_nameunit(sc->dev));

	rt2860_stop(sc);

	sc->flags &= ~RT2860_SOFTC_FLAGS_UCODE_LOADED;

	return 0;
}

/*
 * rt2860_suspend
 */
int rt2860_suspend(device_t dev)
{
	struct rt2860_softc *sc;

	sc = device_get_softc(dev);

	RT2860_DPRINTF(sc, RT2860_DEBUG_ANY,
		"%s: suspending\n",
		device_get_nameunit(sc->dev));

	rt2860_stop(sc);

	sc->flags &= ~RT2860_SOFTC_FLAGS_UCODE_LOADED;

	return 0;
}

/*
 * rt2860_resume
 */
int rt2860_resume(device_t dev)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;

	sc = device_get_softc(dev);
	ifp = sc->ifp;

	RT2860_DPRINTF(sc, RT2860_DEBUG_ANY,
		"%s: resuming\n",
		device_get_nameunit(sc->dev));

	if (ifp->if_flags & IFF_UP)
		rt2860_init(sc);

	return 0;
}

/*
 * rt2860_init_channels
 */
static void rt2860_init_channels(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211_channel *c;
	int i, flags;

	ifp = sc->ifp;
	ic = ifp->if_l2com;

	/* set supported channels for 2GHz band */

	for (i = 1; i <= 14; i++)
	{
		c = &ic->ic_channels[ic->ic_nchans++];
		flags = IEEE80211_CHAN_B;

		c->ic_freq = ieee80211_ieee2mhz(i, flags);
		c->ic_ieee = i;
		c->ic_flags = flags;

		c = &ic->ic_channels[ic->ic_nchans++];
		flags = IEEE80211_CHAN_B | IEEE80211_CHAN_HT20;

		c->ic_freq = ieee80211_ieee2mhz(i, flags);
		c->ic_ieee = i;
		c->ic_flags = flags;

		c = &ic->ic_channels[ic->ic_nchans++];
		flags = IEEE80211_CHAN_G;

		c->ic_freq = ieee80211_ieee2mhz(i, flags);
		c->ic_ieee = i;
		c->ic_flags = flags;

		c = &ic->ic_channels[ic->ic_nchans++];
		flags = IEEE80211_CHAN_G | IEEE80211_CHAN_HT20;

		c->ic_freq = ieee80211_ieee2mhz(i, flags);
		c->ic_ieee = i;
		c->ic_flags = flags;
	}

	/* set supported channels for 5GHz band */

	if (sc->rf_rev == RT2860_EEPROM_RF_2850 ||
		sc->rf_rev == RT2860_EEPROM_RF_2750 ||
		sc->rf_rev == RT2860_EEPROM_RF_3052)
	{
		for (i = 36; i <= 64; i += 4)
		{
			c = &ic->ic_channels[ic->ic_nchans++];
			flags = IEEE80211_CHAN_A;

			c->ic_freq = ieee80211_ieee2mhz(i, flags);
			c->ic_ieee = i;
			c->ic_flags = flags;

			c = &ic->ic_channels[ic->ic_nchans++];
			flags = IEEE80211_CHAN_A | IEEE80211_CHAN_HT20;

			c->ic_freq = ieee80211_ieee2mhz(i, flags);
			c->ic_ieee = i;
			c->ic_flags = flags;
		}

		for (i = 100; i <= 140; i += 4)
		{
			c = &ic->ic_channels[ic->ic_nchans++];
			flags = IEEE80211_CHAN_A;

			c->ic_freq = ieee80211_ieee2mhz(i, flags);
			c->ic_ieee = i;
			c->ic_flags = flags;

			c = &ic->ic_channels[ic->ic_nchans++];
			flags = IEEE80211_CHAN_A | IEEE80211_CHAN_HT20;

			c->ic_freq = ieee80211_ieee2mhz(i, flags);
			c->ic_ieee = i;
			c->ic_flags = flags;
		}

		for (i = 149; i <= 165; i += 4)
		{
			c = &ic->ic_channels[ic->ic_nchans++];
			flags = IEEE80211_CHAN_A;

			c->ic_freq = ieee80211_ieee2mhz(i, flags);
			c->ic_ieee = i;
			c->ic_flags = flags;

			c = &ic->ic_channels[ic->ic_nchans++];
			flags = IEEE80211_CHAN_A | IEEE80211_CHAN_HT20;

			c->ic_freq = ieee80211_ieee2mhz(i, flags);
			c->ic_ieee = i;
			c->ic_flags = flags;
		}
	}
}

/*
 * rt2860_init_channels_ht40
 */
static void rt2860_init_channels_ht40(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211_channel *c, *cent, *ext;
	int i, flags;

	ifp = sc->ifp;
	ic = ifp->if_l2com;

	/* set supported channels for 2GHz band */

	for (i = 1; i <= 14; i++)
	{
		flags = IEEE80211_CHAN_G | IEEE80211_CHAN_HT40;

		/* find the center channel */

		cent = ieee80211_find_channel_byieee(ic, i,
			flags & ~IEEE80211_CHAN_HT);
		if (cent == NULL)
		{
			printf("%s: skip channel %d, could not find center channel\n",
				device_get_nameunit(sc->dev), i);
			continue;
		}

		/* find the extension channel */

		ext = ieee80211_find_channel(ic, cent->ic_freq + 20,
			flags & ~IEEE80211_CHAN_HT);
		if (ext == NULL)
		{
			printf("%s: skip channel %d, could not find extension channel\n",
				device_get_nameunit(sc->dev), i);
			continue;
		}

		c = &ic->ic_channels[ic->ic_nchans++];

		*c = *cent;
		c->ic_extieee = ext->ic_ieee;
		c->ic_flags &= ~IEEE80211_CHAN_HT;
		c->ic_flags |= IEEE80211_CHAN_HT40U;

		c = &ic->ic_channels[ic->ic_nchans++];

		*c = *ext;
		c->ic_extieee = cent->ic_ieee;
		c->ic_flags &= ~IEEE80211_CHAN_HT;
		c->ic_flags |= IEEE80211_CHAN_HT40D;
	}

	/* set supported channels for 5GHz band */

	if (sc->rf_rev == RT2860_EEPROM_RF_2850 ||
		sc->rf_rev == RT2860_EEPROM_RF_2750 ||
		sc->rf_rev == RT2860_EEPROM_RF_3052)
	{
		for (i = 36; i <= 64; i += 4)
		{
			flags = IEEE80211_CHAN_A | IEEE80211_CHAN_HT40;

			/* find the center channel */

			cent = ieee80211_find_channel_byieee(ic, i,
				flags & ~IEEE80211_CHAN_HT);
			if (cent == NULL)
			{
				printf("%s: skip channel %d, could not find center channel\n",
					device_get_nameunit(sc->dev), i);
				continue;
			}

			/* find the extension channel */

			ext = ieee80211_find_channel(ic, cent->ic_freq + 20,
				flags & ~IEEE80211_CHAN_HT);
			if (ext == NULL)
			{
				printf("%s: skip channel %d, could not find extension channel\n",
					device_get_nameunit(sc->dev), i);
				continue;
			}

			c = &ic->ic_channels[ic->ic_nchans++];

			*c = *cent;
			c->ic_extieee = ext->ic_ieee;
			c->ic_flags &= ~IEEE80211_CHAN_HT;
			c->ic_flags |= IEEE80211_CHAN_HT40U;

			c = &ic->ic_channels[ic->ic_nchans++];

			*c = *ext;
			c->ic_extieee = cent->ic_ieee;
			c->ic_flags &= ~IEEE80211_CHAN_HT;
			c->ic_flags |= IEEE80211_CHAN_HT40D;
		}

		for (i = 100; i <= 140; i += 4)
		{
			flags = IEEE80211_CHAN_A | IEEE80211_CHAN_HT40;

			/* find the center channel */

			cent = ieee80211_find_channel_byieee(ic, i,
				flags & ~IEEE80211_CHAN_HT);
			if (cent == NULL)
			{
				printf("%s: skip channel %d, could not find center channel\n",
					device_get_nameunit(sc->dev), i);
				continue;
			}

			/* find the extension channel */

			ext = ieee80211_find_channel(ic, cent->ic_freq + 20,
				flags & ~IEEE80211_CHAN_HT);
			if (ext == NULL)
			{
				printf("%s: skip channel %d, could not find extension channel\n",
					device_get_nameunit(sc->dev), i);
				continue;
			}

			c = &ic->ic_channels[ic->ic_nchans++];

			*c = *cent;
			c->ic_extieee = ext->ic_ieee;
			c->ic_flags &= ~IEEE80211_CHAN_HT;
			c->ic_flags |= IEEE80211_CHAN_HT40U;

			c = &ic->ic_channels[ic->ic_nchans++];

			*c = *ext;
			c->ic_extieee = cent->ic_ieee;
			c->ic_flags &= ~IEEE80211_CHAN_HT;
			c->ic_flags |= IEEE80211_CHAN_HT40D;
		}

		for (i = 149; i <= 165; i += 4)
		{
			flags = IEEE80211_CHAN_A | IEEE80211_CHAN_HT40;

			/* find the center channel */

			cent = ieee80211_find_channel_byieee(ic, i,
				flags & ~IEEE80211_CHAN_HT);
			if (cent == NULL)
			{
				printf("%s: skip channel %d, could not find center channel\n",
					device_get_nameunit(sc->dev), i);
				continue;
			}

			/* find the extension channel */

			ext = ieee80211_find_channel(ic, cent->ic_freq + 20,
				flags & ~IEEE80211_CHAN_HT);
			if (ext == NULL)
			{
				printf("%s: skip channel %d, could not find extension channel\n",
					device_get_nameunit(sc->dev), i);
				continue;
			}

			c = &ic->ic_channels[ic->ic_nchans++];

			*c = *cent;
			c->ic_extieee = ext->ic_ieee;
			c->ic_flags &= ~IEEE80211_CHAN_HT;
			c->ic_flags |= IEEE80211_CHAN_HT40U;

			c = &ic->ic_channels[ic->ic_nchans++];

			*c = *ext;
			c->ic_extieee = cent->ic_ieee;
			c->ic_flags &= ~IEEE80211_CHAN_HT;
			c->ic_flags |= IEEE80211_CHAN_HT40D;
		}
	}
}

/*
 * rt2860_init_locked
 */
static void rt2860_init_locked(void *priv)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;
	struct ieee80211com *ic;
	int error, i, ntries;
	uint32_t tmp, stacnt[6];

	sc = priv;
	ifp = sc->ifp;
	ic = ifp->if_l2com;

	RT2860_DPRINTF(sc, RT2860_DEBUG_ANY,
		"%s: initializing\n",
		device_get_nameunit(sc->dev));

	RT2860_SOFTC_ASSERT_LOCKED(sc);

	if (sc->mac_rev != 0x28720200)
	{
	    if (!(sc->flags & RT2860_SOFTC_FLAGS_UCODE_LOADED))
	    {
		RT2860_DPRINTF(sc, RT2860_DEBUG_ANY,
			"%s: loading 8051 microcode\n",
			device_get_nameunit(sc->dev));

		error = rt2860_io_mcu_load_ucode(sc, rt2860_ucode, sizeof(rt2860_ucode));
		if (error != 0)
		{
			printf("%s: could not load 8051 microcode\n",
				device_get_nameunit(sc->dev));
			goto fail;
		}

		RT2860_DPRINTF(sc, RT2860_DEBUG_ANY,
			"%s: 8051 microcode was successfully loaded\n",
			device_get_nameunit(sc->dev));

		sc->flags |= RT2860_SOFTC_FLAGS_UCODE_LOADED;
	    }
	}
	else
	{
		sc->flags |= RT2860_SOFTC_FLAGS_UCODE_LOADED;

		/* Blink every TX */
#define LED_CFG_LED_POLARITY	(1<<30)
#define LED_CFG_Y_LED_MODE_ONTX	(1<<28)
#define LED_CFG_G_LED_MODE_ONTX	(1<<26)
#define LED_CFG_R_LED_MODE_ONTX	(1<<24)
#define LED_CFG_SLOW_BLK_TIME 	(0x03<<16) /* sec */
#define LED_CFG_LED_OFF_TIME 	(0x1e<<8) /* msec */
#define LED_CFG_LED_ON_TIME 	(0x46) /* msec */
		rt2860_io_mac_write(sc, RT2860_REG_LED_CFG, 
		    LED_CFG_LED_POLARITY |
		    LED_CFG_Y_LED_MODE_ONTX |
		    LED_CFG_G_LED_MODE_ONTX |
		    LED_CFG_R_LED_MODE_ONTX |
		    LED_CFG_SLOW_BLK_TIME |
		    LED_CFG_LED_OFF_TIME |
		    LED_CFG_LED_ON_TIME);
	}

	rt2860_io_mac_write(sc, RT2860_REG_PWR_PIN_CFG, 0x2);

	/* disable DMA engine */

	tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG);

	tmp &= 0xff0;
	tmp |= RT2860_REG_TX_WB_DDONE;

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG, tmp);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WPDMA_RST_IDX, 0xffffffff);

	/* PBF hardware reset */

	rt2860_io_mac_write(sc, RT2860_REG_PBF_SYS_CTRL, 0xe1f);
	rt2860_io_mac_write(sc, RT2860_REG_PBF_SYS_CTRL, 0xe00);

	/* wait while DMA engine is busy */

	for (ntries = 0; ntries < 100; ntries++)
	{
		tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG);
		if (!(tmp & (RT2860_REG_TX_DMA_BUSY | RT2860_REG_RX_DMA_BUSY)))
			break;

		DELAY(1000);
	}

	if (ntries == 100)
	{
		printf("%s: timeout waiting for DMA engine\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	tmp &= 0xff0;
	tmp |= RT2860_REG_TX_WB_DDONE;

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG, tmp);

	/* reset Rx and Tx rings */

	tmp = RT2860_REG_RST_IDX_RX |
		RT2860_REG_RST_IDX_TX_MGMT |
		RT2860_REG_RST_IDX_TX_HCCA |
		RT2860_REG_RST_IDX_TX_AC3 |
		RT2860_REG_RST_IDX_TX_AC2 |
		RT2860_REG_RST_IDX_TX_AC1 |
		RT2860_REG_RST_IDX_TX_AC0;

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WPDMA_RST_IDX, tmp);

	/* PBF hardware reset */

	rt2860_io_mac_write(sc, RT2860_REG_PBF_SYS_CTRL, 0xe1f);
	rt2860_io_mac_write(sc, RT2860_REG_PBF_SYS_CTRL, 0xe00);

	rt2860_io_mac_write(sc, RT2860_REG_PWR_PIN_CFG, 0x3);

	rt2860_io_mac_write(sc, RT2860_REG_SYS_CTRL,
		RT2860_REG_MAC_SRST | RT2860_REG_BBP_HRST);
	rt2860_io_mac_write(sc, RT2860_REG_SYS_CTRL, 0);

	/* init Tx power per rate */

	for (i = 0; i < RT2860_SOFTC_TXPOW_RATE_COUNT; i++)
	{
		if (sc->txpow_rate_20mhz[i] == 0xffffffff)
			continue;

		rt2860_io_mac_write(sc, RT2860_REG_TX_PWR_CFG(i),
			sc->txpow_rate_20mhz[i]);
	}

	for (i = 0; i < RT2860_DEF_MAC_SIZE; i++)
		rt2860_io_mac_write(sc, rt2860_def_mac[i].reg,
			rt2860_def_mac[i].val);

	/* wait while MAC is busy */

	for (ntries = 0; ntries < 100; ntries++)
	{
		if (!(rt2860_io_mac_read(sc, RT2860_REG_STATUS_CFG) &
			(RT2860_REG_STATUS_TX_BUSY | RT2860_REG_STATUS_RX_BUSY)))
			break;

		DELAY(1000);
	}

	if (ntries == 100)
	{
		printf("%s: timeout waiting for MAC\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}
	
	/* clear Host to MCU mailbox */

	rt2860_io_mac_write(sc, RT2860_REG_H2M_MAILBOX_BBP_AGENT, 0);
	rt2860_io_mac_write(sc, RT2860_REG_H2M_MAILBOX, 0);

	rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_BOOT,
		RT2860_REG_H2M_TOKEN_NO_INTR, 0);

	DELAY(1000);

	error = rt2860_init_bbp(sc);
	if (error != 0)
		goto fail;

	/* set up maximum buffer sizes */

	tmp = (1 << 12) | RT2860_MAX_AGG_SIZE;

	rt2860_io_mac_write(sc, RT2860_REG_MAX_LEN_CFG, tmp);

	if (sc->mac_rev == 0x28720200)
	{
		/* set max. PSDU length from 16K to 32K bytes */

		tmp = rt2860_io_mac_read(sc, RT2860_REG_MAX_LEN_CFG);

		tmp &= ~(3 << 12);
		tmp |= (2 << 12);

		rt2860_io_mac_write(sc, RT2860_REG_MAX_LEN_CFG, tmp);
	}

	if (sc->mac_rev >= 0x28720200 && sc->mac_rev < 0x30700200)
	{
		tmp = rt2860_io_mac_read(sc, RT2860_REG_MAX_LEN_CFG);

		tmp &= 0xfff;
		tmp |= 0x2000;

		rt2860_io_mac_write(sc, RT2860_REG_MAX_LEN_CFG, tmp);
	}

	/* set mac address */

	rt2860_asic_set_macaddr(sc, IF_LLADDR(ifp));

	/* clear statistic registers */

	rt2860_io_mac_read_multi(sc, RT2860_REG_RX_STA_CNT0,
		stacnt, sizeof(stacnt));

	/* set RTS threshold */

	rt2860_asic_update_rtsthreshold(sc);

	/* set Tx power */

	rt2860_asic_update_txpower(sc);

	/* set up protection mode */

	sc->tx_ampdu_sessions = 0;

	rt2860_asic_updateprot(sc);

	/* clear beacon frame space (entries = 8, entry size = 512) */

	rt2860_io_mac_set_region_4(sc, RT2860_REG_BEACON_BASE(0), 0, 1024);

	taskqueue_unblock(sc->taskqueue);

	/* init Tx rings (4 EDCAs + HCCA + MGMT) */

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
		rt2860_reset_tx_ring(sc, &sc->tx_ring[i]);

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
	{
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_BASE_PTR(i),
			sc->tx_ring[i].desc_phys_addr);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_MAX_CNT(i),
			RT2860_SOFTC_TX_RING_DESC_COUNT);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_CTX_IDX(i), 0);
	}

	/* init Rx ring */

	rt2860_reset_rx_ring(sc, &sc->rx_ring);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_BASE_PTR,
		sc->rx_ring.desc_phys_addr);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_MAX_CNT,
		RT2860_SOFTC_RX_RING_DATA_COUNT);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_CALC_IDX,
		RT2860_SOFTC_RX_RING_DATA_COUNT - 1);

	/* wait while DMA engine is busy */

	for (ntries = 0; ntries < 100; ntries++)
	{
		tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG);
		if (!(tmp & (RT2860_REG_TX_DMA_BUSY | RT2860_REG_RX_DMA_BUSY)))
			break;

		DELAY(1000);
	}

	if (ntries == 100)
	{
		printf("%s: timeout waiting for DMA engine\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	tmp &= 0xff0;
	tmp |= RT2860_REG_TX_WB_DDONE;

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG, tmp);

	/* disable interrupts mitigation */

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_DELAY_INT_CFG, 0);

	/* select Main antenna for 1T1R devices */
	if (sc->rf_rev == RT2860_EEPROM_RF_2020 ||
	    sc->rf_rev == RT2860_EEPROM_RF_3020 ||
	    sc->rf_rev == RT2860_EEPROM_RF_3320)
		rt3090_set_rx_antenna(sc, 0);

	/* send LEDs operating mode to microcontroller */
	rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_LED1,
		RT2860_REG_H2M_TOKEN_NO_INTR, sc->led_off[0]);
	rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_LED2,
		RT2860_REG_H2M_TOKEN_NO_INTR, sc->led_off[1]);
	rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_LED3,
		RT2860_REG_H2M_TOKEN_NO_INTR, sc->led_off[2]);

	/* turn radio LED on */

	rt2860_led_cmd(sc, RT2860_LED_CMD_RADIO_ON);

	/* write vendor-specific BBP values (from EEPROM) */

	for (i = 0; i < RT2860_SOFTC_BBP_EEPROM_COUNT; i++)
	{
		if (sc->bbp_eeprom[i].reg == 0x00 ||
			sc->bbp_eeprom[i].reg == 0xff)
			continue;

		rt2860_io_bbp_write(sc, sc->bbp_eeprom[i].reg,
			sc->bbp_eeprom[i].val);
	}

	if ((sc->mac_rev & 0xffff0000) >= 0x30710000)
		rt3090_rf_init(sc);

	if (sc->mac_rev != 0x28720200) {
		/* 0x28720200 don`t have RT2860_REG_SCHDMA_GPIO_CTRL_CFG */
		tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_GPIO_CTRL_CFG);
	        if (tmp & (1 << 2)) {
			rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_SLEEP,
				RT2860_REG_H2M_TOKEN_RADIOOFF, 0x02ff);
			rt2860_io_mcu_cmd_check(sc, RT2860_REG_H2M_TOKEN_RADIOOFF);

			rt2860_io_mcu_cmd(sc, RT2860_IO_MCU_CMD_WAKEUP,
				RT2860_REG_H2M_TOKEN_WAKEUP, 0);
			rt2860_io_mcu_cmd_check(sc, RT2860_REG_H2M_TOKEN_WAKEUP);
		}
	}

	if ((sc->mac_rev & 0xffff0000) >= 0x30710000)
		rt3090_rf_wakeup(sc);

	/* disable non-existing Rx chains */

	tmp = rt2860_io_bbp_read(sc, 3);

	tmp &= ~((1 << 4) | (1 << 3));

	if (sc->nrxpath == 3)
		tmp |= (1 << 4);
	else if (sc->nrxpath == 2)
		tmp |= (1 << 3);

	rt2860_io_bbp_write(sc, 3, tmp);

	/* disable non-existing Tx chains */

	tmp = rt2860_io_bbp_read(sc, 1);

	tmp &= ~((1 << 4) | (1 << 3));

	if (sc->ntxpath == 2)
		tmp |= (1 << 4);

	rt2860_io_bbp_write(sc, 1, tmp);

	if ((sc->mac_rev & 0xffff0000) >= 0x30710000)
		rt3090_rf_setup(sc);

	if (sc->rf_rev == RT2860_EEPROM_RF_3022)
	{
		/* calibrate RF */
                tmp = rt2860_io_rf_read(sc, 30);
                tmp |= 0x80;
                rt2860_io_rf_write(sc, 30, tmp);
                DELAY(1000);
                tmp &= 0x7F;
                rt2860_io_rf_write(sc, 30, tmp);

                /* Initialize RF register to default value */
		rt2860_io_rf_load_defaults(sc);
	}

	/* set current channel */
	rt2860_rf_set_chan(sc, ic->ic_curchan);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WMM_TXOP0_CFG, 0);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WMM_TXOP1_CFG,
		(48 << 16) | 96);

	if ((sc->mac_rev & 0xffff) != 0x0101)
		rt2860_io_mac_write(sc, RT2860_REG_TX_TXOP_CTRL_CFG, 0x583f);

	/* clear pending interrupts */

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_INT_STATUS, 0xffffffff);

	/* enable interrupts */

	tmp = RT2860_REG_INT_TX_COHERENT |
		RT2860_REG_INT_RX_COHERENT |
		RT2860_REG_INT_GP_TIMER |
		RT2860_REG_INT_AUTO_WAKEUP |
		RT2860_REG_INT_FIFO_STA_FULL |
		RT2860_REG_INT_PRE_TBTT |
		RT2860_REG_INT_TBTT |
		RT2860_REG_INT_TXRX_COHERENT |
		RT2860_REG_INT_MCU_CMD |
		RT2860_REG_INT_TX_MGMT_DONE |
		RT2860_REG_INT_TX_HCCA_DONE |
		RT2860_REG_INT_TX_AC3_DONE |
		RT2860_REG_INT_TX_AC2_DONE |
		RT2860_REG_INT_TX_AC1_DONE |
		RT2860_REG_INT_TX_AC0_DONE |
		RT2860_REG_INT_RX_DONE;

	sc->intr_enable_mask = tmp;

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_INT_MASK, tmp);

	if (rt2860_txrx_enable(sc) != 0)
		goto fail;

	/* clear garbage interrupts */

	tmp = rt2860_io_mac_read(sc, 0x1300);

	ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;
	ifp->if_drv_flags |= IFF_DRV_RUNNING;

	sc->periodic_round = 0;

	callout_reset(&sc->periodic_ch, hz / 10, rt2860_periodic, sc);

	return;

fail:

	rt2860_stop_locked(sc);
}

/*
 * rt2860_init
 */
static void rt2860_init(void *priv)
{
	struct rt2860_softc *sc;

	sc = priv;

	RT2860_SOFTC_LOCK(sc);

	rt2860_init_locked(sc);

	RT2860_SOFTC_UNLOCK(sc);
}

/*
 * rt2860_init_bbp
 */
static int rt2860_init_bbp(struct rt2860_softc *sc)
{
	int ntries, i;
	uint8_t tmp;

	for (ntries = 0; ntries < 20; ntries++)
	{
		tmp = rt2860_io_bbp_read(sc, 0);
		if (tmp != 0x00 && tmp != 0xff)
			break;
	}

	if (tmp == 0x00 || tmp == 0xff)
	{
		printf("%s: timeout waiting for BBP to wakeup\n",
			device_get_nameunit(sc->dev));
		return ETIMEDOUT;
	}

	for (i = 0; i < RT2860_DEF_BBP_SIZE; i++)
		rt2860_io_bbp_write(sc, rt2860_def_bbp[i].reg,
			rt2860_def_bbp[i].val);

	if ((sc->mac_rev & 0xffff) != 0x0101)
		rt2860_io_bbp_write(sc, 84, 0x19);

	if (sc->mac_rev == 0x28600100)
	{
		rt2860_io_bbp_write(sc, 69, 0x16);
		rt2860_io_bbp_write(sc, 73, 0x12);
	}

	return 0;
}

/*
 * rt2860_stop_locked
 */
static void rt2860_stop_locked(void *priv)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;
	struct ieee80211com *ic;
	uint32_t tmp;

	sc = priv;
	ifp = sc->ifp;
	ic = ifp->if_l2com;

	RT2860_DPRINTF(sc, RT2860_DEBUG_ANY,
		"%s: stopping\n",
		device_get_nameunit(sc->dev));

	RT2860_SOFTC_ASSERT_LOCKED(sc);

	sc->tx_timer = 0;

	if (ifp->if_drv_flags & IFF_DRV_RUNNING)
		rt2860_led_cmd(sc, RT2860_LED_CMD_RADIO_OFF);

	ifp->if_drv_flags &= ~(IFF_DRV_RUNNING | IFF_DRV_OACTIVE);

	callout_stop(&sc->periodic_ch);
	callout_stop(&sc->tx_watchdog_ch);

	RT2860_SOFTC_UNLOCK(sc);

	taskqueue_block(sc->taskqueue);

	taskqueue_drain(sc->taskqueue, &sc->rx_done_task);
	taskqueue_drain(sc->taskqueue, &sc->tx_done_task);
	taskqueue_drain(sc->taskqueue, &sc->fifo_sta_full_task);
	taskqueue_drain(sc->taskqueue, &sc->periodic_task);

	RT2860_SOFTC_LOCK(sc);

	/* clear key tables */

	rt2860_asic_clear_keytables(sc);

	/* disable interrupts */

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_INT_MASK, 0);

	/* disable Tx/Rx */

	tmp = rt2860_io_mac_read(sc, RT2860_REG_SYS_CTRL);

	tmp &= ~(RT2860_REG_RX_ENABLE | RT2860_REG_TX_ENABLE);

	rt2860_io_mac_write(sc, RT2860_REG_SYS_CTRL, tmp);

	/* reset adapter */

	rt2860_io_mac_write(sc, RT2860_REG_SYS_CTRL,
		RT2860_REG_MAC_SRST | RT2860_REG_BBP_HRST);
	rt2860_io_mac_write(sc, RT2860_REG_SYS_CTRL, 0);
}

/*
 * rt2860_stop
 */
static void rt2860_stop(void *priv)
{
	struct rt2860_softc *sc;

	sc = priv;

	RT2860_SOFTC_LOCK(sc);

	rt2860_stop_locked(sc);

	RT2860_SOFTC_UNLOCK(sc);
}

/*
 * rt2860_start
 */
static void rt2860_start(struct ifnet *ifp)
{
	struct rt2860_softc *sc;
	struct ieee80211_node *ni;
	struct mbuf *m;
	int qid;

	sc = ifp->if_softc;

	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
		return;

	for (;;)
	{
		IFQ_DRV_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		ni = (struct ieee80211_node *) m->m_pkthdr.rcvif;

		m->m_pkthdr.rcvif = NULL;

		qid = M_WME_GETAC(m);

		RT2860_SOFTC_TX_RING_LOCK(&sc->tx_ring[qid]);

		if (sc->tx_ring[qid].data_queued >= RT2860_SOFTC_TX_RING_DATA_COUNT)
		{
			RT2860_SOFTC_TX_RING_UNLOCK(&sc->tx_ring[qid]);

			RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
				"%s: if_start: Tx ring with qid=%d is full\n",
				device_get_nameunit(sc->dev), qid);

			m_freem(m);
			ieee80211_free_node(ni);

			ifp->if_drv_flags |= IFF_DRV_OACTIVE;
			ifp->if_oerrors++;

			sc->tx_data_queue_full[qid]++;

			break;
		}

		if (rt2860_tx_data(sc, m, ni, qid) != 0)
		{
			RT2860_SOFTC_TX_RING_UNLOCK(&sc->tx_ring[qid]);

			ieee80211_free_node(ni);

			ifp->if_oerrors++;

			break;
		}

		RT2860_SOFTC_TX_RING_UNLOCK(&sc->tx_ring[qid]);

		rt2860_drain_fifo_stats(sc);

		sc->tx_timer = RT2860_TX_WATCHDOG_TIMEOUT;

		callout_reset(&sc->tx_watchdog_ch, hz, rt2860_tx_watchdog, sc);
	}
}

/*
 * rt2860_ioctl
 */
static int rt2860_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifreq *ifr;
	int error, startall;

	sc = ifp->if_softc;
	ic = ifp->if_l2com;
	ifr = (struct ifreq *) data;

	error = 0;

	switch (cmd)
	{
		case SIOCSIFFLAGS:
			startall = 0;

			RT2860_SOFTC_LOCK(sc);

			if (ifp->if_flags & IFF_UP)
			{
				if (ifp->if_drv_flags & IFF_DRV_RUNNING)
				{
					if ((ifp->if_flags ^ sc->if_flags) & IFF_PROMISC)
						rt2860_asic_update_promisc(sc);
				}
				else
				{
					rt2860_init_locked(sc);
					startall = 1;
				}
			}
			else
			{
				if (ifp->if_drv_flags & IFF_DRV_RUNNING)
					rt2860_stop_locked(sc);
			}

			sc->if_flags = ifp->if_flags;

			RT2860_SOFTC_UNLOCK(sc);

			if (startall)
				ieee80211_start_all(ic);
		break;

		case SIOCGIFMEDIA:
		case SIOCSIFMEDIA:
			error = ifmedia_ioctl(ifp, ifr, &ic->ic_media, cmd);
		break;

		case SIOCGIFADDR:
			error = ether_ioctl(ifp, cmd, data);
		break;

		default:
			error = EINVAL;
		break;
	}

	return error;
}

/*
 * rt2860_vap_create
 */
static struct ieee80211vap *rt2860_vap_create(struct ieee80211com *ic,
	const char name[IFNAMSIZ], int unit, int opmode, int flags,
	const uint8_t bssid[IEEE80211_ADDR_LEN],
	const uint8_t mac[IEEE80211_ADDR_LEN])
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;
	struct rt2860_softc_vap *rvap;
	struct ieee80211vap *vap;

	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATE,
		"%s: VAP create: opmode=%s\n",
		device_get_nameunit(sc->dev),
		ieee80211_opmode_name[opmode]);

	switch (opmode)
	{
		case IEEE80211_M_IBSS:
		case IEEE80211_M_STA:
		case IEEE80211_M_AHDEMO:
		case IEEE80211_M_HOSTAP:
		case IEEE80211_M_MBSS:
			if ((sc->napvaps + sc->nadhocvaps + sc->nstavaps) != 0)
			{
				if_printf(ifp, "only 1 VAP supported\n");
				return NULL;
			}
		
			if (opmode == IEEE80211_M_STA)
				flags |= IEEE80211_CLONE_NOBEACONS;
		break;

		case IEEE80211_M_WDS:
			if (sc->napvaps == 0)
			{
				if_printf(ifp, "WDS only supported in AP mode\n");
				return NULL;
			}
		break;

		case IEEE80211_M_MONITOR:
		break;

		default:
			if_printf(ifp, "unknown opmode %d\n", opmode);
			return NULL;
	}

	rvap = (struct rt2860_softc_vap *) malloc(sizeof(struct rt2860_softc_vap),
		M_80211_VAP, M_NOWAIT | M_ZERO);
	if (rvap == NULL)
		return NULL;

	vap = &rvap->vap;

	ieee80211_vap_setup(ic, vap, name, unit, opmode, flags, bssid, mac);

	rvap->newstate = vap->iv_newstate;
	vap->iv_newstate = rt2860_vap_newstate;

	vap->iv_reset = rt2860_vap_reset;
	vap->iv_key_update_begin = rt2860_vap_key_update_begin;
	vap->iv_key_update_end = rt2860_vap_key_update_end;
	vap->iv_key_set = rt2860_vap_key_set;
	vap->iv_key_delete = rt2860_vap_key_delete;
	vap->iv_update_beacon = rt2860_vap_update_beacon;

	rt2860_amrr_init(&rvap->amrr, vap,
		sc->ntxpath,
		RT2860_AMRR_MIN_SUCCESS_THRESHOLD,
		RT2860_AMRR_MAX_SUCCESS_THRESHOLD,
		500);

	vap->iv_max_aid = RT2860_SOFTC_STAID_COUNT;

	/* overwrite default Rx A-MPDU factor */

	vap->iv_ampdu_rxmax = IEEE80211_HTCAP_MAXRXAMPDU_32K;
	vap->iv_ampdu_density = IEEE80211_HTCAP_MPDUDENSITY_NA;
	vap->iv_ampdu_limit = vap->iv_ampdu_rxmax;

	ieee80211_vap_attach(vap, rt2860_media_change, ieee80211_media_status);

	switch (vap->iv_opmode)
	{
		case IEEE80211_M_HOSTAP:
		case IEEE80211_M_MBSS:
		case IEEE80211_M_AHDEMO:
			sc->napvaps++;
		break;

		case IEEE80211_M_IBSS:
			sc->nadhocvaps++;
		break;

		case IEEE80211_M_STA:
			sc->nstavaps++;
		break;

		case IEEE80211_M_WDS:
			sc->nwdsvaps++;
		break;

		default:
		break;
	}

	sc->nvaps++;

	if (sc->napvaps > 0)
		ic->ic_opmode = IEEE80211_M_HOSTAP;
	else if (sc->nadhocvaps > 0)
		ic->ic_opmode = IEEE80211_M_IBSS;
	else if (sc->nstavaps > 0)
		ic->ic_opmode = IEEE80211_M_STA;
	else
		ic->ic_opmode = opmode;

	return vap;
}

/*
 * rt2860_vap_delete
 */
static void rt2860_vap_delete(struct ieee80211vap *vap)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_vap *rvap;
	enum ieee80211_opmode opmode;

	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rvap = (struct rt2860_softc_vap *) vap;
	opmode = vap->iv_opmode;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATE,
		"%s: VAP delete: opmode=%s\n",
		device_get_nameunit(sc->dev), ieee80211_opmode_name[opmode]);

	rt2860_amrr_cleanup(&rvap->amrr);

	ieee80211_vap_detach(vap);

	if (rvap->beacon_mbuf != NULL)
	{
		m_free(rvap->beacon_mbuf);
		rvap->beacon_mbuf = NULL;
	}

	switch (opmode)
	{
		case IEEE80211_M_HOSTAP:
		case IEEE80211_M_MBSS:
		case IEEE80211_M_AHDEMO:
			sc->napvaps--;
		break;

		case IEEE80211_M_IBSS:
			sc->nadhocvaps--;
		break;

		case IEEE80211_M_STA:
			sc->nstavaps--;
		break;

		case IEEE80211_M_WDS:
			sc->nwdsvaps--;
		break;

		default:
		break;
	}

	sc->nvaps--;

	if (sc->napvaps > 0)
		ic->ic_opmode = IEEE80211_M_HOSTAP;
	else if (sc->nadhocvaps > 0)
		ic->ic_opmode = IEEE80211_M_IBSS;
	else if (sc->nstavaps > 0)
		ic->ic_opmode = IEEE80211_M_STA;

	free(rvap, M_80211_VAP);
}

/*
 * rt2860_reset_vap
 */
static int rt2860_vap_reset(struct ieee80211vap *vap, u_long cmd)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_vap *rvap;
	int error;

	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rvap = (struct rt2860_softc_vap *) vap;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATE,
		"%s: VAP reset: cmd=%lu\n",
		device_get_nameunit(sc->dev), cmd);

	error = 0;

	switch (cmd)
	{
		case IEEE80211_IOC_RTSTHRESHOLD:
		case IEEE80211_IOC_AMSDU:
			rt2860_asic_update_rtsthreshold(sc);
		break;

		case IEEE80211_IOC_PROTMODE:
		case IEEE80211_IOC_HTPROTMODE:
			rt2860_asic_updateprot(sc);
		break;

		case IEEE80211_IOC_TXPOWER:
			rt2860_asic_update_txpower(sc);
		break;

		case IEEE80211_IOC_BURST:
			rt2860_asic_updateslot(sc);
		break;

		case IEEE80211_IOC_SHORTGI:
		case IEEE80211_IOC_AMPDU_DENSITY:
		case IEEE80211_IOC_SMPS:
		break;

		default:
			error = ENETRESET;
		break;
	}

	return error;
}

/*
 * rt2860_vap_newstate
 */
static int rt2860_vap_newstate(struct ieee80211vap *vap,
	enum ieee80211_state nstate, int arg)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_vap *rvap;
	struct ieee80211_node *ni;
	enum ieee80211_state ostate;
	int error;

	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rvap = (struct rt2860_softc_vap *) vap;

	ostate = vap->iv_state;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATE,
		"%s: VAP newstate: %s -> %s\n",
		device_get_nameunit(sc->dev),
		ieee80211_state_name[ostate], ieee80211_state_name[nstate]);

	error = rvap->newstate(vap, nstate, arg);
	if (error != 0)
		return error;

	IEEE80211_UNLOCK(ic);

	RT2860_SOFTC_LOCK(sc);

	/* turn link LED off */

	if (nstate != IEEE80211_S_RUN)
		rt2860_led_cmd(sc, RT2860_LED_CMD_RADIO_OFF);

	switch (nstate)
	{
		case IEEE80211_S_INIT:
			rt2860_asic_disable_tsf_sync(sc);
		break;

		case IEEE80211_S_RUN:
			ni = vap->iv_bss;

			rt2860_rf_set_chan(sc, ni->ni_chan);

			if (vap->iv_opmode != IEEE80211_M_MONITOR)
			{
				rt2860_asic_enable_mrr(sc);
				rt2860_asic_set_txpreamble(sc);
				rt2860_asic_set_basicrates(sc);
				rt2860_asic_update_txpower(sc);
				rt2860_asic_set_bssid(sc, ni->ni_bssid);
			}

			if (vap->iv_opmode == IEEE80211_M_HOSTAP ||
		    	vap->iv_opmode == IEEE80211_M_IBSS ||
		    	vap->iv_opmode == IEEE80211_M_MBSS)
			{
				error = rt2860_beacon_alloc(sc, vap);
				if (error != 0)
					break;

				rt2860_asic_update_beacon(sc, vap);
			}

			if (vap->iv_opmode != IEEE80211_M_MONITOR)
				rt2860_asic_enable_tsf_sync(sc);

			/* turn link LED on */

			if (vap->iv_opmode != IEEE80211_M_MONITOR)
			{
				rt2860_led_cmd(sc, RT2860_LED_CMD_RADIO_ON |
		    		(IEEE80211_IS_CHAN_2GHZ(ni->ni_chan) ?
		     			RT2860_LED_CMD_LINK_2GHZ : RT2860_LED_CMD_LINK_5GHZ));
			}
		break;

		case IEEE80211_S_SLEEP:
		break;

		default:
		break;
	}

	RT2860_SOFTC_UNLOCK(sc);

	IEEE80211_LOCK(ic);

	return error;
}

/*
 * rt2860_vap_key_update_begin
 */
static void rt2860_vap_key_update_begin(struct ieee80211vap *vap)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;

	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	RT2860_DPRINTF(sc, RT2860_DEBUG_KEY,
		"%s: VAP key update begin\n",
		device_get_nameunit(sc->dev));

	taskqueue_block(sc->taskqueue);

	IF_LOCK(&ifp->if_snd);
}

/*
 * rt2860_vap_key_update_end
 */
static void rt2860_vap_key_update_end(struct ieee80211vap *vap)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;

	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	RT2860_DPRINTF(sc, RT2860_DEBUG_KEY,
		"%s: VAP key update end\n",
		device_get_nameunit(sc->dev));

	IF_UNLOCK(&ifp->if_snd);

	taskqueue_unblock(sc->taskqueue);
}

/*
 * rt2860_vap_key_set
 */
static int rt2860_vap_key_set(struct ieee80211vap *vap,
	const struct ieee80211_key *k, const uint8_t mac[IEEE80211_ADDR_LEN])
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct ieee80211_node *ni;
	struct rt2860_softc_node *rni;
	uint16_t key_base, keymode_base;
	uint8_t mode, vapid, wcid, iv[8];
	uint32_t tmp;

	if (k->wk_cipher->ic_cipher != IEEE80211_CIPHER_WEP &&
		k->wk_cipher->ic_cipher != IEEE80211_CIPHER_TKIP &&
		k->wk_cipher->ic_cipher != IEEE80211_CIPHER_AES_CCM)
		return 0;

	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	switch (k->wk_cipher->ic_cipher)
	{
		case IEEE80211_CIPHER_WEP:
			if(k->wk_keylen < 8)
				mode = RT2860_REG_CIPHER_MODE_WEP40;
			else
				mode = RT2860_REG_CIPHER_MODE_WEP104;
		break;

		case IEEE80211_CIPHER_TKIP:
			mode = RT2860_REG_CIPHER_MODE_TKIP;
		break;

		case IEEE80211_CIPHER_AES_CCM:
			mode = RT2860_REG_CIPHER_MODE_AES_CCMP;
		break;

		default:
			return 0;
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_KEY,
		"%s: VAP key set: keyix=%d, keylen=%d, macaddr=%s, mode=%d, group=%d\n",
		device_get_nameunit(sc->dev), k->wk_keyix, k->wk_keylen, ether_sprintf(k->wk_macaddr),
		mode, (k->wk_flags & IEEE80211_KEY_GROUP) ? 1 : 0);

	if (!(k->wk_flags & IEEE80211_KEY_GROUP))
	{
		/* install pairwise key */

		ni = ieee80211_find_vap_node(&ic->ic_sta, vap, mac);
		if (!ni) {
			printf("ieee80211_find_vap_node return 0\n");
			return 0;
		}
		rni = (struct rt2860_softc_node *) ni;

		vapid = 0;
		wcid = rni->staid;
		key_base = RT2860_REG_PKEY(wcid);

		ieee80211_free_node(ni);

		if (k->wk_cipher->ic_cipher == IEEE80211_CIPHER_WEP)
		{
			memset(iv, 0, 8);

			iv[3] = (k->wk_keyix << 6);
		}
		else
		{
			if (k->wk_cipher->ic_cipher == IEEE80211_CIPHER_TKIP)
			{
				iv[0] = (k->wk_keytsc >> 8);
				iv[1] = ((iv[0] | 0x20) & 0x7f);
				iv[2] = k->wk_keytsc;
			}
			else
			{
				/* AES CCMP */

				iv[0] = k->wk_keytsc;
				iv[1] = k->wk_keytsc >> 8;
				iv[2] = 0;
			}

			iv[3] = ((k->wk_keyix << 6) | IEEE80211_WEP_EXTIV);
			iv[4] = (k->wk_keytsc >> 16);
			iv[5] = (k->wk_keytsc >> 24);
			iv[6] = (k->wk_keytsc >> 32);
			iv[7] = (k->wk_keytsc >> 40);

			RT2860_DPRINTF(sc, RT2860_DEBUG_KEY,
				"%s: VAP key set: iv=%02x %02x %02x %02x %02x %02x %02x %02x\n",
				device_get_nameunit(sc->dev),
				iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7]);
		}

		rt2860_io_mac_write_multi(sc, RT2860_REG_IVEIV(wcid), iv, 8);

		if (k->wk_cipher->ic_cipher == IEEE80211_CIPHER_TKIP)
		{
			rt2860_io_mac_write_multi(sc, key_base, k->wk_key, 16);

			if (vap->iv_opmode != IEEE80211_M_HOSTAP)
			{
				rt2860_io_mac_write_multi(sc, key_base + 16, &k->wk_key[16], 8);
				rt2860_io_mac_write_multi(sc, key_base + 24, &k->wk_key[24], 8);
			}
			else
			{
				rt2860_io_mac_write_multi(sc, key_base + 16, &k->wk_key[24], 8);
				rt2860_io_mac_write_multi(sc, key_base + 24, &k->wk_key[16], 8);
			}
		}
		else
		{
			rt2860_io_mac_write_multi(sc, key_base, k->wk_key, k->wk_keylen);
		}

		tmp = ((vapid & RT2860_REG_VAP_MASK) << RT2860_REG_VAP_SHIFT) |
			(mode << RT2860_REG_CIPHER_MODE_SHIFT) | RT2860_REG_PKEY_ENABLE;

		rt2860_io_mac_write(sc, RT2860_REG_WCID_ATTR(wcid), tmp);
	}

	if ((k->wk_flags & IEEE80211_KEY_GROUP) ||
		(k->wk_cipher->ic_cipher == IEEE80211_CIPHER_WEP))
	{
		/* install group key */

		vapid = 0;
		wcid = RT2860_WCID_MCAST;
		key_base = RT2860_REG_SKEY(vapid, k->wk_keyix);
		keymode_base = RT2860_REG_SKEY_MODE(vapid);

		if (k->wk_cipher->ic_cipher == IEEE80211_CIPHER_TKIP)
		{
			rt2860_io_mac_write_multi(sc, key_base, k->wk_key, 16);

			if (vap->iv_opmode != IEEE80211_M_HOSTAP)
			{
				rt2860_io_mac_write_multi(sc, key_base + 16, &k->wk_key[16], 8);
				rt2860_io_mac_write_multi(sc, key_base + 24, &k->wk_key[24], 8);
			}
			else
			{
				rt2860_io_mac_write_multi(sc, key_base + 16, &k->wk_key[24], 8);
				rt2860_io_mac_write_multi(sc, key_base + 24, &k->wk_key[16], 8);
			}
		}
		else
		{
			rt2860_io_mac_write_multi(sc, key_base, k->wk_key, k->wk_keylen);
		}

		tmp = rt2860_io_mac_read(sc, keymode_base);

		tmp &= ~(0xf << (k->wk_keyix * 4 + 16 * (vapid % 2)));
		tmp |= (mode << (k->wk_keyix * 4 + 16 * (vapid % 2)));

		rt2860_io_mac_write(sc, keymode_base, tmp);

		if (vap->iv_opmode == IEEE80211_M_HOSTAP)
		{
			if (k->wk_cipher->ic_cipher == IEEE80211_CIPHER_WEP)
			{
				memset(iv, 0, 8);

				iv[3] = (k->wk_keyix << 6);
			}
			else
			{
				if (k->wk_cipher->ic_cipher == IEEE80211_CIPHER_TKIP)
				{
					iv[0] = (k->wk_keytsc >> 8);
					iv[1] = ((iv[0] | 0x20) & 0x7f);
					iv[2] = k->wk_keytsc;
				}
				else
				{
					/* AES CCMP */

					iv[0] = k->wk_keytsc;
					iv[1] = k->wk_keytsc >> 8;
					iv[2] = 0;
				}

				iv[3] = ((k->wk_keyix << 6) | IEEE80211_WEP_EXTIV);
				iv[4] = (k->wk_keytsc >> 16);
				iv[5] = (k->wk_keytsc >> 24);
				iv[6] = (k->wk_keytsc >> 32);
				iv[7] = (k->wk_keytsc >> 40);

				RT2860_DPRINTF(sc, RT2860_DEBUG_KEY,
					"%s: VAP key set: iv=%02x %02x %02x %02x %02x %02x %02x %02x\n",
					device_get_nameunit(sc->dev),
					iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7]);
			}

			rt2860_io_mac_write_multi(sc, RT2860_REG_IVEIV(wcid), iv, 8);

			tmp = ((vapid & RT2860_REG_VAP_MASK) << RT2860_REG_VAP_SHIFT) |
				(mode << RT2860_REG_CIPHER_MODE_SHIFT);

			rt2860_io_mac_write(sc, RT2860_REG_WCID_ATTR(wcid), tmp);
		}
	}

	return 1;
}

/*
 * rt2860_vap_key_delete
 */
static int rt2860_vap_key_delete(struct ieee80211vap *vap,
	const struct ieee80211_key *k)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	uint8_t vapid, wcid;
	uint32_t tmp;

	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	RT2860_DPRINTF(sc, RT2860_DEBUG_KEY,
		"%s: VAP key delete: keyix=%d, keylen=%d, macaddr=%s, group=%d\n",
		device_get_nameunit(sc->dev), k->wk_keyix, k->wk_keylen, ether_sprintf(k->wk_macaddr),
		(k->wk_flags & IEEE80211_KEY_GROUP) ? 1 : 0);

	if (k->wk_flags & IEEE80211_KEY_GROUP)
	{
		/* remove group key */

		vapid = 0;
		wcid = RT2860_WCID_MCAST;

		tmp = rt2860_io_mac_read(sc, RT2860_REG_SKEY_MODE(vapid));

		tmp &= ~(0xf << (k->wk_keyix * 4 + 16 * (vapid % 2)));
		tmp |= (RT2860_REG_CIPHER_MODE_NONE << (k->wk_keyix * 4 + 16 * (vapid % 2)));

		rt2860_io_mac_write(sc, RT2860_REG_SKEY_MODE(vapid), tmp);

		if (vap->iv_opmode == IEEE80211_M_HOSTAP)
		{
			tmp = ((vapid & RT2860_REG_VAP_MASK) << RT2860_REG_VAP_SHIFT) |
				(RT2860_REG_CIPHER_MODE_NONE << RT2860_REG_CIPHER_MODE_SHIFT) | RT2860_REG_PKEY_ENABLE;

			rt2860_io_mac_write(sc, RT2860_REG_WCID_ATTR(wcid), tmp);
		}
	}

	return 1;
}

/*
 * rt2860_vap_update_beacon
 */
static void rt2860_vap_update_beacon(struct ieee80211vap *vap, int what)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_vap *rvap;
	struct mbuf *m;
	struct ieee80211_beacon_offsets *bo;
	int error;

	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rvap = (struct rt2860_softc_vap *) vap;
	m = rvap->beacon_mbuf;
	bo = &rvap->beacon_offsets;

	RT2860_DPRINTF(sc, RT2860_DEBUG_BEACON,
		"%s: VAP update beacon: what=%d\n",
		device_get_nameunit(sc->dev), what);

	setbit(bo->bo_flags, what);

	if (m == NULL)
	{
		error = rt2860_beacon_alloc(sc, vap);
		if (error != 0)
			return;

		m = rvap->beacon_mbuf;
	}

	ieee80211_beacon_update(vap->iv_bss, bo, m, 0);

	rt2860_asic_update_beacon(sc, vap);
}

/*
 * rt2860_media_change
 */
static int rt2860_media_change(struct ifnet *ifp)
{
	int error;

	error = ieee80211_media_change(ifp);

	return (error == ENETRESET ? 0 : error);
}

/*
 * rt2860_node_alloc
 */
static struct ieee80211_node *rt2860_node_alloc(struct ieee80211vap *vap,
	const uint8_t mac[IEEE80211_ADDR_LEN])
{
	return malloc(sizeof(struct rt2860_softc_node),
		M_80211_NODE, M_NOWAIT | M_ZERO);
}

/*
 * rt2860_node_cleanup
 */
static void rt2860_node_cleanup(struct ieee80211_node *ni)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_node *rni;
	uint8_t vapid, wcid;
	uint32_t tmp;

	ic = ni->ni_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rni = (struct rt2860_softc_node *) ni;

	RT2860_DPRINTF(sc, RT2860_DEBUG_NODE,
		"%s: node cleanup: macaddr=%s, associd=0x%04x, staid=0x%02x\n",
		device_get_nameunit(sc->dev), ether_sprintf(ni->ni_macaddr),
		ni->ni_associd, rni->staid);

	if (rni->staid != 0)
	{
		vapid = 0;
		wcid = rni->staid;

		tmp = ((vapid & RT2860_REG_VAP_MASK) << RT2860_REG_VAP_SHIFT) |
			(RT2860_REG_CIPHER_MODE_NONE << RT2860_REG_CIPHER_MODE_SHIFT) | RT2860_REG_PKEY_ENABLE;

		rt2860_io_mac_write(sc, RT2860_REG_WCID_ATTR(wcid), tmp);

		rt2860_io_mac_write(sc, RT2860_REG_WCID(wcid), 0x00000000);
		rt2860_io_mac_write(sc, RT2860_REG_WCID(wcid) + 4, 0x00000000);

		rt2860_staid_delete(sc, rni->staid);

		rni->staid = 0;
	}

	sc->node_cleanup(ni);
}

/*
 * rt2860_node_getmimoinfo
 */
static void rt2860_node_getmimoinfo(const struct ieee80211_node *ni,
	struct ieee80211_mimo_info *mi)
{
	const struct rt2860_softc_node *rni;
	int i;

	rni = (const struct rt2860_softc_node *) ni;

	for (i = 0; i < RT2860_SOFTC_RSSI_COUNT; i++)
	{
		mi->rssi[i] = rni->last_rssi_dbm[i];
		mi->noise[i] = RT2860_NOISE_FLOOR;
	}
}

/*
 * rt2860_setregdomain
 */
static int rt2860_setregdomain(struct ieee80211com *ic,
	struct ieee80211_regdomain *reg,
	int nchans, struct ieee80211_channel chans[])
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;

	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATE,
		"%s: set regulatory domain: country=%d, country code string=%c%c, location=%c\n",
		device_get_nameunit(sc->dev),
		reg->country, reg->isocc[0], reg->isocc[1], reg->location);

	return 0;
}

/*
 * rt2860_getradiocaps
 */
static void rt2860_getradiocaps(struct ieee80211com *ic,
	int maxchans, int *nchans, struct ieee80211_channel chans[])
{
	*nchans = (ic->ic_nchans >= maxchans) ? maxchans : ic->ic_nchans;

	memcpy(chans, ic->ic_channels, (*nchans) * sizeof(struct ieee80211_channel));
}

/*
 * rt2860_scan_start
 */
static void rt2860_scan_start(struct ieee80211com *ic)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;

	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	rt2860_asic_disable_tsf_sync(sc);
}

/*
 * rt2860_scan_end
 */
static void rt2860_scan_end(struct ieee80211com *ic)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;

	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	rt2860_asic_enable_tsf_sync(sc);
}

/*
 * rt2860_set_channel
 */
static void rt2860_set_channel(struct ieee80211com *ic)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;

	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	RT2860_DPRINTF(sc, RT2860_DEBUG_CHAN,
		"%s: set channel: channel=%u, HT%s%s\n",
		device_get_nameunit(sc->dev),
		ieee80211_chan2ieee(ic, ic->ic_curchan),
		!IEEE80211_IS_CHAN_HT(ic->ic_curchan) ? " disabled" :
			IEEE80211_IS_CHAN_HT20(ic->ic_curchan) ? "20":
				IEEE80211_IS_CHAN_HT40U(ic->ic_curchan) ? "40U" : "40D",
		(ic->ic_flags & IEEE80211_F_SCAN) ? ", scanning" : "");

	RT2860_SOFTC_LOCK(sc);

	rt2860_rf_set_chan(sc, ic->ic_curchan);

	RT2860_SOFTC_UNLOCK(sc);
}

/*
 * rt2860_newassoc
 */
static void rt2860_newassoc(struct ieee80211_node *ni, int isnew)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct ieee80211vap *vap;
	struct rt2860_softc_vap *rvap;
	struct rt2860_softc_node *rni;
	uint16_t aid;
	uint8_t wcid;
	uint32_t tmp;

	vap = ni->ni_vap;
	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rvap = (struct rt2860_softc_vap *) vap;
	rni = (struct rt2860_softc_node *) ni;

	if (isnew)
	{
		aid = IEEE80211_AID(ni->ni_associd);
		rni->staid = rt2860_staid_alloc(sc, aid);
		wcid = rni->staid;

		tmp = (ni->ni_macaddr[3] << 24) |
			(ni->ni_macaddr[2] << 16) |
			(ni->ni_macaddr[1] << 8) |
			ni->ni_macaddr[0];

		rt2860_io_mac_write(sc, RT2860_REG_WCID(wcid), tmp);

		tmp = (ni->ni_macaddr[5] << 8) |
			ni->ni_macaddr[4];

		rt2860_io_mac_write(sc, RT2860_REG_WCID(wcid) + 4, tmp);

		rt2860_amrr_node_init(&rvap->amrr, &sc->amrr_node[wcid], ni);

		RT2860_DPRINTF(sc, RT2860_DEBUG_RATE,
			"%s: initial%s node Tx rate: associd=0x%04x, rate=0x%02x, max rate=0x%02x\n",
			device_get_nameunit(sc->dev),
			(ni->ni_flags & IEEE80211_NODE_HT) ? " HT" : "",
			ni->ni_associd, ni->ni_txrate,
			(ni->ni_flags & IEEE80211_NODE_HT) ?
				(ni->ni_htrates.rs_rates[ni->ni_htrates.rs_nrates - 1] | IEEE80211_RATE_MCS) :
				(ni->ni_rates.rs_rates[ni->ni_rates.rs_nrates - 1] & IEEE80211_RATE_VAL));

		rt2860_asic_updateprot(sc);
		rt2860_asic_updateslot(sc);
		rt2860_asic_set_txpreamble(sc);
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_NODE,
		"%s: new association: isnew=%d, macaddr=%s, associd=0x%04x, staid=0x%02x, QoS %s, ERP %s, HT %s\n",
		device_get_nameunit(sc->dev), isnew, ether_sprintf(ni->ni_macaddr),
		ni->ni_associd, rni->staid,
		(ni->ni_flags & IEEE80211_NODE_QOS) ? "enabled" : "disabled",
		(ni->ni_flags & IEEE80211_NODE_ERP) ? "enabled" : "disabled",
		(ni->ni_flags & IEEE80211_NODE_HT) ? "enabled" : "disabled");
}

/*
 * rt2860_updateslot
 */
static void rt2860_updateslot(struct ifnet *ifp)
{
	struct rt2860_softc *sc;

	sc = ifp->if_softc;

	rt2860_asic_updateslot(sc);
}

/*
 * rt2860_update_promisc
 */
static void rt2860_update_promisc(struct ifnet *ifp)
{
	struct rt2860_softc *sc;

	sc = ifp->if_softc;

	rt2860_asic_update_promisc(sc);
}

/*
 * rt2860_update_mcast
 */
static void rt2860_update_mcast(struct ifnet *ifp)
{
	struct rt2860_softc *sc;

	sc = ifp->if_softc;
}

/*
 * rt2860_wme_update
 */
static int rt2860_wme_update(struct ieee80211com *ic)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;

	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	rt2860_asic_wme_update(sc);

	return 0;
}

/*
 * rt2860_raw_xmit
 */
static int rt2860_raw_xmit(struct ieee80211_node *ni, struct mbuf *m,
	const struct ieee80211_bpf_params *params)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;

	ic = ni->ni_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;

	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
	{
		m_freem(m);
		ieee80211_free_node(ni);

		return ENETDOWN;
	}

	RT2860_SOFTC_TX_RING_LOCK(&sc->tx_ring[sc->tx_ring_mgtqid]);

	if (sc->tx_ring[sc->tx_ring_mgtqid].data_queued >= RT2860_SOFTC_TX_RING_DATA_COUNT)
	{
		RT2860_SOFTC_TX_RING_UNLOCK(&sc->tx_ring[sc->tx_ring_mgtqid]);

		RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
			"%s: raw xmit: Tx ring with qid=%d is full\n",
			device_get_nameunit(sc->dev), sc->tx_ring_mgtqid);

		ifp->if_drv_flags |= IFF_DRV_OACTIVE;

		m_freem(m);
		ieee80211_free_node(ni);

		sc->tx_data_queue_full[sc->tx_ring_mgtqid]++;

		return ENOBUFS;
	}

	if (rt2860_tx_mgmt(sc, m, ni, sc->tx_ring_mgtqid) != 0)
	{
		RT2860_SOFTC_TX_RING_UNLOCK(&sc->tx_ring[sc->tx_ring_mgtqid]);

		ifp->if_oerrors++;

		ieee80211_free_node(ni);

		return EIO;
	}

	RT2860_SOFTC_TX_RING_UNLOCK(&sc->tx_ring[sc->tx_ring_mgtqid]);

	sc->tx_timer = RT2860_TX_WATCHDOG_TIMEOUT;

	return 0;
}

/*
 * rt2860_recv_action
 */
static int rt2860_recv_action(struct ieee80211_node *ni,
	const struct ieee80211_frame *wh,
	const uint8_t *frm, const uint8_t *efrm)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_node *rni;
	const struct ieee80211_action *ia;
	uint16_t baparamset;
	uint8_t wcid;
	int tid;

	ic = ni->ni_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rni = (struct rt2860_softc_node *) ni;

	ia = (const struct ieee80211_action *) frm;

	if (ia->ia_category == IEEE80211_ACTION_CAT_BA)
	{
		switch (ia->ia_action)
		{
			/* IEEE80211_ACTION_BA_DELBA */
			case IEEE80211_ACTION_BA_DELBA:
				baparamset = LE_READ_2(frm + 2);
				tid = RT2860_MS(baparamset, IEEE80211_BAPS_TID);
				wcid = rni->staid;

				RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
					"%s: received DELBA request: associd=0x%04x, staid=0x%02x, tid=%d\n",
					device_get_nameunit(sc->dev), ni->ni_associd, rni->staid, tid);

				if (rni->staid != 0)
					rt2860_asic_del_ba_session(sc, wcid, tid);
			break;
		}
	}

	return sc->recv_action(ni, wh, frm, efrm);
}

/*
 * rt2860_send_action
 */
static int rt2860_send_action(struct ieee80211_node *ni,
	int cat, int act, void *sa)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_node *rni;
	uint16_t *args, status, baparamset;
	uint8_t wcid;
	int tid, bufsize;

	ic = ni->ni_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rni = (struct rt2860_softc_node *) ni;

	wcid = rni->staid;
	args = sa;

	if (cat == IEEE80211_ACTION_CAT_BA)
	{
		switch (act)
		{
			/* IEEE80211_ACTION_BA_ADDBA_RESPONSE */
			case IEEE80211_ACTION_BA_ADDBA_RESPONSE:
				status = args[1];
				baparamset = args[2];
				tid = RT2860_MS(baparamset, IEEE80211_BAPS_TID);
				bufsize = RT2860_MS(baparamset, IEEE80211_BAPS_BUFSIZ);

				RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
					"%s: sending ADDBA response: associd=0x%04x, staid=0x%02x, status=%d, tid=%d, bufsize=%d\n",
					device_get_nameunit(sc->dev), ni->ni_associd, rni->staid, status, tid, bufsize);

				if (status == IEEE80211_STATUS_SUCCESS)
					rt2860_asic_add_ba_session(sc, wcid, tid);
			break;

			/* IEEE80211_ACTION_BA_DELBA */
			case IEEE80211_ACTION_BA_DELBA:
				baparamset = RT2860_SM(args[0], IEEE80211_DELBAPS_TID) | args[1];
				tid = RT2860_MS(baparamset, IEEE80211_DELBAPS_TID);

				RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
					"%s: sending DELBA request: associd=0x%04x, staid=0x%02x, tid=%d\n",
					device_get_nameunit(sc->dev), ni->ni_associd, rni->staid, tid);

				if (RT2860_MS(baparamset, IEEE80211_DELBAPS_INIT) != IEEE80211_DELBAPS_INIT)
					rt2860_asic_del_ba_session(sc, wcid, tid);
			break;
		}
	}

	return sc->send_action(ni, cat, act, sa);
}

/*
 * rt2860_addba_response
 */
static int rt2860_addba_response(struct ieee80211_node *ni,
	struct ieee80211_tx_ampdu *tap,
	int status, int baparamset, int batimeout)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_node *rni;
	ieee80211_seq seqno;
	int ret, tid, old_bufsize, new_bufsize;

	ic = ni->ni_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rni = (struct rt2860_softc_node *) ni;

	tid = RT2860_MS(baparamset, IEEE80211_BAPS_TID);
	old_bufsize = RT2860_MS(baparamset, IEEE80211_BAPS_BUFSIZ);
	new_bufsize = old_bufsize;

	if (status == IEEE80211_STATUS_SUCCESS)
	{
		if (sc->mac_rev >= 0x28830300)
		{
			if (sc->mac_rev >= 0x30700200)
				new_bufsize = 13;
			else
				new_bufsize = 31;
		}
		else if (sc->mac_rev >= 0x28720200)
		{
			new_bufsize = 13;
		}
		else
		{
			new_bufsize = 7;
		}

		if (old_bufsize > new_bufsize)
		{
			baparamset &= ~IEEE80211_BAPS_BUFSIZ;
			baparamset = RT2860_SM(new_bufsize, IEEE80211_BAPS_BUFSIZ);
		}

		if (!(tap->txa_flags & IEEE80211_AGGR_RUNNING))
		{
			sc->tx_ampdu_sessions++;

			if (sc->tx_ampdu_sessions == 1)
				rt2860_asic_updateprot(sc);
		}
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
		"%s: received ADDBA response: associd=0x%04x, staid=0x%02x, status=%d, tid=%d, "
		"old bufsize=%d, new bufsize=%d\n",
		device_get_nameunit(sc->dev), ni->ni_associd, rni->staid, status, tid,
		old_bufsize, new_bufsize);

	ret = sc->addba_response(ni, tap, status, baparamset, batimeout);

	if (status == IEEE80211_STATUS_SUCCESS)
	{
		seqno = ni->ni_txseqs[tid];

		rt2860_send_bar(ni, tap, seqno);
	}

	return ret;
}

/*
 * rt2860_addba_stop
 */
static void rt2860_addba_stop(struct ieee80211_node *ni,
	struct ieee80211_tx_ampdu *tap)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_node *rni;
	int tid;

	ic = ni->ni_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rni = (struct rt2860_softc_node *) ni;

	tid = WME_AC_TO_TID(tap->txa_ac);

	RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
		"%s: stopping A-MPDU Tx: associd=0x%04x, staid=0x%02x, tid=%d\n",
		device_get_nameunit(sc->dev), ni->ni_associd, rni->staid, tid);

	if (tap->txa_flags & IEEE80211_AGGR_RUNNING)
	{
		if (sc->tx_ampdu_sessions > 0)
		{
			sc->tx_ampdu_sessions--;

			if (sc->tx_ampdu_sessions == 0)
				rt2860_asic_updateprot(sc);
		}
		else
		{
			printf("%s: number of A-MPDU Tx sessions cannot be negative\n",
				device_get_nameunit(sc->dev));
		}
	}

	sc->addba_stop(ni, tap);
}

/*
 * rt2860_ampdu_rx_start
 */
static int rt2860_ampdu_rx_start(struct ieee80211_node *ni,
	struct ieee80211_rx_ampdu *rap,
	int baparamset, int batimeout, int baseqctl)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_node *rni;
	int tid;

	ic = ni->ni_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rni = (struct rt2860_softc_node *) ni;

	tid = RT2860_MS(baparamset, IEEE80211_BAPS_TID);

	RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
		"%s: starting A-MPDU Rx: associd=0x%04x, staid=0x%02x, tid=%d\n",
		device_get_nameunit(sc->dev), ni->ni_associd, rni->staid, tid);

	if (!(rap->rxa_flags & IEEE80211_AGGR_RUNNING))
		sc->rx_ampdu_sessions++;

	return sc->ampdu_rx_start(ni, rap, baparamset, batimeout, baseqctl);
}

/*
 * rt2860_ampdu_rx_stop
 */
static void rt2860_ampdu_rx_stop(struct ieee80211_node *ni,
	struct ieee80211_rx_ampdu *rap)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct rt2860_softc_node *rni;

	ic = ni->ni_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rni = (struct rt2860_softc_node *) ni;

	RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
		"%s: stopping A-MPDU Rx: associd=0x%04x, staid=0x%02x\n",
		device_get_nameunit(sc->dev), ni->ni_associd, rni->staid);

	if (rap->rxa_flags & IEEE80211_AGGR_RUNNING)
	{
		if (sc->rx_ampdu_sessions > 0)
			sc->rx_ampdu_sessions--;
		else
			printf("%s: number of A-MPDU Rx sessions cannot be negative\n",
				device_get_nameunit(sc->dev));
	}

	sc->ampdu_rx_stop(ni, rap);
}

/*
 * rt2860_send_bar
 */
static int rt2860_send_bar(struct ieee80211_node *ni,
	struct ieee80211_tx_ampdu *tap, ieee80211_seq seqno)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct ieee80211vap *vap;
	struct ieee80211_frame_bar *bar;
	struct rt2860_softc_node *rni;
	struct mbuf *m;
	uint16_t barctl, barseqctl;
	uint8_t *frm;
	int ret, tid;

	ic = ni->ni_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	vap = ni->ni_vap;
	rni = (struct rt2860_softc_node *) ni;

	if (!(tap->txa_flags & IEEE80211_AGGR_RUNNING))
		return EINVAL;

	m = ieee80211_getmgtframe(&frm, ic->ic_headroom, sizeof(struct ieee80211_frame_bar));
	if (m == NULL)
		return ENOMEM;

	bar = mtod(m, struct ieee80211_frame_bar *);

	bar->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_CTL | IEEE80211_FC0_SUBTYPE_BAR;
	bar->i_fc[1] = 0;

	IEEE80211_ADDR_COPY(bar->i_ra, ni->ni_macaddr);
	IEEE80211_ADDR_COPY(bar->i_ta, vap->iv_myaddr);

	tid = WME_AC_TO_TID(tap->txa_ac);

	barctl = (tap->txa_flags & IEEE80211_AGGR_IMMEDIATE ?  0 : IEEE80211_BAR_NOACK) |
		IEEE80211_BAR_COMP |
		RT2860_SM(tid, IEEE80211_BAR_TID);
	barseqctl = RT2860_SM(seqno, IEEE80211_BAR_SEQ_START);

	bar->i_ctl = htole16(barctl);
	bar->i_seq = htole16(barseqctl);

	m->m_pkthdr.len = m->m_len = sizeof(struct ieee80211_frame_bar);

	tap->txa_start = seqno;

	ieee80211_ref_node(ni);

	RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
		"%s: sending BAR: associd=0x%04x, staid=0x%02x, tid=%d, seqno=%d\n",
		device_get_nameunit(sc->dev), ni->ni_associd, rni->staid, tid, seqno);

	ret = ic->ic_raw_xmit(ni, m, NULL);
	if (ret != 0)
		ieee80211_free_node(ni);

	return ret;
}

/*
 * rt2860_amrr_update_iter_func
 */
static void rt2860_amrr_update_iter_func(void *arg, struct ieee80211_node *ni)
{
	struct rt2860_softc *sc;
	struct ieee80211com *ic;
	struct ifnet *ifp;
	struct ieee80211vap *vap;
	struct rt2860_softc_vap *rvap;
	struct rt2860_softc_node *rni;
	uint8_t wcid;

	vap = arg;
	ic = vap->iv_ic;
	ifp = ic->ic_ifp;
	sc = ifp->if_softc;
	rvap = (struct rt2860_softc_vap *) vap;
	rni = (struct rt2860_softc_node *) ni;

	/* only associated stations */

	if ((ni->ni_vap == vap) && (rni->staid != 0))
	{
		wcid = rni->staid;

		RT2860_DPRINTF(sc, RT2860_DEBUG_RATE,
			"%s: AMRR node: staid=0x%02x, txcnt=%d, success=%d, retrycnt=%d\n",
			device_get_nameunit(sc->dev),
			rni->staid, sc->amrr_node[wcid].txcnt, sc->amrr_node[wcid].success, sc->amrr_node[wcid].retrycnt);

		rt2860_amrr_choose(ni, &sc->amrr_node[wcid]);

		RT2860_DPRINTF(sc, RT2860_DEBUG_RATE,
			"%s:%s node Tx rate: associd=0x%04x, staid=0x%02x, rate=0x%02x, max rate=0x%02x\n",
			device_get_nameunit(sc->dev),
			(ni->ni_flags & IEEE80211_NODE_HT) ? " HT" : "",
			ni->ni_associd, rni->staid, ni->ni_txrate,
			(ni->ni_flags & IEEE80211_NODE_HT) ?
				(ni->ni_htrates.rs_rates[ni->ni_htrates.rs_nrates - 1] | IEEE80211_RATE_MCS) :
				(ni->ni_rates.rs_rates[ni->ni_rates.rs_nrates - 1] & IEEE80211_RATE_VAL));
	}
}

/*
 * rt2860_periodic
 */
static void rt2860_periodic(void *arg)
{
	struct rt2860_softc *sc;

	sc = arg;

	RT2860_DPRINTF(sc, RT2860_DEBUG_PERIODIC,
		"%s: periodic\n",
		device_get_nameunit(sc->dev));

	taskqueue_enqueue(sc->taskqueue, &sc->periodic_task);
}

/*
 * rt2860_tx_watchdog
 */
static void rt2860_tx_watchdog(void *arg)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;

	sc = arg;
	ifp = sc->ifp;

	if (sc->tx_timer == 0)
		return;

	if (--sc->tx_timer == 0)
	{
		printf("%s: Tx watchdog timeout: resetting\n",
			device_get_nameunit(sc->dev));

		rt2860_stop_locked(sc);
		rt2860_init_locked(sc);

		ifp->if_oerrors++;

		sc->tx_watchdog_timeouts++;
	}

	callout_reset(&sc->tx_watchdog_ch, hz, rt2860_tx_watchdog, sc);
}

/*
 * rt2860_staid_alloc
 */
static int rt2860_staid_alloc(struct rt2860_softc *sc, int aid)
{
	int staid;

	if ((aid > 0 && aid < RT2860_SOFTC_STAID_COUNT) && isclr(sc->staid_mask, aid))
	{
		staid = aid;
	}
	else
	{
		for (staid = 1; staid < RT2860_SOFTC_STAID_COUNT; staid++)
		{
			if (isclr(sc->staid_mask, staid))
				break;
		}
	}

	setbit(sc->staid_mask, staid);

	return staid;
}

/*
 * rt2860_staid_delete
 */
static void rt2860_staid_delete(struct rt2860_softc *sc, int staid)
{
	clrbit(sc->staid_mask, staid);
}

/*
 * rt2860_asic_set_bssid
 */
static void rt2860_asic_set_bssid(struct rt2860_softc *sc,
	const uint8_t *bssid)
{
	uint32_t tmp;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATE,
		"%s: set bssid: bssid=%s\n",
		device_get_nameunit(sc->dev),
		ether_sprintf(bssid));

	tmp = bssid[0] | (bssid[1]) << 8 | (bssid[2] << 16) | (bssid[3] << 24);

	rt2860_io_mac_write(sc, RT2860_REG_BSSID_DW0, tmp);

	tmp = bssid[4] | (bssid[5] << 8);

	rt2860_io_mac_write(sc, RT2860_REG_BSSID_DW1, tmp);
}

/*
 * rt2860_asic_set_macaddr
 */
static void rt2860_asic_set_macaddr(struct rt2860_softc *sc,
	const uint8_t *addr)
{
	uint32_t tmp;

	tmp = addr[0] | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24);

	rt2860_io_mac_write(sc, RT2860_REG_ADDR_DW0, tmp);

	tmp = addr[4] | (addr[5] << 8) | (0xff << 16);

	rt2860_io_mac_write(sc, RT2860_REG_ADDR_DW1, tmp);
}

/*
 * rt2860_asic_enable_tsf_sync
 */
static void rt2860_asic_enable_tsf_sync(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;
	uint32_t tmp;

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	vap = TAILQ_FIRST(&ic->ic_vaps);

	RT2860_DPRINTF(sc, RT2860_DEBUG_BEACON,
		"%s: enabling TSF\n",
		device_get_nameunit(sc->dev));

	tmp = rt2860_io_mac_read(sc, RT2860_REG_BCN_TIME_CFG);

	tmp &= ~0x1fffff;
	tmp |= vap->iv_bss->ni_intval * 16;
	tmp |= (RT2860_REG_TSF_TIMER_ENABLE | RT2860_REG_TBTT_TIMER_ENABLE);

	if (vap->iv_opmode == IEEE80211_M_STA)
	{
		tmp |= (RT2860_REG_TSF_SYNC_MODE_STA << RT2860_REG_TSF_SYNC_MODE_SHIFT);
	}
	else if (vap->iv_opmode == IEEE80211_M_IBSS)
	{
		tmp |= RT2860_REG_BCN_TX_ENABLE;
		tmp |= (RT2860_REG_TSF_SYNC_MODE_IBSS << RT2860_REG_TSF_SYNC_MODE_SHIFT);
	}
	else if (vap->iv_opmode == IEEE80211_M_HOSTAP)
	{
		tmp |= RT2860_REG_BCN_TX_ENABLE;
		tmp |= (RT2860_REG_TSF_SYNC_MODE_HOSTAP << RT2860_REG_TSF_SYNC_MODE_SHIFT);
	}

	rt2860_io_mac_write(sc, RT2860_REG_BCN_TIME_CFG, tmp);
}

/*
 * rt2860_asic_disable_tsf_sync
 */
static void rt2860_asic_disable_tsf_sync(struct rt2860_softc *sc)
{
	uint32_t tmp;

	RT2860_DPRINTF(sc, RT2860_DEBUG_BEACON,
		"%s: disabling TSF\n",
		device_get_nameunit(sc->dev));

	tmp = rt2860_io_mac_read(sc, RT2860_REG_BCN_TIME_CFG);

	tmp &= ~(RT2860_REG_BCN_TX_ENABLE |
		RT2860_REG_TSF_TIMER_ENABLE |
		RT2860_REG_TBTT_TIMER_ENABLE);

	tmp &= ~(RT2860_REG_TSF_SYNC_MODE_MASK << RT2860_REG_TSF_SYNC_MODE_SHIFT);
	tmp |= (RT2860_REG_TSF_SYNC_MODE_DISABLE << RT2860_REG_TSF_SYNC_MODE_SHIFT);

	rt2860_io_mac_write(sc, RT2860_REG_BCN_TIME_CFG, tmp);
}

/*
 * rt2860_asic_enable_mrr
 */
static void rt2860_asic_enable_mrr(struct rt2860_softc *sc)
{
#define CCK(mcs)	(mcs)
#define OFDM(mcs)	((1 << 3) | (mcs))
#define HT(mcs)		(mcs)

	rt2860_io_mac_write(sc, RT2860_REG_TX_LG_FBK_CFG0,
		(OFDM(6) << 28) |	/* 54 -> 48 */
		(OFDM(5) << 24) |	/* 48 -> 36 */
		(OFDM(4) << 20) |	/* 36 -> 24 */
		(OFDM(3) << 16) |	/* 24 -> 18 */
		(OFDM(2) << 12) |	/* 18 -> 12 */
		(OFDM(1) << 8)  |	/* 12 -> 9 */
		(OFDM(0) << 4)  |	/*  9 -> 6 */
		OFDM(0));			/*  6 -> 6 */

	rt2860_io_mac_write(sc, RT2860_REG_TX_LG_FBK_CFG1,
		(CCK(2) << 12) |	/* 11  -> 5.5 */
		(CCK(1) << 8)  |	/* 5.5 -> 2 */
		(CCK(0) << 4)  |	/*   2 -> 1 */
		CCK(0));			/*   1 -> 1 */

	rt2860_io_mac_write(sc, RT2860_REG_TX_HT_FBK_CFG0,
		(HT(6) << 28) |
		(HT(5) << 24) |
		(HT(4) << 20) |
		(HT(3) << 16) |
		(HT(2) << 12) |
		(HT(1) << 8)  |
		(HT(0) << 4)  |
		HT(0));

	rt2860_io_mac_write(sc, RT2860_REG_TX_HT_FBK_CFG1,
		(HT(14) << 28) |
		(HT(13) << 24) |
		(HT(12) << 20) |
		(HT(11) << 16) |
		(HT(10) << 12) |
		(HT(9) << 8)   |
		(HT(8) << 4)   |
		HT(7));

#undef HT
#undef OFDM
#undef CCK
}

/*
 * rt2860_asic_set_txpreamble
 */
static void rt2860_asic_set_txpreamble(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	uint32_t tmp;

	ifp = sc->ifp;
	ic = ifp->if_l2com;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATE,
		"%s: %s short Tx preamble\n",
		device_get_nameunit(sc->dev),
		(ic->ic_flags & IEEE80211_F_SHPREAMBLE) ? "enabling" : "disabling");

	tmp = rt2860_io_mac_read(sc, RT2860_REG_AUTO_RSP_CFG);

	tmp &= ~RT2860_REG_CCK_SHORT_ENABLE;

	if (ic->ic_flags & IEEE80211_F_SHPREAMBLE)
		tmp |= RT2860_REG_CCK_SHORT_ENABLE;

	rt2860_io_mac_write(sc, RT2860_REG_AUTO_RSP_CFG, tmp);
}

/*
 * rt2860_asic_set_basicrates
 */
static void rt2860_asic_set_basicrates(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;

	ifp = sc->ifp;
	ic = ifp->if_l2com;

	if (ic->ic_curmode == IEEE80211_MODE_11B)
		rt2860_io_mac_write(sc, RT2860_REG_LEGACY_BASIC_RATE, 0xf);
	else if (ic->ic_curmode == IEEE80211_MODE_11A)
		rt2860_io_mac_write(sc, RT2860_REG_LEGACY_BASIC_RATE, 0x150);
	else
		rt2860_io_mac_write(sc, RT2860_REG_LEGACY_BASIC_RATE, 0x15f);
}

/*
 * rt2860_asic_update_rtsthreshold
 */
static void rt2860_asic_update_rtsthreshold(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;
	uint32_t tmp;
	uint16_t threshold;

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	vap = TAILQ_FIRST(&ic->ic_vaps);

	if (vap == NULL)
		threshold = IEEE80211_RTS_MAX;
	else if (vap->iv_flags_ht & IEEE80211_FHT_AMSDU_TX)
		threshold = 0x1000;
	else
		threshold = vap->iv_rtsthreshold;

	RT2860_DPRINTF(sc, RT2860_DEBUG_PROT,
		"%s: updating RTS threshold: %d\n",
		device_get_nameunit(sc->dev), threshold);

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_RTS_CFG);

	tmp &= ~(RT2860_REG_TX_RTS_THRESHOLD_MASK << RT2860_REG_TX_RTS_THRESHOLD_SHIFT);

	tmp |= ((threshold & RT2860_REG_TX_RTS_THRESHOLD_MASK) <<
		RT2860_REG_TX_RTS_THRESHOLD_SHIFT);

	rt2860_io_mac_write(sc, RT2860_REG_TX_RTS_CFG, tmp);
}

/*
 * rt2860_asic_update_txpower
 */
static void rt2860_asic_update_txpower(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	uint32_t *txpow_rate;
	int8_t delta;
	uint8_t val;
	uint32_t tmp;
	int i;

	ifp = sc->ifp;
	ic = ifp->if_l2com;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATE,
		"%s: updating Tx power: %d\n",
		device_get_nameunit(sc->dev), ic->ic_txpowlimit);

	if (!IEEE80211_IS_CHAN_HT40(ic->ic_curchan))
	{
		txpow_rate = sc->txpow_rate_20mhz;
	}
	else
	{
		if (IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan))
			txpow_rate = sc->txpow_rate_40mhz_2ghz;
		else
			txpow_rate = sc->txpow_rate_40mhz_5ghz;
	}

	delta = 0;

	val = rt2860_io_bbp_read(sc, 1);
	val &= 0xfc;

	if (ic->ic_txpowlimit > 90)
	{
		/* do nothing */
	}
	else if (ic->ic_txpowlimit > 60)
	{
		delta -= 1;
	}
	else if (ic->ic_txpowlimit > 30)
	{
		delta -= 3;
	}
	else if (ic->ic_txpowlimit > 15)
	{
		val |= 0x1;
	}
	else if (ic->ic_txpowlimit > 9)
	{
		val |= 0x1;
		delta -= 3;
	}
	else
	{
		val |= 0x2;
	}

	rt2860_io_bbp_write(sc, 1, val);

	for (i = 0; i < RT2860_SOFTC_TXPOW_RATE_COUNT; i++)
	{
		if (txpow_rate[i] == 0xffffffff)
			continue;

		tmp = rt2860_read_eeprom_txpow_rate_add_delta(txpow_rate[i], delta);

		rt2860_io_mac_write(sc, RT2860_REG_TX_PWR_CFG(i), tmp);
	}
}

/*
 * rt2860_asic_update_promisc
 */
static void rt2860_asic_update_promisc(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	uint32_t tmp;

	ifp = sc->ifp;

	printf("%s: %s promiscuous mode\n",
		device_get_nameunit(sc->dev),
		(ifp->if_flags & IFF_PROMISC) ? "entering" : "leaving");

	tmp = rt2860_io_mac_read(sc, RT2860_REG_RX_FILTER_CFG);

	tmp &= ~RT2860_REG_RX_FILTER_DROP_UC_NOME;

	if (!(ifp->if_flags & IFF_PROMISC))
		tmp |= RT2860_REG_RX_FILTER_DROP_UC_NOME;

	rt2860_io_mac_write(sc, RT2860_REG_RX_FILTER_CFG, tmp);
}

/*
 * rt2860_asic_updateprot
 */
static void rt2860_asic_updateprot(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;
	uint32_t cck_prot, ofdm_prot, mm20_prot, mm40_prot, gf20_prot, gf40_prot;
	uint8_t htopmode;
	enum ieee80211_protmode htprotmode;

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	vap = TAILQ_FIRST(&ic->ic_vaps);

	/* CCK frame protection */

	cck_prot = RT2860_REG_RTSTH_ENABLE | RT2860_REG_PROT_NAV_SHORT |
		RT2860_REG_TXOP_ALLOW_ALL | RT2860_REG_PROT_CTRL_NONE;

	/* set up protection frame phy mode and rate (MCS code) */

	if (ic->ic_curmode == IEEE80211_MODE_11A)
		cck_prot |= (RT2860_REG_PROT_PHYMODE_OFDM << RT2860_REG_PROT_PHYMODE_SHIFT) |
			(0 << RT2860_REG_PROT_MCS_SHIFT);
	else
		cck_prot |= ((RT2860_REG_PROT_PHYMODE_CCK << RT2860_REG_PROT_PHYMODE_SHIFT) |
			(3 << RT2860_REG_PROT_MCS_SHIFT));

	rt2860_io_mac_write(sc, RT2860_REG_TX_CCK_PROT_CFG, cck_prot);

	/* OFDM frame protection */

	ofdm_prot = RT2860_REG_RTSTH_ENABLE | RT2860_REG_PROT_NAV_SHORT |
		RT2860_REG_TXOP_ALLOW_ALL;

	if (ic->ic_flags & IEEE80211_F_USEPROT)
	{
		RT2860_DPRINTF(sc, RT2860_DEBUG_PROT,
			"%s: updating protection mode: b/g protection mode=%s\n",
			device_get_nameunit(sc->dev),
			(ic->ic_protmode == IEEE80211_PROT_RTSCTS) ? "RTS/CTS" :
				((ic->ic_protmode == IEEE80211_PROT_CTSONLY) ? "CTS-to-self" : "none"));

		if (ic->ic_protmode == IEEE80211_PROT_RTSCTS)
			ofdm_prot |= RT2860_REG_PROT_CTRL_RTS_CTS;
		else if (ic->ic_protmode == IEEE80211_PROT_CTSONLY)
			ofdm_prot |= RT2860_REG_PROT_CTRL_CTS;
		else
			ofdm_prot |= RT2860_REG_PROT_CTRL_NONE;
	}
	else
	{
		RT2860_DPRINTF(sc, RT2860_DEBUG_PROT,
			"%s: updating protection mode: b/g protection mode=%s\n",
			device_get_nameunit(sc->dev), "none");

		ofdm_prot |= RT2860_REG_PROT_CTRL_NONE;
	}

	rt2860_io_mac_write(sc, RT2860_REG_TX_OFDM_PROT_CFG, ofdm_prot);

	/* HT frame protection */

	if ((vap != NULL) && (vap->iv_opmode == IEEE80211_M_STA) && (vap->iv_state == IEEE80211_S_RUN))
		htopmode = vap->iv_bss->ni_htopmode;
	else
		htopmode = ic->ic_curhtprotmode;

	htprotmode = ic->ic_htprotmode;

	/* force HT mixed mode and RTS/CTS protection if A-MPDU Tx aggregation is enabled */

	if (sc->tx_ampdu_sessions > 0)
	{
		RT2860_DPRINTF(sc, RT2860_DEBUG_PROT,
			"%s: updating protection mode: forcing HT mixed mode and RTS/CTS protection\n",
			device_get_nameunit(sc->dev));

		htopmode = IEEE80211_HTINFO_OPMODE_MIXED;
		htprotmode = IEEE80211_PROT_RTSCTS;
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_PROT,
		"%s: updating protection mode: HT operation mode=0x%02x, protection mode=%s\n",
		device_get_nameunit(sc->dev),
		htopmode & IEEE80211_HTINFO_OPMODE,
		(htprotmode == IEEE80211_PROT_RTSCTS) ? "RTS/CTS" :
			((htprotmode == IEEE80211_PROT_CTSONLY) ? "CTS-to-self" : "none"));

	switch (htopmode & IEEE80211_HTINFO_OPMODE)
	{
		/* IEEE80211_HTINFO_OPMODE_HT20PR */
		case IEEE80211_HTINFO_OPMODE_HT20PR:
			mm20_prot = RT2860_REG_PROT_NAV_SHORT | RT2860_REG_PROT_CTRL_NONE |
				RT2860_REG_TXOP_ALLOW_CCK | RT2860_REG_TXOP_ALLOW_OFDM |
				RT2860_REG_TXOP_ALLOW_MM20 | RT2860_REG_TXOP_ALLOW_GF20 |
				(RT2860_REG_PROT_PHYMODE_OFDM << RT2860_REG_PROT_PHYMODE_SHIFT) |
				(4 << RT2860_REG_PROT_MCS_SHIFT);

			gf20_prot = mm20_prot;

			mm40_prot = RT2860_REG_PROT_NAV_SHORT | RT2860_REG_TXOP_ALLOW_ALL |
				(RT2860_REG_PROT_PHYMODE_OFDM << RT2860_REG_PROT_PHYMODE_SHIFT) |
				(0x84 << RT2860_REG_PROT_MCS_SHIFT);

			if (htprotmode == IEEE80211_PROT_RTSCTS)
				mm40_prot |= RT2860_REG_PROT_CTRL_RTS_CTS;
			else if (htprotmode == IEEE80211_PROT_CTSONLY)
				mm40_prot |= RT2860_REG_PROT_CTRL_CTS;
			else
				mm40_prot |= RT2860_REG_PROT_CTRL_NONE;

			gf40_prot = mm40_prot;
		break;

		/* IEEE80211_HTINFO_OPMODE_MIXED */
		case IEEE80211_HTINFO_OPMODE_MIXED:
			mm20_prot = RT2860_REG_PROT_NAV_SHORT |
				RT2860_REG_TXOP_ALLOW_CCK | RT2860_REG_TXOP_ALLOW_OFDM |
				RT2860_REG_TXOP_ALLOW_MM20 | RT2860_REG_TXOP_ALLOW_GF20;

			if (ic->ic_flags & IEEE80211_F_USEPROT)
				mm20_prot |= (RT2860_REG_PROT_PHYMODE_CCK << RT2860_REG_PROT_PHYMODE_SHIFT) |
					(3 << RT2860_REG_PROT_MCS_SHIFT);
			else
				mm20_prot |= (RT2860_REG_PROT_PHYMODE_OFDM << RT2860_REG_PROT_PHYMODE_SHIFT) |
					(4 << RT2860_REG_PROT_MCS_SHIFT);

			if (htprotmode == IEEE80211_PROT_RTSCTS)
				mm20_prot |= RT2860_REG_PROT_CTRL_RTS_CTS;
			else if (htprotmode == IEEE80211_PROT_CTSONLY)
				mm20_prot |= RT2860_REG_PROT_CTRL_CTS;
			else
				mm20_prot |= RT2860_REG_PROT_CTRL_NONE;

			gf20_prot = mm20_prot;

			mm40_prot = RT2860_REG_PROT_NAV_SHORT | RT2860_REG_TXOP_ALLOW_ALL;

			if (ic->ic_flags & IEEE80211_F_USEPROT)
				mm40_prot |= (RT2860_REG_PROT_PHYMODE_CCK << RT2860_REG_PROT_PHYMODE_SHIFT) |
					(3 << RT2860_REG_PROT_MCS_SHIFT);
			else
				mm40_prot |= (RT2860_REG_PROT_PHYMODE_OFDM << RT2860_REG_PROT_PHYMODE_SHIFT) |
					(0x84 << RT2860_REG_PROT_MCS_SHIFT);

			if (htprotmode == IEEE80211_PROT_RTSCTS)
				mm40_prot |= RT2860_REG_PROT_CTRL_RTS_CTS;
			else if (htprotmode == IEEE80211_PROT_CTSONLY)
				mm40_prot |= RT2860_REG_PROT_CTRL_CTS;
			else
				mm40_prot |= RT2860_REG_PROT_CTRL_NONE;

			gf40_prot = mm40_prot;
		break;

		/*
		 * IEEE80211_HTINFO_OPMODE_PURE
		 * IEEE80211_HTINFO_OPMODE_PROTOPT
		 */
		case IEEE80211_HTINFO_OPMODE_PURE:
		case IEEE80211_HTINFO_OPMODE_PROTOPT:
		default:
			mm20_prot = RT2860_REG_PROT_NAV_SHORT | RT2860_REG_PROT_CTRL_NONE |
				RT2860_REG_TXOP_ALLOW_CCK | RT2860_REG_TXOP_ALLOW_OFDM |
				RT2860_REG_TXOP_ALLOW_MM20 | RT2860_REG_TXOP_ALLOW_GF20 |
				(RT2860_REG_PROT_PHYMODE_OFDM << RT2860_REG_PROT_PHYMODE_SHIFT) |
				(4 << RT2860_REG_PROT_MCS_SHIFT);

			gf20_prot = mm20_prot;

			mm40_prot = RT2860_REG_PROT_NAV_SHORT | RT2860_REG_PROT_CTRL_NONE |
				RT2860_REG_TXOP_ALLOW_ALL |
				(RT2860_REG_PROT_PHYMODE_OFDM << RT2860_REG_PROT_PHYMODE_SHIFT) |
				(0x84 << RT2860_REG_PROT_MCS_SHIFT);

			gf40_prot = mm40_prot;
		break;
	}

	rt2860_io_mac_write(sc, RT2860_REG_TX_MM20_PROT_CFG, mm20_prot);
	rt2860_io_mac_write(sc, RT2860_REG_TX_MM40_PROT_CFG, mm40_prot);
	rt2860_io_mac_write(sc, RT2860_REG_TX_GF20_PROT_CFG, gf20_prot);
	rt2860_io_mac_write(sc, RT2860_REG_TX_GF40_PROT_CFG, gf40_prot);
}

/*
 * rt2860_asic_updateslot
 */
static void rt2860_asic_updateslot(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;
	uint32_t tmp;

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	vap = TAILQ_FIRST(&ic->ic_vaps);

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATE,
		"%s: %s short slot time\n",
		device_get_nameunit(sc->dev),
		((ic->ic_flags & IEEE80211_F_SHSLOT) ||
		 ((vap != NULL) && (vap->iv_flags & IEEE80211_F_BURST))) ? "enabling" : "disabling");

	tmp = rt2860_io_mac_read(sc, RT2860_REG_BKOFF_SLOT_CFG);

	tmp &= ~0xff;

	if ((ic->ic_flags & IEEE80211_F_SHSLOT) ||
		((vap != NULL) && (vap->iv_flags & IEEE80211_F_BURST)))
		tmp |= IEEE80211_DUR_SHSLOT;
	else
		tmp |= IEEE80211_DUR_SLOT;

	rt2860_io_mac_write(sc, RT2860_REG_BKOFF_SLOT_CFG, tmp);
}

/*
 * rt2860_asic_wme_update
 */
static void rt2860_asic_wme_update(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211_wme_state *wme;
	const struct wmeParams *wmep;
	int i;

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	wme = &ic->ic_wme;
	wmep = wme->wme_chanParams.cap_wmeParams;

	RT2860_DPRINTF(sc, RT2860_DEBUG_WME,
		"%s: wme update: WME_AC_VO=%d/%d/%d/%d, WME_AC_VI=%d/%d/%d/%d, "
		"WME_AC_BK=%d/%d/%d/%d, WME_AC_BE=%d/%d/%d/%d\n",
		device_get_nameunit(sc->dev),
		wmep[WME_AC_VO].wmep_aifsn,
		wmep[WME_AC_VO].wmep_logcwmin, wmep[WME_AC_VO].wmep_logcwmax,
		wmep[WME_AC_VO].wmep_txopLimit,
		wmep[WME_AC_VI].wmep_aifsn,
		wmep[WME_AC_VI].wmep_logcwmin, wmep[WME_AC_VI].wmep_logcwmax,
		wmep[WME_AC_VI].wmep_txopLimit,
		wmep[WME_AC_BK].wmep_aifsn,
		wmep[WME_AC_BK].wmep_logcwmin, wmep[WME_AC_BK].wmep_logcwmax,
		wmep[WME_AC_BK].wmep_txopLimit,
		wmep[WME_AC_BE].wmep_aifsn,
		wmep[WME_AC_BE].wmep_logcwmin, wmep[WME_AC_BE].wmep_logcwmax,
		wmep[WME_AC_BE].wmep_txopLimit);

	for (i = 0; i < WME_NUM_AC; i++)
		rt2860_io_mac_write(sc, RT2860_REG_TX_EDCA_AC_CFG(i),
			(wmep[i].wmep_logcwmax << 16) | (wmep[i].wmep_logcwmin << 12) |
			(wmep[i].wmep_aifsn << 8) | wmep[i].wmep_txopLimit);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WMM_AIFSN_CFG,
		(wmep[WME_AC_VO].wmep_aifsn << 12) | (wmep[WME_AC_VI].wmep_aifsn << 8) |
		(wmep[WME_AC_BK].wmep_aifsn << 4) | wmep[WME_AC_BE].wmep_aifsn);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WMM_CWMIN_CFG,
		(wmep[WME_AC_VO].wmep_logcwmin << 12) | (wmep[WME_AC_VI].wmep_logcwmin << 8) |
		(wmep[WME_AC_BK].wmep_logcwmin << 4) | wmep[WME_AC_BE].wmep_logcwmin);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WMM_CWMAX_CFG,
		(wmep[WME_AC_VO].wmep_logcwmax << 12) | (wmep[WME_AC_VI].wmep_logcwmax << 8) |
		(wmep[WME_AC_BK].wmep_logcwmax << 4) | wmep[WME_AC_BE].wmep_logcwmax);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WMM_TXOP0_CFG,
		(wmep[WME_AC_BK].wmep_txopLimit << 16) | wmep[WME_AC_BE].wmep_txopLimit);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WMM_TXOP1_CFG,
		(wmep[WME_AC_VO].wmep_txopLimit << 16) | wmep[WME_AC_VI].wmep_txopLimit);
}

/*
 * rt2860_asic_update_beacon
 */
static void rt2860_asic_update_beacon(struct rt2860_softc *sc,
	struct ieee80211vap *vap)
{
	struct rt2860_softc_vap *rvap;
	struct mbuf *m;
	struct rt2860_txwi *txwi;
	uint32_t tmp;

	rvap = (struct rt2860_softc_vap *) vap;

	m = rvap->beacon_mbuf;
	txwi = &rvap->beacon_txwi;

	/* disable temporarily TSF sync */

	tmp = rt2860_io_mac_read(sc, RT2860_REG_BCN_TIME_CFG);

	tmp &= ~(RT2860_REG_BCN_TX_ENABLE |
		RT2860_REG_TSF_TIMER_ENABLE |
		RT2860_REG_TBTT_TIMER_ENABLE);

	rt2860_io_mac_write(sc, RT2860_REG_BCN_TIME_CFG, tmp);

	/* write Tx wireless info and beacon frame to on-chip memory */

	rt2860_io_mac_write_multi(sc, RT2860_REG_BEACON_BASE(0),
		txwi, sizeof(struct rt2860_txwi));

	rt2860_io_mac_write_multi(sc, RT2860_REG_BEACON_BASE(0) + sizeof(struct rt2860_txwi),
		mtod(m, uint8_t *), m->m_pkthdr.len);

	/* enable again TSF sync */

	tmp = rt2860_io_mac_read(sc, RT2860_REG_BCN_TIME_CFG);

	tmp |= (RT2860_REG_BCN_TX_ENABLE |
		RT2860_REG_TSF_TIMER_ENABLE |
		RT2860_REG_TBTT_TIMER_ENABLE);

	rt2860_io_mac_write(sc, RT2860_REG_BCN_TIME_CFG, tmp);
}

/*
 * rt2860_asic_clear_keytables
 */
static void rt2860_asic_clear_keytables(struct rt2860_softc *sc)
{
	int i;

	/* clear Rx WCID search table (entries = 256, entry size = 8) */

	for (i = 0; i < 256; i++)
	{
		rt2860_io_mac_write(sc, RT2860_REG_WCID(i), 0xffffffff);
		rt2860_io_mac_write(sc, RT2860_REG_WCID(i) + 4, 0x0000ffff);
	}

	/* clear WCID attribute table (entries = 256, entry size = 4) */

	rt2860_io_mac_set_region_4(sc, RT2860_REG_WCID_ATTR(0), 0, 256);

	/* clear IV/EIV table (entries = 256, entry size = 8) */

	rt2860_io_mac_set_region_4(sc, RT2860_REG_IVEIV(0), 0, 2 * 256);

	/* clear pairwise key table (entries = 64, entry size = 32) */

	rt2860_io_mac_set_region_4(sc, RT2860_REG_PKEY(0), 0, 8 * 64);

	/* clear shared key table (entries = 32, entry size = 32) */

	rt2860_io_mac_set_region_4(sc, RT2860_REG_SKEY(0, 0), 0, 8 * 32);

	/* clear shared key mode (entries = 32, entry size = 2) */

	rt2860_io_mac_set_region_4(sc, RT2860_REG_SKEY_MODE(0), 0, 16);
}

/*
 * rt2860_asic_add_ba_session
 */
static void rt2860_asic_add_ba_session(struct rt2860_softc *sc,
	uint8_t wcid, int tid)
{
	uint32_t tmp;

	RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
		"%s: adding BA session: wcid=0x%02x, tid=%d\n",
		device_get_nameunit(sc->dev), wcid, tid);

	tmp = rt2860_io_mac_read(sc, RT2860_REG_WCID(wcid) + 4);

	tmp |= (0x10000 << tid);

	rt2860_io_mac_write(sc, RT2860_REG_WCID(wcid) + 4, tmp);
}

/*
 * rt2860_asic_del_ba_session
 */
static void rt2860_asic_del_ba_session(struct rt2860_softc *sc,
	uint8_t wcid, int tid)
{
	uint32_t tmp;

	RT2860_DPRINTF(sc, RT2860_DEBUG_BA,
		"%s: deleting BA session: wcid=0x%02x, tid=%d\n",
		device_get_nameunit(sc->dev), wcid, tid);

	tmp = rt2860_io_mac_read(sc, RT2860_REG_WCID(wcid) + 4);

	tmp &= ~(0x10000 << tid);

	rt2860_io_mac_write(sc, RT2860_REG_WCID(wcid) + 4, tmp);
}

/*
 * rt2860_beacon_alloc
 */
static int rt2860_beacon_alloc(struct rt2860_softc *sc,
	struct ieee80211vap *vap)
{
	struct ieee80211com *ic;
	struct rt2860_softc_vap *rvap;
	struct mbuf *m;
	struct rt2860_txwi txwi;
	uint8_t rate, mcs;

	ic = vap->iv_ic;
	rvap = (struct rt2860_softc_vap *) vap;

	m = ieee80211_beacon_alloc(vap->iv_bss, &rvap->beacon_offsets);
	if (m == NULL)
		return ENOMEM;

	rate = IEEE80211_IS_CHAN_5GHZ(vap->iv_bss->ni_chan) ? 12 : 2;
	mcs = rt2860_rate2mcs(rate);

	RT2860_DPRINTF(sc, RT2860_DEBUG_BEACON,
		"%s: beacon allocate: mcs=0x%02x\n",
		device_get_nameunit(sc->dev), mcs);

	memset(&txwi, 0, sizeof(struct rt2860_txwi));

	txwi.wcid = RT2860_WCID_RESERVED;
	txwi.pid_mpdu_len = ((htole16(m->m_pkthdr.len) & RT2860_TXWI_MPDU_LEN_MASK) <<
		 RT2860_TXWI_MPDU_LEN_SHIFT);
	txwi.txop = (RT2860_TXWI_TXOP_HT << RT2860_TXWI_TXOP_SHIFT);
	txwi.mpdu_density_flags |=
		(RT2860_TXWI_FLAGS_TS << RT2860_TXWI_FLAGS_SHIFT);
	txwi.bawin_size_xflags |=
		(RT2860_TXWI_XFLAGS_NSEQ << RT2860_TXWI_XFLAGS_SHIFT);

	if (rate == 2)
	{
		txwi.phymode_ifs_stbc_shortgi =
			(RT2860_TXWI_PHYMODE_CCK << RT2860_TXWI_PHYMODE_SHIFT);

		if (rate != 2 && (ic->ic_flags & IEEE80211_F_SHPREAMBLE))
			mcs |= RT2860_TXWI_MCS_SHOTPRE;
	}
	else
	{
		txwi.phymode_ifs_stbc_shortgi =
			(RT2860_TXWI_PHYMODE_OFDM << RT2860_TXWI_PHYMODE_SHIFT);
	}

	txwi.bw_mcs = (RT2860_TXWI_BW_20 << RT2860_TXWI_BW_SHIFT) |
		((mcs & RT2860_TXWI_MCS_MASK) << RT2860_TXWI_MCS_SHIFT);

	if (rvap->beacon_mbuf != NULL)
	{
		m_free(rvap->beacon_mbuf);
		rvap->beacon_mbuf = NULL;
	}

	rvap->beacon_mbuf = m;
	rvap->beacon_txwi = txwi;

	return 0;
}

/*
 * rt2860_rxrate
 */
static uint8_t rt2860_rxrate(struct rt2860_rxwi *rxwi)
{
	uint8_t mcs, phymode;
	uint8_t rate;

	mcs = (rxwi->bw_mcs >> RT2860_RXWI_MCS_SHIFT) & RT2860_RXWI_MCS_MASK;
	phymode = (rxwi->phymode_stbc_shortgi >> RT2860_RXWI_PHYMODE_SHIFT) &
		RT2860_RXWI_PHYMODE_MASK;

	rate = 2;

	switch (phymode)
	{
		case RT2860_RXWI_PHYMODE_CCK:
			switch (mcs & ~RT2860_RXWI_MCS_SHOTPRE)
			{
				case 0: rate = 2; break;	/* 1 Mbps */
				case 1: rate = 4; break;	/* 2 MBps */
				case 2: rate = 11; break;	/* 5.5 Mbps */
				case 3: rate = 22; break;	/* 11 Mbps */
			}
		break;

		case RT2860_RXWI_PHYMODE_OFDM:
			switch (mcs)
			{
				case 0: rate = 12; break;	/* 6 Mbps */
				case 1: rate = 18; break;	/* 9 Mbps */
				case 2: rate = 24; break;	/* 12 Mbps */
				case 3: rate = 36; break;	/* 18 Mbps */
				case 4: rate = 48; break;	/* 24 Mbps */
				case 5: rate = 72; break;	/* 36 Mbps */
				case 6: rate = 96; break;	/* 48 Mbps */
				case 7: rate = 108; break;	/* 54 Mbps */
			}
		break;

		case RT2860_RXWI_PHYMODE_HT_MIXED:
		case RT2860_RXWI_PHYMODE_HT_GF:
		break;
	}

	return rate;
}

/*
 * rt2860_maxrssi_rxpath
 */
static uint8_t rt2860_maxrssi_rxpath(struct rt2860_softc *sc,
	const struct rt2860_rxwi *rxwi)
{
	uint8_t rxpath;

	rxpath = 0;

	if (sc->nrxpath > 1)
		if (rxwi->rssi[1] > rxwi->rssi[rxpath])
			rxpath = 1;

	if (sc->nrxpath > 2)
		if (rxwi->rssi[2] > rxwi->rssi[rxpath])
			rxpath = 2;

	return rxpath;
}

/*
 * rt2860_rssi2dbm
 */
static int8_t rt2860_rssi2dbm(struct rt2860_softc *sc,
	uint8_t rssi, uint8_t rxpath)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211_channel *c;
	int chan;
	int8_t rssi_off, lna_gain;

	if (rssi == 0)
		return -99;

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	c = ic->ic_curchan;
	chan = ieee80211_chan2ieee(ic, c);

	if (IEEE80211_IS_CHAN_5GHZ(c))
	{
		rssi_off = sc->rssi_off_5ghz[rxpath];

		if (chan <= 64)
			lna_gain = sc->lna_gain[1];
		else if (chan <= 128)
			lna_gain = sc->lna_gain[2];
		else
			lna_gain = sc->lna_gain[3];
	}
	else
	{
		rssi_off = sc->rssi_off_2ghz[rxpath] - sc->lna_gain[0];
		lna_gain = sc->lna_gain[0];
	}

	return (-12 - rssi_off - lna_gain - rssi);
}

/*
 * rt2860_rate2mcs
 */
static uint8_t rt2860_rate2mcs(uint8_t rate)
{
	switch (rate)
	{
		/* CCK rates */
		case 2:	return 0;
		case 4:	return 1;
		case 11: return 2;
		case 22: return 3;

		/* OFDM rates */
		case 12: return 0;
		case 18: return 1;
		case 24: return 2;
		case 36: return 3;
		case 48: return 4;
		case 72: return 5;
		case 96: return 6;
		case 108: return 7;
	}

	return 0;
}

/*
 * rt2860_tx_mgmt
 */
static int rt2860_tx_mgmt(struct rt2860_softc *sc,
	struct mbuf *m, struct ieee80211_node *ni, int qid)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;
	const struct ieee80211_txparam *tp;
	struct rt2860_softc_node *rni;
	struct rt2860_softc_tx_ring *ring;
	struct rt2860_softc_tx_data *data;
	struct rt2860_txdesc *desc;
	struct rt2860_txwi *txwi;
	struct ieee80211_frame *wh;
	struct rt2860_softc_tx_radiotap_header *tap;
	bus_dma_segment_t dma_seg[RT2860_SOFTC_MAX_SCATTER];
	u_int hdrsize, hdrspace;
	uint8_t rate, mcs, pid, qsel;
	uint16_t len, dmalen, mpdu_len, dur;
	int error, mimops, ndmasegs, ndescs, i, j;

	KASSERT(qid >= 0 && qid < RT2860_SOFTC_TX_RING_COUNT,
		("%s: Tx MGMT: invalid qid=%d\n",
		 device_get_nameunit(sc->dev), qid));

	RT2860_SOFTC_TX_RING_ASSERT_LOCKED(&sc->tx_ring[qid]);

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	vap = ni->ni_vap;
	rni = (struct rt2860_softc_node *) ni;
	tp = ni->ni_txparms;

	ring = &sc->tx_ring[qid];
	desc = &ring->desc[ring->desc_cur];
	data = &ring->data[ring->data_cur];
	txwi = (struct rt2860_txwi *) (ring->seg0 + ring->data_cur * RT2860_TX_DATA_SEG0_SIZE);

	wh = mtod(m, struct ieee80211_frame *);

	rate = tp->mgmtrate & IEEE80211_RATE_VAL;
/* XXX */
	if (!rate)
		return EFBIG;

	/* fill Tx wireless info */

	if (ni->ni_flags & IEEE80211_NODE_HT)
		mcs = rate;
	else
		mcs = rt2860_rate2mcs(rate);

	/* calculate MPDU length without padding */

	hdrsize = ieee80211_anyhdrsize(wh);
	hdrspace = ieee80211_anyhdrspace(ic, wh);
	mpdu_len = m->m_pkthdr.len - hdrspace + hdrsize;

	memset(txwi, 0, sizeof(struct rt2860_txwi));

	/* management frames do not need encryption */

	txwi->wcid = RT2860_WCID_RESERVED;

	/* MIMO power save */

	if ((ni->ni_flags & IEEE80211_NODE_HT) && (ni->ni_flags & IEEE80211_NODE_MIMO_PS))
	{
		if (mcs > 7)
		{
			if (ni->ni_flags & IEEE80211_NODE_MIMO_RTS)
			{
				/* dynamic MIMO power save */

				txwi->mpdu_density_flags |=
					(RT2860_TXWI_FLAGS_MIMOPS << RT2860_TXWI_FLAGS_SHIFT);
			}
			else
			{
				/* static MIMO power save */

				mcs = 7;
			}
		}

		mimops = 1;
	}
	else
	{
		mimops = 0;
	}

	pid = (mcs < 0xf) ? (mcs + 1) : mcs;

	txwi->pid_mpdu_len = ((htole16(pid) & RT2860_TXWI_PID_MASK) <<
		RT2860_TXWI_PID_SHIFT) | ((htole16(mpdu_len) & RT2860_TXWI_MPDU_LEN_MASK) <<
			RT2860_TXWI_MPDU_LEN_SHIFT);

	if (ni->ni_flags & IEEE80211_NODE_HT)
	{
		txwi->phymode_ifs_stbc_shortgi |=
			(RT2860_TXWI_PHYMODE_HT_MIXED << RT2860_TXWI_PHYMODE_SHIFT);
	}
	else
	{
		if (ieee80211_rate2phytype(ic->ic_rt, rate) != IEEE80211_T_OFDM)
		{
			txwi->phymode_ifs_stbc_shortgi |=
				(RT2860_TXWI_PHYMODE_CCK << RT2860_TXWI_PHYMODE_SHIFT);

			if (rate != 2 && (ic->ic_flags & IEEE80211_F_SHPREAMBLE))
				mcs |= RT2860_TXWI_MCS_SHOTPRE;
		}
		else
		{
			txwi->phymode_ifs_stbc_shortgi |=
				(RT2860_TXWI_PHYMODE_OFDM << RT2860_TXWI_PHYMODE_SHIFT);
		}
	}

	txwi->bw_mcs = (RT2860_TXWI_BW_20 << RT2860_TXWI_BW_SHIFT) |
		((mcs & RT2860_TXWI_MCS_MASK) << RT2860_TXWI_MCS_SHIFT);

	txwi->txop = (RT2860_TXWI_TXOP_BACKOFF << RT2860_TXWI_TXOP_SHIFT);

	/* skip ACKs for multicast frames */

	if (!IEEE80211_IS_MULTICAST(wh->i_addr1))
	{
		txwi->bawin_size_xflags |=
			(RT2860_TXWI_XFLAGS_ACK << RT2860_TXWI_XFLAGS_SHIFT);

		if (ni->ni_flags & IEEE80211_NODE_HT)
		{
			/* preamble + plcp + signal extension + SIFS */

			dur = 16 + 4 + 6 + 10;
		}
		else
		{
			dur = ieee80211_ack_duration(ic->ic_rt, rate,
				ic->ic_flags & IEEE80211_F_SHPREAMBLE);
		}

		*(uint16_t *) wh->i_dur = htole16(dur);
	}

	/* ask MAC to insert timestamp into probe responses */

	if ((wh->i_fc[0] & (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_MASK)) ==
		(IEEE80211_FC0_TYPE_MGT | IEEE80211_FC0_SUBTYPE_PROBE_RESP))
		txwi->mpdu_density_flags |=
			(RT2860_TXWI_FLAGS_TS << RT2860_TXWI_FLAGS_SHIFT);

	if (ieee80211_radiotap_active_vap(vap))
	{
		tap = &sc->txtap;

		tap->flags = IEEE80211_RADIOTAP_F_DATAPAD;
		tap->chan_flags = htole32(ic->ic_curchan->ic_flags);
		tap->chan_freq = htole16(ic->ic_curchan->ic_freq);
		tap->chan_ieee = ic->ic_curchan->ic_ieee;
		tap->chan_maxpow = 0;

		if (ni->ni_flags & IEEE80211_NODE_HT)
			tap->rate = mcs | IEEE80211_RATE_MCS;
		else
			tap->rate = rate;

		if (mcs & RT2860_TXWI_MCS_SHOTPRE)
			tap->flags |= IEEE80211_RADIOTAP_F_SHORTPRE;

		if (wh->i_fc[1] & IEEE80211_FC1_WEP)
			tap->flags |= IEEE80211_RADIOTAP_F_WEP;

		if (wh->i_fc[1] & IEEE80211_FC1_WEP)
		{
			wh->i_fc[1] &= ~IEEE80211_FC1_WEP;

			ieee80211_radiotap_tx(vap, m);

			wh->i_fc[1] |= IEEE80211_FC1_WEP;
		}
		else
		{
			ieee80211_radiotap_tx(vap, m);
		}
	}

	/* copy and trim 802.11 header */

	m_copydata(m, 0, hdrsize, (caddr_t) (txwi + 1));
	m_adj(m, hdrspace);

	error = bus_dmamap_load_mbuf_sg(ring->data_dma_tag, data->dma_map, m,
	    dma_seg, &ndmasegs, BUS_DMA_NOWAIT);
	if (error != 0)
	{
		/* too many fragments, linearize */

		RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
			"%s: could not load mbuf DMA map, trying to linearize mbuf: ndmasegs=%d, len=%d, error=%d\n",
			device_get_nameunit(sc->dev), ndmasegs, m->m_pkthdr.len, error);

		m = m_defrag(m, M_DONTWAIT);
		if (m == NULL)
			return ENOMEM;

		sc->tx_defrag_packets++;

		error = bus_dmamap_load_mbuf_sg(ring->data_dma_tag, data->dma_map, m,
	    	dma_seg, &ndmasegs, BUS_DMA_NOWAIT);
		if (error != 0)
		{
			printf("%s: could not load mbuf DMA map: ndmasegs=%d, len=%d, error=%d\n",
				device_get_nameunit(sc->dev), ndmasegs, m->m_pkthdr.len, error);
			m_freem(m);
			return error;
		}
	}

	if (m->m_pkthdr.len == 0)
		ndmasegs = 0;

	/* determine how many Tx descs are required */

	ndescs = 1 + ndmasegs / 2;
	if ((ring->desc_queued + ndescs) > (RT2860_SOFTC_TX_RING_DESC_COUNT - 2))
	{
		RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
			"%s: there are not enough Tx descs\n",
			device_get_nameunit(sc->dev));

		sc->no_tx_desc_avail++;

		bus_dmamap_unload(ring->data_dma_tag, data->dma_map);
		m_freem(m);
		return EFBIG;
	}

	data->m = m;
	data->ni = ni;

	/* set up Tx descs */

	/* first segment is Tx wireless info and 802.11 header */

	len = sizeof(struct rt2860_txwi) + hdrsize;

	/* align end on a 4-bytes boundary */

	dmalen = (len + 3) & ~ 3;

	memset((caddr_t) txwi + len, 0, dmalen - len);

	qsel = RT2860_TXDESC_QSEL_EDCA;

	desc->sdp0 = htole32(ring->seg0_phys_addr + ring->data_cur * RT2860_TX_DATA_SEG0_SIZE);
	desc->sdl0 = htole16(dmalen);
	desc->qsel_flags = (qsel << RT2860_TXDESC_QSEL_SHIFT);

	/* set up payload segments */

	for (i = ndmasegs, j = 0; i >= 2; i -= 2)
	{
		desc->sdp1 = htole32(dma_seg[j].ds_addr);
		desc->sdl1 = htole16(dma_seg[j].ds_len);

		ring->desc_queued++;
		ring->desc_cur = (ring->desc_cur + 1) % RT2860_SOFTC_TX_RING_DESC_COUNT;

		j++;

		desc = &ring->desc[ring->desc_cur];

		desc->sdp0 = htole32(dma_seg[j].ds_addr);
		desc->sdl0 = htole16(dma_seg[j].ds_len);
		desc->qsel_flags = (qsel << RT2860_TXDESC_QSEL_SHIFT);

		j++;
	}

	/* finalize last payload segment */

	if (i > 0)
	{
		desc->sdp1 = htole32(dma_seg[j].ds_addr);
		desc->sdl1 = htole16(dma_seg[j].ds_len | RT2860_TXDESC_SDL1_LASTSEG);
	}
	else
	{
		desc->sdl0 |= htole16(RT2860_TXDESC_SDL0_LASTSEG);
		desc->sdl1 = 0;
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
		"%s: sending MGMT frame: qid=%d, hdrsize=%d, hdrspace=%d, len=%d, "
		"mcs=%d, mimops=%d, DMA len=%d, ndmasegs=%d, DMA ds_len=%d/%d/%d/%d/%d\n",
		device_get_nameunit(sc->dev),
		qid, hdrsize, hdrspace, m->m_pkthdr.len + hdrsize,
		mcs, mimops, dmalen, ndmasegs,
		(int) dma_seg[0].ds_len, (int) dma_seg[1].ds_len, (int) dma_seg[2].ds_len, (int) dma_seg[3].ds_len, (int) dma_seg[4].ds_len);

	bus_dmamap_sync(ring->seg0_dma_tag, ring->seg0_dma_map,
		BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(ring->data_dma_tag, data->dma_map,
		BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
		BUS_DMASYNC_PREWRITE);

	ring->desc_queued++;
	ring->desc_cur = (ring->desc_cur + 1) % RT2860_SOFTC_TX_RING_DESC_COUNT;

	ring->data_queued++;
	ring->data_cur = (ring->data_cur + 1) % RT2860_SOFTC_TX_RING_DATA_COUNT;

	/* kick Tx */

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_CTX_IDX(qid), ring->desc_cur);

	return 0;
}

/*
 * rt2860_tx_data
 */
static int rt2860_tx_data(struct rt2860_softc *sc,
	struct mbuf *m, struct ieee80211_node *ni, int qid)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;
	const struct ieee80211_txparam *tp;
	struct rt2860_softc_node *rni;
	struct rt2860_softc_tx_ring *ring;
	struct rt2860_softc_tx_data *data;
	struct rt2860_txdesc *desc;
	struct rt2860_txwi *txwi;
	struct ieee80211_frame *wh;
	struct ieee80211_tx_ampdu *tx_ampdu;
	ieee80211_seq seqno;
	struct rt2860_softc_tx_radiotap_header *tap;
	bus_dma_segment_t dma_seg[RT2860_SOFTC_MAX_SCATTER];
	u_int hdrsize, hdrspace;
	uint8_t type, rate, bw, stbc, shortgi, mcs, pid, wcid, mpdu_density, bawin_size, qsel;
	uint16_t qos, len, dmalen, mpdu_len, dur;
	int error, hasqos, ac, tid, ampdu, mimops, ndmasegs, ndescs, i, j;

	KASSERT(qid >= 0 && qid < RT2860_SOFTC_TX_RING_COUNT,
		("%s: Tx data: invalid qid=%d\n",
		 device_get_nameunit(sc->dev), qid));

	RT2860_SOFTC_TX_RING_ASSERT_LOCKED(&sc->tx_ring[qid]);

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	vap = ni->ni_vap;
	rni = (struct rt2860_softc_node *) ni;
	tp = ni->ni_txparms;

	ring = &sc->tx_ring[qid];
	desc = &ring->desc[ring->desc_cur];
	data = &ring->data[ring->data_cur];
	txwi = (struct rt2860_txwi *) (ring->seg0 + ring->data_cur * RT2860_TX_DATA_SEG0_SIZE);

	wh = mtod(m, struct ieee80211_frame *);

	type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;

	hasqos = IEEE80211_QOS_HAS_SEQ(wh);
	if (hasqos)
	{
		if (IEEE80211_HAS_ADDR4(wh))
			qos = le16toh(*(const uint16_t *)
				(((struct ieee80211_qosframe_addr4 *) wh)->i_qos));
		else
			qos = le16toh(*(const uint16_t *)
				(((struct ieee80211_qosframe *) wh)->i_qos));
	}
	else
	{
		qos = 0;
	}

	if (IEEE80211_IS_MULTICAST(wh->i_addr1))
		rate = tp->mcastrate;
	else if (m->m_flags & M_EAPOL)
		rate = tp->mgmtrate;
	else if (tp->ucastrate != IEEE80211_FIXED_RATE_NONE)
		rate = tp->ucastrate;
	else
		rate = ni->ni_txrate;

	rate &= IEEE80211_RATE_VAL;
/* XXX */
	if (!rate)
		return EFBIG;

	/* fill Tx wireless info */

	if (ni->ni_flags & IEEE80211_NODE_HT)
		mcs = rate;
	else
		mcs = rt2860_rate2mcs(rate);

	if (type == IEEE80211_FC0_TYPE_DATA)
		wcid = !IEEE80211_IS_MULTICAST(wh->i_addr1) ? rni->staid : RT2860_WCID_MCAST;
	else
		wcid = RT2860_WCID_RESERVED;

	/* calculate MPDU length without padding */

	hdrsize = ieee80211_anyhdrsize(wh);
	hdrspace = ieee80211_anyhdrspace(ic, wh);
	mpdu_len = m->m_pkthdr.len - hdrspace + hdrsize;

	memset(txwi, 0, sizeof(struct rt2860_txwi));

	txwi->wcid = wcid;

	/* MIMO power save */

	if ((ni->ni_flags & IEEE80211_NODE_HT) && (ni->ni_flags & IEEE80211_NODE_MIMO_PS))
	{
		if (mcs > 7)
		{
			if (ni->ni_flags & IEEE80211_NODE_MIMO_RTS)
			{
				/* dynamic MIMO power save */

				txwi->mpdu_density_flags |=
					(RT2860_TXWI_FLAGS_MIMOPS << RT2860_TXWI_FLAGS_SHIFT);
			}
			else
			{
				/* static MIMO power save */

				mcs = 7;
			}
		}

		mimops = 1;
	}
	else
	{
		mimops = 0;
	}

	pid = (mcs < 0xf) ? (mcs + 1) : mcs;

	txwi->pid_mpdu_len = ((htole16(pid) & RT2860_TXWI_PID_MASK) <<
		RT2860_TXWI_PID_SHIFT) | ((htole16(mpdu_len) & RT2860_TXWI_MPDU_LEN_MASK) <<
			RT2860_TXWI_MPDU_LEN_SHIFT);

	stbc = sc->tx_stbc && (mcs <= 7) && (vap->iv_htcaps & IEEE80211_HTCAP_TXSTBC) &&
		(ni->ni_flags & IEEE80211_NODE_HT) && (ni->ni_htcap & IEEE80211_HTCAP_RXSTBC);

	shortgi = ((vap->iv_flags_ht & IEEE80211_FHT_SHORTGI20) && (ni->ni_flags & IEEE80211_NODE_SGI20) && (ni->ni_chw == 20)) ||
		((vap->iv_flags_ht & IEEE80211_FHT_SHORTGI40) && (ni->ni_flags & IEEE80211_NODE_SGI40) && (ni->ni_chw == 40));

	txwi->phymode_ifs_stbc_shortgi |=
		((stbc & RT2860_TXWI_STBC_MASK) << RT2860_TXWI_STBC_SHIFT) |
		((shortgi & RT2860_TXWI_SHORTGI_MASK) << RT2860_TXWI_SHORTGI_SHIFT);

	if (ni->ni_flags & IEEE80211_NODE_HT)
	{
		txwi->phymode_ifs_stbc_shortgi |=
			(RT2860_TXWI_PHYMODE_HT_MIXED << RT2860_TXWI_PHYMODE_SHIFT);
	}
	else
	{
		if (ieee80211_rate2phytype(ic->ic_rt, rate) != IEEE80211_T_OFDM)
		{
			txwi->phymode_ifs_stbc_shortgi |=
				(RT2860_TXWI_PHYMODE_CCK << RT2860_TXWI_PHYMODE_SHIFT);

			if (rate != 2 && (ic->ic_flags & IEEE80211_F_SHPREAMBLE))
				mcs |= RT2860_TXWI_MCS_SHOTPRE;
		}
		else
		{
			txwi->phymode_ifs_stbc_shortgi |=
				(RT2860_TXWI_PHYMODE_OFDM << RT2860_TXWI_PHYMODE_SHIFT);
		}
	}

	if ((ni->ni_flags & IEEE80211_NODE_HT) && (ni->ni_chw == 40))
		bw = RT2860_TXWI_BW_40;
	else
		bw = RT2860_TXWI_BW_20;

	txwi->bw_mcs = ((bw & RT2860_TXWI_BW_MASK) << RT2860_TXWI_BW_SHIFT) |
		((mcs & RT2860_TXWI_MCS_MASK) << RT2860_TXWI_MCS_SHIFT);

	txwi->txop = (RT2860_TXWI_TXOP_HT << RT2860_TXWI_TXOP_SHIFT);

	if (!IEEE80211_IS_MULTICAST(wh->i_addr1) &&
		(!hasqos || (qos & IEEE80211_QOS_ACKPOLICY) != IEEE80211_QOS_ACKPOLICY_NOACK))
	{
		txwi->bawin_size_xflags |=
			(RT2860_TXWI_XFLAGS_ACK << RT2860_TXWI_XFLAGS_SHIFT);

		if (ni->ni_flags & IEEE80211_NODE_HT)
		{
			/* preamble + plcp + signal extension + SIFS */

			dur = 16 + 4 + 6 + 10;
		}
		else
		{
			dur = ieee80211_ack_duration(ic->ic_rt, rate,
				ic->ic_flags & IEEE80211_F_SHPREAMBLE);
		}

		*(uint16_t *) wh->i_dur = htole16(dur);
	}

	/* check for A-MPDU */

	if (m->m_flags & M_AMPDU_MPDU)
	{
		ac = M_WME_GETAC(m);
		tid = WME_AC_TO_TID(ac);
		tx_ampdu = &ni->ni_tx_ampdu[ac];

		mpdu_density = RT2860_MS(ni->ni_htparam, IEEE80211_HTCAP_MPDUDENSITY);
		bawin_size = tx_ampdu->txa_wnd;

		txwi->mpdu_density_flags |=
			((mpdu_density & RT2860_TXWI_MPDU_DENSITY_MASK) << RT2860_TXWI_MPDU_DENSITY_SHIFT) |
			(RT2860_TXWI_FLAGS_AMPDU << RT2860_TXWI_FLAGS_SHIFT);

		txwi->bawin_size_xflags |=
			((bawin_size & RT2860_TXWI_BAWIN_SIZE_MASK) << RT2860_TXWI_BAWIN_SIZE_SHIFT);

		seqno = ni->ni_txseqs[tid]++;

		*(uint16_t *) &wh->i_seq[0] = htole16(seqno << IEEE80211_SEQ_SEQ_SHIFT);

		ampdu = 1;
	}
	else
	{
		mpdu_density = 0;
		bawin_size = 0;
		ampdu = 0;
	}

	/* ask MAC to insert timestamp into probe responses */

	if ((wh->i_fc[0] & (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_MASK)) ==
		(IEEE80211_FC0_TYPE_MGT | IEEE80211_FC0_SUBTYPE_PROBE_RESP))
		txwi->mpdu_density_flags |=
			(RT2860_TXWI_FLAGS_TS << RT2860_TXWI_FLAGS_SHIFT);

	if (ieee80211_radiotap_active_vap(vap))
	{
		tap = &sc->txtap;

		tap->flags = IEEE80211_RADIOTAP_F_DATAPAD;
		tap->chan_flags = htole32(ic->ic_curchan->ic_flags);
		tap->chan_freq = htole16(ic->ic_curchan->ic_freq);
		tap->chan_ieee = ic->ic_curchan->ic_ieee;
		tap->chan_maxpow = 0;

		if (ni->ni_flags & IEEE80211_NODE_HT)
			tap->rate = mcs | IEEE80211_RATE_MCS;
		else
			tap->rate = rate;

		if (mcs & RT2860_TXWI_MCS_SHOTPRE)
			tap->flags |= IEEE80211_RADIOTAP_F_SHORTPRE;

		if (shortgi)
			tap->flags |= IEEE80211_RADIOTAP_F_SHORTGI;

		if (wh->i_fc[1] & IEEE80211_FC1_WEP)
			tap->flags |= IEEE80211_RADIOTAP_F_WEP;

		/* XXX use temporarily radiotap CFP flag as A-MPDU flag */

		if (ampdu)
			tap->flags |= IEEE80211_RADIOTAP_F_CFP;

		if (wh->i_fc[1] & IEEE80211_FC1_WEP)
		{
			wh->i_fc[1] &= ~IEEE80211_FC1_WEP;

			ieee80211_radiotap_tx(vap, m);

			wh->i_fc[1] |= IEEE80211_FC1_WEP;
		}
		else
		{
			ieee80211_radiotap_tx(vap, m);
		}
	}

	/* copy and trim 802.11 header */

	m_copydata(m, 0, hdrsize, (caddr_t) (txwi + 1));
	m_adj(m, hdrspace);

	error = bus_dmamap_load_mbuf_sg(ring->data_dma_tag, data->dma_map, m,
	    dma_seg, &ndmasegs, BUS_DMA_NOWAIT);
	if (error != 0)
	{
		/* too many fragments, linearize */

		RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
			"%s: could not load mbuf DMA map, trying to linearize mbuf: ndmasegs=%d, len=%d, error=%d\n",
			device_get_nameunit(sc->dev), ndmasegs, m->m_pkthdr.len, error);

		m = m_defrag(m, M_DONTWAIT);
		if (m == NULL)
			return ENOMEM;

		sc->tx_defrag_packets++;

		error = bus_dmamap_load_mbuf_sg(ring->data_dma_tag, data->dma_map, m,
	    	dma_seg, &ndmasegs, BUS_DMA_NOWAIT);
		if (error != 0)
		{
			printf("%s: could not load mbuf DMA map: ndmasegs=%d, len=%d, error=%d\n",
				device_get_nameunit(sc->dev), ndmasegs, m->m_pkthdr.len, error);
			m_freem(m);
			return error;
		}
	}

	if (m->m_pkthdr.len == 0)
		ndmasegs = 0;

	/* determine how many Tx descs are required */

	ndescs = 1 + ndmasegs / 2;
	if ((ring->desc_queued + ndescs) > (RT2860_SOFTC_TX_RING_DESC_COUNT - 2))
	{
		RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
			"%s: there are not enough Tx descs\n",
			device_get_nameunit(sc->dev));

		sc->no_tx_desc_avail++;

		bus_dmamap_unload(ring->data_dma_tag, data->dma_map);
		m_freem(m);
		return EFBIG;
	}

	data->m = m;
	data->ni = ni;

	/* set up Tx descs */

	/* first segment is Tx wireless info and 802.11 header */

	len = sizeof(struct rt2860_txwi) + hdrsize;

	/* align end on a 4-bytes boundary */

	dmalen = (len + 3) & ~ 3;

	memset((caddr_t) txwi + len, 0, dmalen - len);

	qsel = RT2860_TXDESC_QSEL_EDCA;

	desc->sdp0 = htole32(ring->seg0_phys_addr + ring->data_cur * RT2860_TX_DATA_SEG0_SIZE);
	desc->sdl0 = htole16(dmalen);
	desc->qsel_flags = (qsel << RT2860_TXDESC_QSEL_SHIFT);

	/* set up payload segments */

	for (i = ndmasegs, j = 0; i >= 2; i -= 2)
	{
		desc->sdp1 = htole32(dma_seg[j].ds_addr);
		desc->sdl1 = htole16(dma_seg[j].ds_len);

		ring->desc_queued++;
		ring->desc_cur = (ring->desc_cur + 1) % RT2860_SOFTC_TX_RING_DESC_COUNT;

		j++;

		desc = &ring->desc[ring->desc_cur];

		desc->sdp0 = htole32(dma_seg[j].ds_addr);
		desc->sdl0 = htole16(dma_seg[j].ds_len);
		desc->qsel_flags = (qsel << RT2860_TXDESC_QSEL_SHIFT);

		j++;
	}

	/* finalize last payload segment */

	if (i > 0)
	{
		desc->sdp1 = htole32(dma_seg[j].ds_addr);
		desc->sdl1 = htole16(dma_seg[j].ds_len | RT2860_TXDESC_SDL1_LASTSEG);
	}
	else
	{
		desc->sdl0 |= htole16(RT2860_TXDESC_SDL0_LASTSEG);
		desc->sdl1 = 0;
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
		"%s: sending data: qid=%d, hdrsize=%d, hdrspace=%d, len=%d, "
		"bw=%d, stbc=%d, shortgi=%d, mcs=%d, wcid=0x%02x, "
		"ampdu=%d (density=%d, winsize=%d), mimops=%d, DMA len=%d, ndmasegs=%d, DMA ds_len=%d/%d/%d/%d/%d\n",
		device_get_nameunit(sc->dev),
		qid, hdrsize, hdrspace, m->m_pkthdr.len + hdrsize,
		bw, stbc, shortgi, mcs, wcid, ampdu, mpdu_density, bawin_size, mimops, dmalen, ndmasegs,
		(int) dma_seg[0].ds_len, (int) dma_seg[1].ds_len, (int) dma_seg[2].ds_len, (int) dma_seg[3].ds_len, (int) dma_seg[4].ds_len);

	bus_dmamap_sync(ring->seg0_dma_tag, ring->seg0_dma_map,
		BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(ring->data_dma_tag, data->dma_map,
		BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
		BUS_DMASYNC_PREWRITE);

	ring->desc_queued++;
	ring->desc_cur = (ring->desc_cur + 1) % RT2860_SOFTC_TX_RING_DESC_COUNT;

	ring->data_queued++;
	ring->data_cur = (ring->data_cur + 1) % RT2860_SOFTC_TX_RING_DATA_COUNT;

	/* kick Tx */

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_CTX_IDX(qid), ring->desc_cur);

	return 0;
}

/*
 * rt2860_tx_raw
static int rt2860_tx_raw(struct rt2860_softc *sc,
	struct mbuf *m, struct ieee80211_node *ni,
	const struct ieee80211_bpf_params *params)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
		"%s: Tx raw\n",
		device_get_nameunit(sc->dev));

	return 0;
}
 */

/*
 * rt2860_intr
 */
static void rt2860_intr(void *arg)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;
	uint32_t status;

	sc = arg;
#if !defined(__HAIKU__)
	ifp = sc->ifp;

	/* acknowledge interrupts */

	status = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_INT_STATUS);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_INT_STATUS, status);

	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: interrupt: status = 0x%08x\n",
		device_get_nameunit(sc->dev), status);

	if (status == 0xffffffff ||		/* device likely went away */
		status == 0)				/* not for us */
		return;

	sc->interrupts++;

	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
		return;

#else
	status = sc->interrupt_status;
#endif

	if (status & RT2860_REG_INT_TX_COHERENT)
		rt2860_tx_coherent_intr(sc);

	if (status & RT2860_REG_INT_RX_COHERENT)
		rt2860_rx_coherent_intr(sc);

	if (status & RT2860_REG_INT_TXRX_COHERENT)
		rt2860_txrx_coherent_intr(sc);

	if (status & RT2860_REG_INT_FIFO_STA_FULL)
		rt2860_fifo_sta_full_intr(sc);

	if (status & RT2860_REG_INT_TX_MGMT_DONE)
		rt2860_tx_intr(sc, 5);

	if (status & RT2860_REG_INT_RX_DONE)
		rt2860_rx_intr(sc);

	if (status & RT2860_REG_INT_RX_DELAY_DONE)
		rt2860_rx_delay_intr(sc);

	if (status & RT2860_REG_INT_TX_HCCA_DONE)
		rt2860_tx_intr(sc, 4);

	if (status & RT2860_REG_INT_TX_AC3_DONE)
		rt2860_tx_intr(sc, 3);

	if (status & RT2860_REG_INT_TX_AC2_DONE)
		rt2860_tx_intr(sc, 2);

	if (status & RT2860_REG_INT_TX_AC1_DONE)
		rt2860_tx_intr(sc, 1);

	if (status & RT2860_REG_INT_TX_AC0_DONE)
		rt2860_tx_intr(sc, 0);

	if (status & RT2860_REG_INT_TX_DELAY_DONE)
		rt2860_tx_delay_intr(sc);

	if (status & RT2860_REG_INT_PRE_TBTT)
		rt2860_pre_tbtt_intr(sc);

	if (status & RT2860_REG_INT_TBTT)
		rt2860_tbtt_intr(sc);

	if (status & RT2860_REG_INT_MCU_CMD)
		rt2860_mcu_cmd_intr(sc);

	if (status & RT2860_REG_INT_AUTO_WAKEUP)
		rt2860_auto_wakeup_intr(sc);

	if (status & RT2860_REG_INT_GP_TIMER)
		rt2860_gp_timer_intr(sc);
}

/*
 * rt2860_tx_coherent_intr
 */
static void rt2860_tx_coherent_intr(struct rt2860_softc *sc)
{
	uint32_t tmp;
	int i;

	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: Tx coherent interrupt\n",
		device_get_nameunit(sc->dev));

	sc->tx_coherent_interrupts++;

	/* restart DMA engine */

	tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG);

	tmp &= ~(RT2860_REG_TX_WB_DDONE |
		RT2860_REG_RX_DMA_ENABLE |
		RT2860_REG_TX_DMA_ENABLE);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG, tmp);

	/* init Tx rings (4 EDCAs + HCCA + MGMT) */

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
		rt2860_reset_tx_ring(sc, &sc->tx_ring[i]);

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
	{
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_BASE_PTR(i),
			sc->tx_ring[i].desc_phys_addr);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_MAX_CNT(i),
			RT2860_SOFTC_TX_RING_DESC_COUNT);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_CTX_IDX(i), 0);
	}

	/* init Rx ring */

	rt2860_reset_rx_ring(sc, &sc->rx_ring);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_BASE_PTR,
		sc->rx_ring.desc_phys_addr);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_MAX_CNT,
		RT2860_SOFTC_RX_RING_DATA_COUNT);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_CALC_IDX,
		RT2860_SOFTC_RX_RING_DATA_COUNT - 1);

	rt2860_txrx_enable(sc);
}

/*
 * rt2860_rx_coherent_intr
 */
static void rt2860_rx_coherent_intr(struct rt2860_softc *sc)
{
	uint32_t tmp;
	int i;

	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: Rx coherent interrupt\n",
		device_get_nameunit(sc->dev));

	sc->rx_coherent_interrupts++;

	/* restart DMA engine */

	tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG);

	tmp &= ~(RT2860_REG_TX_WB_DDONE |
		RT2860_REG_RX_DMA_ENABLE |
		RT2860_REG_TX_DMA_ENABLE);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG, tmp);

	/* init Tx rings (4 EDCAs + HCCA + MGMT) */

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
		rt2860_reset_tx_ring(sc, &sc->tx_ring[i]);

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
	{
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_BASE_PTR(i),
			sc->tx_ring[i].desc_phys_addr);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_MAX_CNT(i),
			RT2860_SOFTC_TX_RING_DESC_COUNT);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_CTX_IDX(i), 0);
	}

	/* init Rx ring */

	rt2860_reset_rx_ring(sc, &sc->rx_ring);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_BASE_PTR,
		sc->rx_ring.desc_phys_addr);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_MAX_CNT,
		RT2860_SOFTC_RX_RING_DATA_COUNT);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_CALC_IDX,
		RT2860_SOFTC_RX_RING_DATA_COUNT - 1);

	rt2860_txrx_enable(sc);
}

/*
 * rt2860_txrx_coherent_intr
 */
static void rt2860_txrx_coherent_intr(struct rt2860_softc *sc)
{
	uint32_t tmp;
	int i;

	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: Tx/Rx coherent interrupt\n",
		device_get_nameunit(sc->dev));

	sc->txrx_coherent_interrupts++;

	/* restart DMA engine */

	tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG);

	tmp &= ~(RT2860_REG_TX_WB_DDONE |
		RT2860_REG_RX_DMA_ENABLE |
		RT2860_REG_TX_DMA_ENABLE);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG, tmp);

	/* init Tx rings (4 EDCAs + HCCA + MGMT) */

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
		rt2860_reset_tx_ring(sc, &sc->tx_ring[i]);

	for (i = 0; i < RT2860_SOFTC_TX_RING_COUNT; i++)
	{
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_BASE_PTR(i),
			sc->tx_ring[i].desc_phys_addr);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_MAX_CNT(i),
			RT2860_SOFTC_TX_RING_DESC_COUNT);
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_TX_CTX_IDX(i), 0);
	}

	/* init Rx ring */

	rt2860_reset_rx_ring(sc, &sc->rx_ring);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_BASE_PTR,
		sc->rx_ring.desc_phys_addr);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_MAX_CNT,
		RT2860_SOFTC_RX_RING_DATA_COUNT);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_CALC_IDX,
		RT2860_SOFTC_RX_RING_DATA_COUNT - 1);

	rt2860_txrx_enable(sc);
}

/*
 * rt2860_fifo_sta_full_intr
 */
static void rt2860_fifo_sta_full_intr(struct rt2860_softc *sc)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: FIFO statistic full interrupt\n",
		device_get_nameunit(sc->dev));

	sc->fifo_sta_full_interrupts++;

	RT2860_SOFTC_LOCK(sc);

	if (!(sc->intr_disable_mask & RT2860_REG_INT_FIFO_STA_FULL))
	{
		rt2860_intr_disable(sc, RT2860_REG_INT_FIFO_STA_FULL);

		taskqueue_enqueue(sc->taskqueue, &sc->fifo_sta_full_task);
	}

	sc->intr_pending_mask |= RT2860_REG_INT_FIFO_STA_FULL;

	RT2860_SOFTC_UNLOCK(sc);
}

/*
 * rt2860_rx_intr
 */
static void rt2860_rx_intr(struct rt2860_softc *sc)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: Rx interrupt\n",
		device_get_nameunit(sc->dev));

	sc->rx_interrupts++;

	RT2860_SOFTC_LOCK(sc);

	if (!(sc->intr_disable_mask & RT2860_REG_INT_RX_DONE))
	{
		rt2860_intr_disable(sc, RT2860_REG_INT_RX_DONE);

		taskqueue_enqueue(sc->taskqueue, &sc->rx_done_task);
	}

	sc->intr_pending_mask |= RT2860_REG_INT_RX_DONE;

	RT2860_SOFTC_UNLOCK(sc);
}

/*
 * rt2860_rx_delay_intr
 */
static void rt2860_rx_delay_intr(struct rt2860_softc *sc)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: Rx delay interrupt\n",
		device_get_nameunit(sc->dev));

	sc->rx_delay_interrupts++;
}

/*
 * rt2860_tx_intr
 */
static void rt2860_tx_intr(struct rt2860_softc *sc, int qid)
{
	KASSERT(qid >= 0 && qid < RT2860_SOFTC_TX_RING_COUNT,
		("%s: Tx interrupt: invalid qid=%d\n",
		 device_get_nameunit(sc->dev), qid));

	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: Tx interrupt: qid=%d\n",
		device_get_nameunit(sc->dev), qid);

	sc->tx_interrupts[qid]++;

	RT2860_SOFTC_LOCK(sc);

	if (!(sc->intr_disable_mask & (RT2860_REG_INT_TX_AC0_DONE << qid)))
	{
		rt2860_intr_disable(sc, (RT2860_REG_INT_TX_AC0_DONE << qid));

		taskqueue_enqueue(sc->taskqueue, &sc->tx_done_task);
	}

	sc->intr_pending_mask |= (RT2860_REG_INT_TX_AC0_DONE << qid);

	RT2860_SOFTC_UNLOCK(sc);
}

/*
 * rt2860_tx_delay_intr
 */
static void rt2860_tx_delay_intr(struct rt2860_softc *sc)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: Tx delay interrupt\n",
		device_get_nameunit(sc->dev));

	sc->tx_delay_interrupts++;
}

/*
 * rt2860_pre_tbtt_intr
 */
static void rt2860_pre_tbtt_intr(struct rt2860_softc *sc)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: Pre-TBTT interrupt\n",
		device_get_nameunit(sc->dev));

	sc->pre_tbtt_interrupts++;
}

/*
 * rt2860_tbtt_intr
 */
static void rt2860_tbtt_intr(struct rt2860_softc *sc)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: TBTT interrupt\n",
		device_get_nameunit(sc->dev));

	sc->tbtt_interrupts++;
}

/*
 * rt2860_mcu_cmd_intr
 */
static void rt2860_mcu_cmd_intr(struct rt2860_softc *sc)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: MCU command interrupt\n",
		device_get_nameunit(sc->dev));

	sc->mcu_cmd_interrupts++;
}

/*
 * rt2860_auto_wakeup_intr
 */
static void rt2860_auto_wakeup_intr(struct rt2860_softc *sc)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: auto wakeup interrupt\n",
		device_get_nameunit(sc->dev));

	sc->auto_wakeup_interrupts++;
}

/*
 * rt2860_gp_timer_intr
 */
static void rt2860_gp_timer_intr(struct rt2860_softc *sc)
{
	RT2860_DPRINTF(sc, RT2860_DEBUG_INTR,
		"%s: GP timer interrupt\n",
		device_get_nameunit(sc->dev));

	sc->gp_timer_interrupts++;
}

/*
 * rt2860_rx_done_task
 */
static void rt2860_rx_done_task(void *context, int pending)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;
	int again;

	sc = context;
	ifp = sc->ifp;

	RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
		"%s: Rx done task\n",
		device_get_nameunit(sc->dev));

	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
		return;

	sc->intr_pending_mask &= ~RT2860_REG_INT_RX_DONE;

	again = rt2860_rx_eof(sc, sc->rx_process_limit);

	RT2860_SOFTC_LOCK(sc);

	if ((sc->intr_pending_mask & RT2860_REG_INT_RX_DONE) || again)
	{
		RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
			"%s: Rx done task: scheduling again\n",
			device_get_nameunit(sc->dev));

		taskqueue_enqueue(sc->taskqueue, &sc->rx_done_task);
	}
	else
	{
		rt2860_intr_enable(sc, RT2860_REG_INT_RX_DONE);
	}

	RT2860_SOFTC_UNLOCK(sc);
}

/*
 * rt2860_tx_done_task
 */
static void rt2860_tx_done_task(void *context, int pending)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;
	uint32_t intr_mask;
	int i;

	sc = context;
	ifp = sc->ifp;

	RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
		"%s: Tx done task\n",
		device_get_nameunit(sc->dev));

	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
		return;

	for (i = RT2860_SOFTC_TX_RING_COUNT - 1; i >= 0; i--)
	{
		if (sc->intr_pending_mask & (RT2860_REG_INT_TX_AC0_DONE << i))
		{
			sc->intr_pending_mask &= ~(RT2860_REG_INT_TX_AC0_DONE << i);

			rt2860_tx_eof(sc, &sc->tx_ring[i]);
		}
	}

	sc->tx_timer = 0;

	ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;

	intr_mask = (RT2860_REG_INT_TX_MGMT_DONE |
		RT2860_REG_INT_TX_HCCA_DONE |
		RT2860_REG_INT_TX_AC3_DONE |
		RT2860_REG_INT_TX_AC2_DONE |
		RT2860_REG_INT_TX_AC1_DONE |
		RT2860_REG_INT_TX_AC0_DONE);

	RT2860_SOFTC_LOCK(sc);

	rt2860_intr_enable(sc, ~sc->intr_pending_mask &
		(sc->intr_disable_mask & intr_mask));

	if (sc->intr_pending_mask & intr_mask)
	{
		RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
			"%s: Tx done task: scheduling again\n",
			device_get_nameunit(sc->dev));

		taskqueue_enqueue(sc->taskqueue, &sc->tx_done_task);
	}

	RT2860_SOFTC_UNLOCK(sc);

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		rt2860_start(ifp);
}

/*
 * rt2860_fifo_sta_full_task
 */
static void rt2860_fifo_sta_full_task(void *context, int pending)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;

	sc = context;
	ifp = sc->ifp;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATS,
		"%s: FIFO statistic full task\n",
		device_get_nameunit(sc->dev));

	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
		return;

	sc->intr_pending_mask &= ~RT2860_REG_INT_FIFO_STA_FULL;

	rt2860_drain_fifo_stats(sc);

	RT2860_SOFTC_LOCK(sc);

	if (sc->intr_pending_mask & RT2860_REG_INT_FIFO_STA_FULL)
	{
		RT2860_DPRINTF(sc, RT2860_DEBUG_STATS,
			"%s: FIFO statistic full task: scheduling again\n",
			device_get_nameunit(sc->dev));

		taskqueue_enqueue(sc->taskqueue, &sc->fifo_sta_full_task);
	}
	else
	{
		rt2860_intr_enable(sc, RT2860_REG_INT_FIFO_STA_FULL);
	}

	RT2860_SOFTC_UNLOCK(sc);

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		rt2860_start(ifp);
}

/*
 * rt2860_periodic_task
 */
static void rt2860_periodic_task(void *context, int pending)
{
	struct rt2860_softc *sc;
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;

	sc = context;
	ifp = sc->ifp;
	ic = ifp->if_l2com;
	vap = TAILQ_FIRST(&ic->ic_vaps);

	RT2860_DPRINTF(sc, RT2860_DEBUG_PERIODIC,
		"%s: periodic task: round=%lu\n",
		device_get_nameunit(sc->dev), sc->periodic_round);

	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
		return;

	RT2860_SOFTC_LOCK(sc);

	sc->periodic_round++;

	rt2860_update_stats(sc);

	if ((sc->periodic_round % 10) == 0)
	{
		rt2860_bbp_tuning(sc);

		rt2860_update_raw_counters(sc);

		rt2860_watchdog(sc);

		if (vap != NULL && vap->iv_opmode != IEEE80211_M_MONITOR && vap->iv_state == IEEE80211_S_RUN)
		{
			if (vap->iv_opmode == IEEE80211_M_STA)
				rt2860_amrr_update_iter_func(vap, vap->iv_bss);
			else
				ieee80211_iterate_nodes(&ic->ic_sta, rt2860_amrr_update_iter_func, vap);
		}
	}

	RT2860_SOFTC_UNLOCK(sc);

	callout_reset(&sc->periodic_ch, hz / 10, rt2860_periodic, sc);
}

/*
 * rt2860_rx_eof
 */
static int rt2860_rx_eof(struct rt2860_softc *sc, int limit)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211_frame *wh;
	struct ieee80211_node *ni;
	struct rt2860_softc_node *rni;
	struct rt2860_softc_rx_radiotap_header *tap;
	struct rt2860_softc_rx_ring *ring;
	struct rt2860_rxdesc *desc;
	struct rt2860_softc_rx_data *data;
	struct rt2860_rxwi *rxwi;
	struct mbuf *m, *mnew;
	bus_dma_segment_t segs[1];
	bus_dmamap_t dma_map;
	uint32_t index, desc_flags;
	uint8_t cipher_err, rssi, ant, phymode, bw, shortgi, stbc, mcs, keyidx, tid, frag;
	uint16_t seq;
	int8_t rssi_dbm;
	int error, nsegs, len, ampdu, amsdu, rssi_dbm_rel, nframes, i;
	
	ifp = sc->ifp;
	ic = ifp->if_l2com;
	ring = &sc->rx_ring;

	nframes = 0;

	while (limit != 0)
	{
		index = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_RX_DRX_IDX);
		if (ring->cur == index)
			break;

		desc = &ring->desc[ring->cur];
		data = &ring->data[ring->cur];

		bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
			BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
#ifdef XXX_TESTED_AND_WORKED
		if (!(desc->sdl0 & htole16(RT2860_RXDESC_SDL0_DDONE)))
			break;
#endif

		nframes++;

		mnew = m_getjcl(M_DONTWAIT, MT_DATA, M_PKTHDR, MJUMPAGESIZE);
		if (mnew == NULL)
		{
			sc->rx_mbuf_alloc_errors++;
			ifp->if_ierrors++;
			goto skip;
		}

		mnew->m_len = mnew->m_pkthdr.len = MJUMPAGESIZE;

		error = bus_dmamap_load_mbuf_sg(ring->data_dma_tag, ring->spare_dma_map,
			mnew, segs, &nsegs, BUS_DMA_NOWAIT);
		if (error != 0)
		{
			RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
				"%s: could not load Rx mbuf DMA map: error=%d, nsegs=%d\n",
				device_get_nameunit(sc->dev), error, nsegs);

			m_freem(mnew);

			sc->rx_mbuf_dmamap_errors++;
			ifp->if_ierrors++;

			goto skip;
		}

		KASSERT(nsegs == 1, ("%s: too many DMA segments",
			device_get_nameunit(sc->dev)));

		bus_dmamap_sync(ring->data_dma_tag, data->dma_map,
			BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(ring->data_dma_tag, data->dma_map);

		dma_map = data->dma_map;
		data->dma_map = ring->spare_dma_map;
		ring->spare_dma_map = dma_map;

		bus_dmamap_sync(ring->data_dma_tag, data->dma_map,
			BUS_DMASYNC_PREREAD);

		m = data->m;

		data->m = mnew;
		desc->sdp0 = htole32(segs[0].ds_addr);

		desc_flags = le32toh(desc->flags);

		RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
			"%s: Rx frame: rxdesc flags=0x%08x\n",
			device_get_nameunit(sc->dev), desc_flags);

		/* get Rx wireless info */

		rxwi = mtod(m, struct rt2860_rxwi *);
		len = (le16toh(rxwi->tid_size) >> RT2860_RXWI_SIZE_SHIFT) &
			RT2860_RXWI_SIZE_MASK;

		/* check for L2 padding between IEEE 802.11 frame header and body */

		if (desc_flags & RT2860_RXDESC_FLAGS_L2PAD)
		{
			RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
				"%s: L2 padding: len=%d\n",
				device_get_nameunit(sc->dev), len);

			len += 2;
		}

		m->m_pkthdr.rcvif = ifp;
		m->m_data = (caddr_t) (rxwi + 1);
		m->m_pkthdr.len = m->m_len = len;

		/* check for crc errors */

		if (desc_flags & RT2860_RXDESC_FLAGS_CRC_ERR)
		{
			RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
				"%s: rxdesc: crc error\n",
				device_get_nameunit(sc->dev));

			ifp->if_ierrors++;

			if (!(ifp->if_flags & IFF_PROMISC))
			{
				m_freem(m);
				goto skip;
			}
		}

		wh = (struct ieee80211_frame *) (rxwi + 1);

		/* check for cipher errors */

		if (desc_flags & RT2860_RXDESC_FLAGS_DECRYPTED)
		{
			cipher_err = ((desc_flags >> RT2860_RXDESC_FLAGS_CIPHER_ERR_SHIFT) &
				RT2860_RXDESC_FLAGS_CIPHER_ERR_MASK);
			if (cipher_err == RT2860_RXDESC_FLAGS_CIPHER_ERR_NONE)
			{
				if (wh->i_fc[1] & IEEE80211_FC1_WEP)
					wh->i_fc[1] &= ~IEEE80211_FC1_WEP;

				m->m_flags |= M_WEP;

				sc->rx_cipher_no_errors++;
			}
			else
			{
				RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
					"%s: rxdesc: cipher error=0x%02x\n",
					device_get_nameunit(sc->dev), cipher_err);

				if (cipher_err == RT2860_RXDESC_FLAGS_CIPHER_ERR_ICV)
					sc->rx_cipher_icv_errors++;
				else if (cipher_err == RT2860_RXDESC_FLAGS_CIPHER_ERR_MIC)
					sc->rx_cipher_mic_errors++;
				else if (cipher_err == RT2860_RXDESC_FLAGS_CIPHER_ERR_INVALID_KEY)
					sc->rx_cipher_invalid_key_errors++;

				if ((cipher_err == RT2860_RXDESC_FLAGS_CIPHER_ERR_MIC) &&
					(desc_flags & RT2860_RXDESC_FLAGS_MYBSS))
				{
					ni = ieee80211_find_rxnode(ic, (const struct ieee80211_frame_min *) wh);
					if (ni != NULL)
					{
						keyidx = (rxwi->udf_bssidx_keyidx >> RT2860_RXWI_KEYIDX_SHIFT) &
							RT2860_RXWI_KEYIDX_MASK;

						ieee80211_notify_michael_failure(ni->ni_vap, wh, keyidx);

						ieee80211_free_node(ni);
					}
				}

				ifp->if_ierrors++;

				if (!(ifp->if_flags & IFF_PROMISC))
				{
					m_free(m);
					goto skip;
				}
			}
		}
		else
		{
			if (wh->i_fc[1] & IEEE80211_FC1_WEP)
			{
				RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
					"%s: rxdesc: not decrypted but protected flag set\n",
					device_get_nameunit(sc->dev));

				ifp->if_ierrors++;

				if (!(ifp->if_flags & IFF_PROMISC))
				{
					m_free(m);
					goto skip;
				}
			}
		}

		/* check for A-MPDU */

		if (desc_flags & RT2860_RXDESC_FLAGS_BA)
		{
			m->m_flags |= M_AMPDU;

			sc->rx_ampdu++;

			if (wh->i_fc[1] & IEEE80211_FC1_RETRY)
				sc->rx_ampdu_retries++;

			ampdu = 1;
		}
		else
		{
			ampdu = 0;
		}

		/* check for A-MSDU */

		if (desc_flags & RT2860_RXDESC_FLAGS_AMSDU)
		{
			sc->rx_amsdu++;

			amsdu = 1;
		}
		else
		{
			amsdu = 0;
		}

		ant = rt2860_maxrssi_rxpath(sc, rxwi);
		rssi = rxwi->rssi[ant];
		rssi_dbm = rt2860_rssi2dbm(sc, rssi, ant);
		phymode = ((rxwi->phymode_stbc_shortgi >> RT2860_RXWI_PHYMODE_SHIFT) &
			RT2860_RXWI_PHYMODE_MASK);
		bw = ((rxwi->bw_mcs >> RT2860_RXWI_BW_SHIFT) & RT2860_RXWI_BW_MASK);
		shortgi = ((rxwi->phymode_stbc_shortgi >> RT2860_RXWI_SHORTGI_SHIFT) &
			RT2860_RXWI_SHORTGI_MASK);
		stbc = ((rxwi->phymode_stbc_shortgi >> RT2860_RXWI_STBC_SHIFT) &
			RT2860_RXWI_STBC_MASK);
		mcs = ((rxwi->bw_mcs >> RT2860_RXWI_MCS_SHIFT) & RT2860_RXWI_MCS_MASK);
		tid = ((rxwi->tid_size >> RT2860_RXWI_TID_SHIFT) & RT2860_RXWI_TID_MASK);
		seq = ((rxwi->seq_frag >> RT2860_RXWI_SEQ_SHIFT) & RT2860_RXWI_SEQ_MASK);
		frag = ((rxwi->seq_frag >> RT2860_RXWI_FRAG_SHIFT) & RT2860_RXWI_FRAG_MASK);

		if (ieee80211_radiotap_active(ic))
		{
			tap = &sc->rxtap;

			tap->flags = (desc_flags & RT2860_RXDESC_FLAGS_L2PAD) ? IEEE80211_RADIOTAP_F_DATAPAD : 0;
			tap->dbm_antsignal = rssi_dbm;
			tap->dbm_antnoise = RT2860_NOISE_FLOOR;
			tap->antenna = ant;
			tap->antsignal = rssi;
			tap->chan_flags = htole32(ic->ic_curchan->ic_flags);
			tap->chan_freq = htole16(ic->ic_curchan->ic_freq);
			tap->chan_ieee = ic->ic_curchan->ic_ieee;
			tap->chan_maxpow = 0;

			if (phymode == RT2860_TXWI_PHYMODE_HT_MIXED || phymode == RT2860_TXWI_PHYMODE_HT_GF)
				tap->rate = mcs | IEEE80211_RATE_MCS;
			else
				tap->rate = rt2860_rxrate(rxwi);

			if (desc_flags & RT2860_RXDESC_FLAGS_CRC_ERR)
				tap->flags |= IEEE80211_RADIOTAP_F_BADFCS;

			if (desc_flags & RT2860_RXDESC_FLAGS_FRAG)
				tap->flags |= IEEE80211_RADIOTAP_F_FRAG;

			if (rxwi->bw_mcs & RT2860_RXWI_MCS_SHOTPRE)
				tap->flags |= IEEE80211_RADIOTAP_F_SHORTPRE;

			if ((desc_flags & RT2860_RXDESC_FLAGS_DECRYPTED) ||
				(wh->i_fc[1] & IEEE80211_FC1_WEP))
				tap->flags |= IEEE80211_RADIOTAP_F_WEP;

			if (shortgi)
				tap->flags |= IEEE80211_RADIOTAP_F_SHORTGI;

			/* XXX use temporarily radiotap CFP flag as A-MPDU flag */

			if (ampdu)
				tap->flags |= IEEE80211_RADIOTAP_F_CFP;
		}

		/*
		 * net80211 assumes that RSSI data are in the range [-127..127] and
		 * in .5 dBm units relative to the current noise floor
		 */

		rssi_dbm_rel = (rssi_dbm - RT2860_NOISE_FLOOR) * 2;
		if (rssi_dbm_rel > 127)
			rssi_dbm_rel = 127;

		RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
			"%s: received frame: len=%d, phymode=%d, bw=%d, shortgi=%d, stbc=0x%02x, mcs=%d, "
			"ant=%d, rssi=%d/%d/%d, snr=%d/%d, wcid=0x%02x, ampdu=%d, amsdu=%d, tid=%d, seq=%d, frag=%d, "
			"retry=%d, rssi_dbm=%d, rssi_dbm_rel=%d\n",
			device_get_nameunit(sc->dev),
			len, phymode, bw, shortgi, stbc, mcs,
			ant, rxwi->rssi[0], rxwi->rssi[1], rxwi->rssi[2],
			rxwi->snr[0], rxwi->snr[1],
			rxwi->wcid, ampdu, amsdu, tid, seq, frag, (wh->i_fc[1] & IEEE80211_FC1_RETRY) ? 1 : 0,
			rssi_dbm, rssi_dbm_rel);

		ni = ieee80211_find_rxnode(ic, (struct ieee80211_frame_min *) wh);
		if (ni != NULL)
		{
			rni = (struct rt2860_softc_node *) ni;

			for (i = 0; i < RT2860_SOFTC_RSSI_COUNT; i++)
			{
				rni->last_rssi[i] = rxwi->rssi[i];
				rni->last_rssi_dbm[i] = rt2860_rssi2dbm(sc, rxwi->rssi[i], i);
			}

			ieee80211_input(ni, m, rssi_dbm_rel, RT2860_NOISE_FLOOR);
			ieee80211_free_node(ni);
		}
		else
		{
			ieee80211_input_all(ic, m, rssi_dbm_rel, RT2860_NOISE_FLOOR);
		}

skip:

		desc->sdl0 &= ~htole16(RT2860_RXDESC_SDL0_DDONE);

		bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		ring->cur = (ring->cur + 1) % RT2860_SOFTC_RX_RING_DATA_COUNT;

		limit--;
	}

	if (ring->cur == 0)
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_CALC_IDX,
			RT2860_SOFTC_RX_RING_DATA_COUNT - 1);
	else
		rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_RX_CALC_IDX,
			ring->cur - 1);

	RT2860_DPRINTF(sc, RT2860_DEBUG_RX,
		"%s: Rx eof: nframes=%d\n",
		device_get_nameunit(sc->dev), nframes);

	sc->rx_packets += nframes;

	return (limit == 0);
}

/*
 * rt2860_tx_eof
 */
static void rt2860_tx_eof(struct rt2860_softc *sc,
	struct rt2860_softc_tx_ring *ring)
{
	struct ifnet *ifp;
	struct rt2860_txdesc *desc;
	struct rt2860_softc_tx_data *data;
	uint32_t index;
	int ndescs, nframes;

	ifp = sc->ifp;

	ndescs = 0;
	nframes = 0;

	for (;;)
	{
		index = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_TX_DTX_IDX(ring->qid));
		if (ring->desc_next == index)
			break;

		ndescs++;

		rt2860_drain_fifo_stats(sc);

		desc = &ring->desc[ring->desc_next];

		bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
			BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		if (desc->sdl0 & htole16(RT2860_TXDESC_SDL0_LASTSEG) ||
			desc->sdl1 & htole16(RT2860_TXDESC_SDL1_LASTSEG))
		{
			nframes++;

			data = &ring->data[ring->data_next];

			if (data->m->m_flags & M_TXCB)
				ieee80211_process_callback(data->ni, data->m, 0);

			bus_dmamap_sync(ring->data_dma_tag, data->dma_map,
				BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(ring->data_dma_tag, data->dma_map);

			m_freem(data->m);

			ieee80211_free_node(data->ni);

			data->m = NULL;
			data->ni = NULL;

			ifp->if_opackets++;

			RT2860_SOFTC_TX_RING_LOCK(ring);

			ring->data_queued--;
			ring->data_next = (ring->data_next + 1) % RT2860_SOFTC_TX_RING_DATA_COUNT;

			RT2860_SOFTC_TX_RING_UNLOCK(ring);
		}

		desc->sdl0 &= ~htole16(RT2860_TXDESC_SDL0_DDONE);

		bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		RT2860_SOFTC_TX_RING_LOCK(ring);

		ring->desc_queued--;
		ring->desc_next = (ring->desc_next + 1) % RT2860_SOFTC_TX_RING_DESC_COUNT;

		RT2860_SOFTC_TX_RING_UNLOCK(ring);
	}

	RT2860_DPRINTF(sc, RT2860_DEBUG_TX,
		"%s: Tx eof: qid=%d, ndescs=%d, nframes=%d\n",
		device_get_nameunit(sc->dev), ring->qid, ndescs, nframes);
}

/*
 * rt2860_update_stats
 */
static void rt2860_update_stats(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	uint32_t stacnt[3];
	int beacons, noretryok, retryok, failed, underflows, zerolen;

	ifp = sc->ifp;
	ic = ifp->if_l2com;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATS,
		"%s: update statistic\n",
		device_get_nameunit(sc->dev));

	rt2860_drain_fifo_stats(sc);

	/* read and clear Tx statistic registers */

	rt2860_io_mac_read_multi(sc, RT2860_REG_TX_STA_CNT0,
		stacnt, sizeof(stacnt));

	stacnt[0] = le32toh(stacnt[0]);
	stacnt[1] = le32toh(stacnt[1]);
	stacnt[2] = le32toh(stacnt[2]);

	beacons = stacnt[0] >> 16;
	noretryok = stacnt[1] & 0xffff;
	retryok = stacnt[1] >> 16;
	failed = stacnt[0] & 0xffff;
	underflows = stacnt[2] >> 16;
	zerolen = stacnt[2] & 0xffff;

	RT2860_DPRINTF(sc, RT2860_DEBUG_STATS,
		"%s: update statistic: beacons=%d, noretryok=%d, retryok=%d, failed=%d, underflows=%d, zerolen=%d\n",
		device_get_nameunit(sc->dev),
		beacons, noretryok, retryok, failed, underflows, zerolen);

	ifp->if_oerrors += failed;

	sc->tx_beacons += beacons;
	sc->tx_noretryok += noretryok;
	sc->tx_retryok += retryok;
	sc->tx_failed += failed;
	sc->tx_underflows += underflows;
	sc->tx_zerolen += zerolen;
}

/*
 * rt2860_bbp_tuning
 */
static void rt2860_bbp_tuning(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;
	struct ieee80211_node *ni;
	int chan, group;
	int8_t rssi, old, new;

	/* RT2860C does not support BBP tuning */

	if (sc->mac_rev == 0x28600100)
		return;

	ifp = sc->ifp;
	ic = ifp->if_l2com;
	vap = TAILQ_FIRST(&ic->ic_vaps);

	if ((ic->ic_flags & IEEE80211_F_SCAN) || vap == NULL ||
		vap->iv_opmode != IEEE80211_M_STA || vap->iv_state != IEEE80211_S_RUN)
		return;

	ni = vap->iv_bss;

	chan = ieee80211_chan2ieee(ic, ni->ni_chan);

	if (chan <= 14)
		group = 0;
	else if (chan <= 64)
		group = 1;
	else if (chan <= 128)
		group = 2;
	else
		group = 3;

	rssi = ieee80211_getrssi(vap);

	if (IEEE80211_IS_CHAN_2GHZ(ni->ni_chan))
	{
		new = 0x2e + sc->lna_gain[group];
	}
	else
	{
		if (!IEEE80211_IS_CHAN_HT40(ni->ni_chan))
			new = 0x32 + sc->lna_gain[group] * 5 / 3;
		else
			new = 0x3a + sc->lna_gain[group] * 5 / 3;
	}

	/* Tune if absolute average RSSI is greater than -80 */

	if (rssi > 30)
		new += 0x10;

	old = rt2860_io_bbp_read(sc, 66);

	if (old != new)
		rt2860_io_bbp_write(sc, 66, new);
}

/*
 * rt2860_watchdog
 */
static void rt2860_watchdog(struct rt2860_softc *sc)
{
	uint32_t tmp;
	int ntries;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_PBF_TXRXQ_PCNT);

	RT2860_DPRINTF(sc, RT2860_DEBUG_WATCHDOG,
		"%s: watchdog: TXRXQ_PCNT=0x%08x\n",
		device_get_nameunit(sc->dev), tmp);

	if (((tmp >> RT2860_REG_TX0Q_PCNT_SHIFT) & RT2860_REG_TX0Q_PCNT_MASK) != 0)
	{
		sc->tx_queue_not_empty[0]++;

		rt2860_io_mac_write(sc, RT2860_REG_PBF_CFG, 0xf40012);

		for (ntries = 0; ntries < 10; ntries++)
		{
			tmp = rt2860_io_mac_read(sc, RT2860_REG_PBF_TXRXQ_PCNT);
			if (((tmp >> RT2860_REG_TX0Q_PCNT_SHIFT) & RT2860_REG_TX0Q_PCNT_MASK) == 0)
				break;

			DELAY(1);
		}

		rt2860_io_mac_write(sc, RT2860_REG_PBF_CFG, 0xf40006);
	}

	if (((tmp >> RT2860_REG_TX1Q_PCNT_SHIFT) & RT2860_REG_TX1Q_PCNT_MASK) != 0)
	{
		sc->tx_queue_not_empty[1]++;

		rt2860_io_mac_write(sc, RT2860_REG_PBF_CFG, 0xf4000a);

		for (ntries = 0; ntries < 10; ntries++)
		{
			tmp = rt2860_io_mac_read(sc, RT2860_REG_PBF_TXRXQ_PCNT);
			if (((tmp >> RT2860_REG_TX1Q_PCNT_SHIFT) & RT2860_REG_TX1Q_PCNT_MASK) == 0)
				break;

			DELAY(1);
		}

		rt2860_io_mac_write(sc, RT2860_REG_PBF_CFG, 0xf40006);
	}
}

/*
 * rt2860_drain_fifo_stats
 */
static void rt2860_drain_fifo_stats(struct rt2860_softc *sc)
{
	struct ifnet *ifp;
	uint32_t stats;
	uint8_t wcid, mcs, pid;
	int ok, agg, retrycnt;

	ifp = sc->ifp;

	/* drain Tx status FIFO (maxsize = 16) */

	while ((stats = rt2860_io_mac_read(sc, RT2860_REG_TX_STA_FIFO)) &
		RT2860_REG_TX_STA_FIFO_VALID)
	{
		wcid = (stats >> RT2860_REG_TX_STA_FIFO_WCID_SHIFT) &
			RT2860_REG_TX_STA_FIFO_WCID_MASK;

		/* if no ACK was requested, no feedback is available */

		if (!(stats & RT2860_REG_TX_STA_FIFO_ACK_REQ) || wcid == RT2860_WCID_RESERVED)
			continue;

		/* update AMRR statistic */

		ok = (stats & RT2860_REG_TX_STA_FIFO_TX_OK) ? 1 : 0;
		agg = (stats & RT2860_REG_TX_STA_FIFO_AGG) ? 1 : 0;
		mcs = (stats >> RT2860_REG_TX_STA_FIFO_MCS_SHIFT) &
			RT2860_REG_TX_STA_FIFO_MCS_MASK;
		pid = (stats >> RT2860_REG_TX_STA_FIFO_PID_SHIFT) &
			RT2860_REG_TX_STA_FIFO_PID_MASK;
		retrycnt = (mcs < 0xf) ? (pid - mcs - 1) : 0;

		RT2860_DPRINTF(sc, RT2860_DEBUG_STATS,
			"%s: FIFO statistic: wcid=0x%02x, ok=%d, agg=%d, mcs=0x%02x, pid=0x%02x, retrycnt=%d\n",
			device_get_nameunit(sc->dev),
			wcid, ok, agg, mcs, pid, retrycnt);

		rt2860_amrr_tx_complete(&sc->amrr_node[wcid], ok, retrycnt);

		if (!ok)
			ifp->if_oerrors++;
	}
}

/*
 * rt2860_update_raw_counters
 */
static void rt2860_update_raw_counters(struct rt2860_softc *sc)
{
	uint32_t tmp;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_AGG_CNT);

	sc->tx_nonagg += tmp & 0xffff;
	sc->tx_agg += tmp >> 16;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_AGG_CNT0);

	sc->tx_ampdu += (tmp & 0xffff) / 1 + (tmp >> 16) / 2;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_AGG_CNT1);

	sc->tx_ampdu += (tmp & 0xffff) / 3 + (tmp >> 16) / 4;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_AGG_CNT2);

	sc->tx_ampdu += (tmp & 0xffff) / 5 + (tmp >> 16) / 6;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_AGG_CNT3);

	sc->tx_ampdu += (tmp & 0xffff) / 7 + (tmp >> 16) / 8;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_AGG_CNT4);

	sc->tx_ampdu += (tmp & 0xffff) / 9 + (tmp >> 16) / 10;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_AGG_CNT5);

	sc->tx_ampdu += (tmp & 0xffff) / 11 + (tmp >> 16) / 12;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_AGG_CNT6);

	sc->tx_ampdu += (tmp & 0xffff) / 13 + (tmp >> 16) / 14;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TX_AGG_CNT7);

	sc->tx_ampdu += (tmp & 0xffff) / 15 + (tmp >> 16) / 16;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_RX_STA_CNT0);

	sc->rx_crc_errors += tmp & 0xffff;
	sc->rx_phy_errors += tmp >> 16;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_RX_STA_CNT1);

	sc->rx_false_ccas += tmp & 0xffff;
	sc->rx_plcp_errors += tmp >> 16;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_RX_STA_CNT2);

	sc->rx_dup_packets += tmp & 0xffff;
	sc->rx_fifo_overflows += tmp >> 16;

	tmp = rt2860_io_mac_read(sc, RT2860_REG_TXRX_MPDU_DEN_CNT);

	sc->tx_mpdu_zero_density += tmp & 0xffff;
	sc->rx_mpdu_zero_density += tmp >> 16;
}

/*
 * rt2860_intr_enable
 */
static void rt2860_intr_enable(struct rt2860_softc *sc, uint32_t intr_mask)
{
	uint32_t tmp;

	sc->intr_disable_mask &= ~intr_mask;

	tmp = sc->intr_enable_mask & ~sc->intr_disable_mask;

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_INT_MASK, tmp);
}

/*
 * rt2860_intr_disable
 */
static void rt2860_intr_disable(struct rt2860_softc *sc, uint32_t intr_mask)
{
	uint32_t tmp;

	sc->intr_disable_mask |= intr_mask;

	tmp = sc->intr_enable_mask & ~sc->intr_disable_mask;

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_INT_MASK, tmp);
}

/*
 * rt2860_txrx_enable
 */
static int rt2860_txrx_enable(struct rt2860_softc *sc)
{
	struct ieee80211com *ic;
	struct ifnet *ifp;
	uint32_t tmp;
	int ntries;

	ifp = sc->ifp;
	ic = ifp->if_l2com;

	/* enable Tx/Rx DMA engine */

	rt2860_io_mac_write(sc, RT2860_REG_SYS_CTRL, RT2860_REG_TX_ENABLE);

	for (ntries = 0; ntries < 200; ntries++)
	{
		tmp = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG);
		if (!(tmp & (RT2860_REG_TX_DMA_BUSY | RT2860_REG_RX_DMA_BUSY)))
			break;

		DELAY(1000);
	}

	if (ntries == 200)
	{
		printf("%s: timeout waiting for DMA engine\n",
			device_get_nameunit(sc->dev));
		return -1;
	}

	DELAY(50);

	tmp |= RT2860_REG_TX_WB_DDONE |
		RT2860_REG_RX_DMA_ENABLE |
		RT2860_REG_TX_DMA_ENABLE |
		(RT2860_REG_WPDMA_BT_SIZE64 << RT2860_REG_WPDMA_BT_SIZE_SHIFT);

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_WPDMA_GLO_CFG, tmp);

	/* set Rx filter */

	tmp = RT2860_REG_RX_FILTER_DROP_CRC_ERR |
		RT2860_REG_RX_FILTER_DROP_PHY_ERR;

	if (ic->ic_opmode != IEEE80211_M_MONITOR)
	{
		tmp |= RT2860_REG_RX_FILTER_DROP_DUPL |
			RT2860_REG_RX_FILTER_DROP_CTS |
			RT2860_REG_RX_FILTER_DROP_BA |
			RT2860_REG_RX_FILTER_DROP_ACK |
			RT2860_REG_RX_FILTER_DROP_VER_ERR |
			RT2860_REG_RX_FILTER_DROP_CTRL_RSV |
			RT2860_REG_RX_FILTER_DROP_CFACK |
			RT2860_REG_RX_FILTER_DROP_CFEND;

		if (ic->ic_opmode == IEEE80211_M_STA)
			tmp |= RT2860_REG_RX_FILTER_DROP_RTS |
				RT2860_REG_RX_FILTER_DROP_PSPOLL;

		if (!(ifp->if_flags & IFF_PROMISC))
			tmp |= RT2860_REG_RX_FILTER_DROP_UC_NOME;
	}

	rt2860_io_mac_write(sc, RT2860_REG_RX_FILTER_CFG, tmp);

	rt2860_io_mac_write(sc, RT2860_REG_SYS_CTRL,
		RT2860_REG_RX_ENABLE | RT2860_REG_TX_ENABLE);

	return 0;
}

/*
 * rt2860_alloc_rx_ring
 */
static int rt2860_alloc_rx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_rx_ring *ring)
{
	struct rt2860_rxdesc *desc;
	struct rt2860_softc_rx_data *data;
	bus_dma_segment_t segs[1];
	int i, nsegs, error;

	error = bus_dma_tag_create(bus_get_dma_tag(sc->dev), PAGE_SIZE, 0, 
		BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
		RT2860_SOFTC_RX_RING_DATA_COUNT * sizeof(struct rt2860_rxdesc), 1,
		RT2860_SOFTC_RX_RING_DATA_COUNT * sizeof(struct rt2860_rxdesc),
		0, NULL, NULL, &ring->desc_dma_tag);
	if (error != 0)
	{
		printf("%s: could not create Rx desc DMA tag\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	error = bus_dmamem_alloc(ring->desc_dma_tag, (void **) &ring->desc,
	    BUS_DMA_NOWAIT | BUS_DMA_ZERO, &ring->desc_dma_map);
	if (error != 0)
	{
		printf("%s: could not allocate Rx desc DMA memory\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	error = bus_dmamap_load(ring->desc_dma_tag, ring->desc_dma_map,
		ring->desc,
		RT2860_SOFTC_RX_RING_DATA_COUNT * sizeof(struct rt2860_rxdesc),
		rt2860_dma_map_addr, &ring->desc_phys_addr, 0);
	if (error != 0)
	{
		printf("%s: could not load Rx desc DMA map\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	error = bus_dma_tag_create(bus_get_dma_tag(sc->dev), PAGE_SIZE, 0, 
	    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
		MJUMPAGESIZE, 1, MJUMPAGESIZE, 0, NULL, NULL,
		&ring->data_dma_tag);
	if (error != 0)
	{
		printf("%s: could not create Rx data DMA tag\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	for (i = 0; i < RT2860_SOFTC_RX_RING_DATA_COUNT; i++)
	{
		desc = &ring->desc[i];
		data = &ring->data[i];

		error = bus_dmamap_create(ring->data_dma_tag, 0, &data->dma_map);
		if (error != 0)
		{
			printf("%s: could not create Rx data DMA map\n",
				device_get_nameunit(sc->dev));
			goto fail;
		}

		data->m = m_getjcl(M_DONTWAIT, MT_DATA, M_PKTHDR, MJUMPAGESIZE);
		if (data->m == NULL)
		{
			printf("%s: could not allocate Rx mbuf\n",
				device_get_nameunit(sc->dev));
			error = ENOMEM;
			goto fail;
		}

		data->m->m_len = data->m->m_pkthdr.len = MJUMPAGESIZE;

		error = bus_dmamap_load_mbuf_sg(ring->data_dma_tag, data->dma_map,
			data->m, segs, &nsegs, BUS_DMA_NOWAIT);
		if (error != 0)
		{
			printf("%s: could not load Rx mbuf DMA map\n",
				device_get_nameunit(sc->dev));
			goto fail;
		}

		KASSERT(nsegs == 1, ("%s: too many DMA segments",
			device_get_nameunit(sc->dev)));

		desc->sdp0 = htole32(segs[0].ds_addr);
		desc->sdl0 = htole16(MJUMPAGESIZE);
	}

	error = bus_dmamap_create(ring->data_dma_tag, 0, &ring->spare_dma_map);
	if (error != 0)
	{
		printf("%s: could not create Rx spare DMA map\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
		BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return 0;

fail:
	
	rt2860_free_rx_ring(sc, ring);

	return error;
}

/*
 * rt2860_reset_rx_ring
 */
static void rt2860_reset_rx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_rx_ring *ring)
{
	struct rt2860_rxdesc *desc;
	int i;

	for (i = 0; i < RT2860_SOFTC_RX_RING_DATA_COUNT; i++)
	{
		desc = &ring->desc[i];

		desc->sdl0 &= ~htole16(RT2860_RXDESC_SDL0_DDONE);
	}

	bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
		BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	ring->cur = 0;
}

/*
 * rt2860_free_rx_ring
 */
static void rt2860_free_rx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_rx_ring *ring)
{
	struct rt2860_softc_rx_data *data;
	int i;

	if (ring->desc != NULL)
	{
		bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
			BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(ring->desc_dma_tag, ring->desc_dma_map);
		bus_dmamem_free(ring->desc_dma_tag, ring->desc,
			ring->desc_dma_map);
	}

	if (ring->desc_dma_tag != NULL)
		bus_dma_tag_destroy(ring->desc_dma_tag);

	for (i = 0; i < RT2860_SOFTC_RX_RING_DATA_COUNT; i++)
	{
		data = &ring->data[i];

		if (data->m != NULL)
		{
			bus_dmamap_sync(ring->data_dma_tag, data->dma_map,
				BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(ring->data_dma_tag, data->dma_map);
			m_freem(data->m);
		}

		if (data->dma_map != NULL)
			bus_dmamap_destroy(ring->data_dma_tag, data->dma_map);
	}

	if (ring->spare_dma_map != NULL)
		bus_dmamap_destroy(ring->data_dma_tag, ring->spare_dma_map);

	if (ring->data_dma_tag != NULL)
		bus_dma_tag_destroy(ring->data_dma_tag);
}

/*
 * rt2860_alloc_tx_ring
 */
static int rt2860_alloc_tx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_tx_ring *ring, int qid)
{
	struct rt2860_softc_tx_data *data;
	int error, i;

	mtx_init(&ring->lock, device_get_nameunit(sc->dev), NULL, MTX_DEF);

	error = bus_dma_tag_create(bus_get_dma_tag(sc->dev), PAGE_SIZE, 0, 
		BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
		RT2860_SOFTC_TX_RING_DESC_COUNT * sizeof(struct rt2860_txdesc), 1,
		RT2860_SOFTC_TX_RING_DESC_COUNT * sizeof(struct rt2860_txdesc),
		0, NULL, NULL, &ring->desc_dma_tag);
	if (error != 0)
	{
		printf("%s: could not create Tx desc DMA tag\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	error = bus_dmamem_alloc(ring->desc_dma_tag, (void **) &ring->desc,
	    BUS_DMA_NOWAIT | BUS_DMA_ZERO, &ring->desc_dma_map);
	if (error != 0)
	{
		printf("%s: could not allocate Tx desc DMA memory\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	error = bus_dmamap_load(ring->desc_dma_tag, ring->desc_dma_map,
		ring->desc,
		RT2860_SOFTC_TX_RING_DESC_COUNT * sizeof(struct rt2860_txdesc),
		rt2860_dma_map_addr, &ring->desc_phys_addr, 0);
	if (error != 0)
	{
		printf("%s: could not load Tx desc DMA map\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	ring->desc_queued = 0;
	ring->desc_cur = 0;
	ring->desc_next = 0;

	error = bus_dma_tag_create(bus_get_dma_tag(sc->dev), PAGE_SIZE, 0, 
		BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
		RT2860_SOFTC_TX_RING_DATA_COUNT * RT2860_TX_DATA_SEG0_SIZE, 1,
		RT2860_SOFTC_TX_RING_DATA_COUNT * RT2860_TX_DATA_SEG0_SIZE,
		0, NULL, NULL, &ring->seg0_dma_tag);
	if (error != 0)
	{
		printf("%s: could not create Tx seg0 DMA tag\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	error = bus_dmamem_alloc(ring->seg0_dma_tag, (void **) &ring->seg0,
	    BUS_DMA_NOWAIT | BUS_DMA_ZERO, &ring->seg0_dma_map);
	if (error != 0)
	{
		printf("%s: could not allocate Tx seg0 DMA memory\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	error = bus_dmamap_load(ring->seg0_dma_tag, ring->seg0_dma_map,
		ring->seg0,
		RT2860_SOFTC_TX_RING_DATA_COUNT * RT2860_TX_DATA_SEG0_SIZE,
		rt2860_dma_map_addr, &ring->seg0_phys_addr, 0);
	if (error != 0)
	{
		printf("%s: could not load Tx seg0 DMA map\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	error = bus_dma_tag_create(bus_get_dma_tag(sc->dev), PAGE_SIZE, 0, 
	    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
		MJUMPAGESIZE, RT2860_SOFTC_MAX_SCATTER, MJUMPAGESIZE, 0, NULL, NULL,
		&ring->data_dma_tag);
	if (error != 0)
	{
		printf("%s: could not create Tx data DMA tag\n",
			device_get_nameunit(sc->dev));
		goto fail;
	}

	for (i = 0; i < RT2860_SOFTC_TX_RING_DATA_COUNT; i++)
	{
		data = &ring->data[i];

		error = bus_dmamap_create(ring->data_dma_tag, 0, &data->dma_map);
		if (error != 0)
		{
			printf("%s: could not create Tx data DMA map\n",
				device_get_nameunit(sc->dev));
			goto fail;
		}
	}

	ring->data_queued = 0;
	ring->data_cur = 0;
	ring->data_next = 0;

	ring->qid = qid;

	return 0;

fail:
	
	rt2860_free_tx_ring(sc, ring);

	return error;
}

/*
 * rt2860_reset_tx_ring
 */
static void rt2860_reset_tx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_tx_ring *ring)
{
	struct rt2860_softc_tx_data *data;
	struct rt2860_txdesc *desc;
	int i;

	for (i = 0; i < RT2860_SOFTC_TX_RING_DESC_COUNT; i++)
	{
		desc = &ring->desc[i];

		desc->sdl0 = 0;
		desc->sdl1 = 0;
	}

	ring->desc_queued = 0;
	ring->desc_cur = 0;
	ring->desc_next = 0;

	bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
		BUS_DMASYNC_PREWRITE);

	bus_dmamap_sync(ring->seg0_dma_tag, ring->seg0_dma_map,
		BUS_DMASYNC_PREWRITE);

	for (i = 0; i < RT2860_SOFTC_TX_RING_DATA_COUNT; i++)
	{
		data = &ring->data[i];

		if (data->m != NULL)
		{
			bus_dmamap_sync(ring->data_dma_tag, data->dma_map,
				BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(ring->data_dma_tag, data->dma_map);
			m_freem(data->m);
			data->m = NULL;
		}

		if (data->ni != NULL)
		{
			ieee80211_free_node(data->ni);
			data->ni = NULL;
		}
	}

	ring->data_queued = 0;
	ring->data_cur = 0;
	ring->data_next = 0;
}

/*
 * rt2860_free_tx_ring
 */
static void rt2860_free_tx_ring(struct rt2860_softc *sc,
	struct rt2860_softc_tx_ring *ring)
{
	struct rt2860_softc_tx_data *data;
	int i;

	if (ring->desc != NULL)
	{
		bus_dmamap_sync(ring->desc_dma_tag, ring->desc_dma_map,
			BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(ring->desc_dma_tag, ring->desc_dma_map);
		bus_dmamem_free(ring->desc_dma_tag, ring->desc,
			ring->desc_dma_map);
	}

	if (ring->desc_dma_tag != NULL)
		bus_dma_tag_destroy(ring->desc_dma_tag);

	if (ring->seg0 != NULL)
	{
		bus_dmamap_sync(ring->seg0_dma_tag, ring->seg0_dma_map,
			BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(ring->seg0_dma_tag, ring->seg0_dma_map);
		bus_dmamem_free(ring->seg0_dma_tag, ring->seg0,
			ring->seg0_dma_map);
	}

	if (ring->seg0_dma_tag != NULL)
		bus_dma_tag_destroy(ring->seg0_dma_tag);

	for (i = 0; i < RT2860_SOFTC_TX_RING_DATA_COUNT; i++)
	{
		data = &ring->data[i];

		if (data->m != NULL)
		{
			bus_dmamap_sync(ring->data_dma_tag, data->dma_map,
				BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(ring->data_dma_tag, data->dma_map);
			m_freem(data->m);
		}

		if (data->ni != NULL)
			ieee80211_free_node(data->ni);

		if (data->dma_map != NULL)
			bus_dmamap_destroy(ring->data_dma_tag, data->dma_map);
	}

	if (ring->data_dma_tag != NULL)
		bus_dma_tag_destroy(ring->data_dma_tag);

	mtx_destroy(&ring->lock);
}

/*
 * rt2860_dma_map_addr
 */
static void rt2860_dma_map_addr(void *arg, bus_dma_segment_t *segs,
	int nseg, int error)
{
	if (error != 0)
		return;

	KASSERT(nseg == 1, ("too many DMA segments, %d should be 1", nseg));

	*(bus_addr_t *) arg = segs[0].ds_addr;
}

/*
 * rt2860_sysctl_attach
 */
static void rt2860_sysctl_attach(struct rt2860_softc *sc)
{
	struct sysctl_ctx_list *ctx;
	struct sysctl_oid *tree;
	struct sysctl_oid *stats;

	ctx = device_get_sysctl_ctx(sc->dev);
	tree = device_get_sysctl_tree(sc->dev);

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(tree), OID_AUTO,
		"tx_stbc", CTLFLAG_RW, &sc->tx_stbc, 0,
		"Tx STBC");

	/* statistic counters */

	stats = SYSCTL_ADD_NODE(ctx, SYSCTL_CHILDREN(tree), OID_AUTO,
		"stats", CTLFLAG_RD, 0, "statistic");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"interrupts", CTLFLAG_RD, &sc->interrupts, 0,
		"all interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_coherent_interrupts", CTLFLAG_RD, &sc->tx_coherent_interrupts, 0,
		"Tx coherent interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_coherent_interrupts", CTLFLAG_RD, &sc->rx_coherent_interrupts, 0,
		"Rx coherent interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"txrx_coherent_interrupts", CTLFLAG_RD, &sc->txrx_coherent_interrupts, 0,
		"Tx/Rx coherent interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"fifo_sta_full_interrupts", CTLFLAG_RD, &sc->fifo_sta_full_interrupts, 0,
		"FIFO statistic full interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_interrupts", CTLFLAG_RD, &sc->rx_interrupts, 0,
		"Rx interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_delay_interrupts", CTLFLAG_RD, &sc->rx_delay_interrupts, 0,
		"Rx delay interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_mgmt_interrupts", CTLFLAG_RD, &sc->tx_interrupts[5], 0,
		"Tx MGMT interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_hcca_interrupts", CTLFLAG_RD, &sc->tx_interrupts[4], 0,
		"Tx HCCA interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac3_interrupts", CTLFLAG_RD, &sc->tx_interrupts[3], 0,
		"Tx AC3 interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac2_interrupts", CTLFLAG_RD, &sc->tx_interrupts[2], 0,
		"Tx AC2 interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac1_interrupts", CTLFLAG_RD, &sc->tx_interrupts[1], 0,
		"Tx AC1 interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac0_interrupts", CTLFLAG_RD, &sc->tx_interrupts[0], 0,
		"Tx AC0 interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_delay_interrupts", CTLFLAG_RD, &sc->tx_delay_interrupts, 0,
		"Tx delay interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"pre_tbtt_interrupts", CTLFLAG_RD, &sc->pre_tbtt_interrupts, 0,
		"Pre-TBTT interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tbtt_interrupts", CTLFLAG_RD, &sc->tbtt_interrupts, 0,
		"TBTT interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"mcu_cmd_interrupts", CTLFLAG_RD, &sc->mcu_cmd_interrupts, 0,
		"MCU command interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"auto_wakeup_interrupts", CTLFLAG_RD, &sc->auto_wakeup_interrupts, 0,
		"auto wakeup interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"gp_timer_interrupts", CTLFLAG_RD, &sc->gp_timer_interrupts, 0,
		"GP timer interrupts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_mgmt_desc_queued", CTLFLAG_RD, &sc->tx_ring[5].desc_queued, 0,
		"Tx MGMT descriptors queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_mgmt_data_queued", CTLFLAG_RD, &sc->tx_ring[5].data_queued, 0,
		"Tx MGMT data queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_hcca_desc_queued", CTLFLAG_RD, &sc->tx_ring[4].desc_queued, 0,
		"Tx HCCA descriptors queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_hcca_data_queued", CTLFLAG_RD, &sc->tx_ring[4].data_queued, 0,
		"Tx HCCA data queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac3_desc_queued", CTLFLAG_RD, &sc->tx_ring[3].desc_queued, 0,
		"Tx AC3 descriptors queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac3_data_queued", CTLFLAG_RD, &sc->tx_ring[3].data_queued, 0,
		"Tx AC3 data queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac2_desc_queued", CTLFLAG_RD, &sc->tx_ring[2].desc_queued, 0,
		"Tx AC2 descriptors queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac2_data_queued", CTLFLAG_RD, &sc->tx_ring[2].data_queued, 0,
		"Tx AC2 data queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac1_desc_queued", CTLFLAG_RD, &sc->tx_ring[1].desc_queued, 0,
		"Tx AC1 descriptors queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac1_data_queued", CTLFLAG_RD, &sc->tx_ring[1].data_queued, 0,
		"Tx AC1 data queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac0_desc_queued", CTLFLAG_RD, &sc->tx_ring[0].desc_queued, 0,
		"Tx AC0 descriptors queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac0_data_queued", CTLFLAG_RD, &sc->tx_ring[0].data_queued, 0,
		"Tx AC0 data queued");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_mgmt_data_queue_full", CTLFLAG_RD, &sc->tx_data_queue_full[5], 0,
		"Tx MGMT data queue full");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_hcca_data_queue_full", CTLFLAG_RD, &sc->tx_data_queue_full[4], 0,
		"Tx HCCA data queue full");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac3_data_queue_full", CTLFLAG_RD, &sc->tx_data_queue_full[3], 0,
		"Tx AC3 data queue full");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac2_data_queue_full", CTLFLAG_RD, &sc->tx_data_queue_full[2], 0,
		"Tx AC2 data queue full");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac1_data_queue_full", CTLFLAG_RD, &sc->tx_data_queue_full[1], 0,
		"Tx AC1 data queue full");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ac0_data_queue_full", CTLFLAG_RD, &sc->tx_data_queue_full[0], 0,
		"Tx AC0 data queue full");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_watchdog_timeouts", CTLFLAG_RD, &sc->tx_watchdog_timeouts, 0,
		"Tx watchdog timeouts");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_defrag_packets", CTLFLAG_RD, &sc->tx_defrag_packets, 0,
		"Tx defragmented packets");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"no_tx_desc_avail", CTLFLAG_RD, &sc->no_tx_desc_avail, 0,
		"no Tx descriptors available");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_mbuf_alloc_errors", CTLFLAG_RD, &sc->rx_mbuf_alloc_errors, 0,
		"Rx mbuf allocation errors");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_mbuf_dmamap_errors", CTLFLAG_RD, &sc->rx_mbuf_dmamap_errors, 0,
		"Rx mbuf DMA mapping errors");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_queue_0_not_empty", CTLFLAG_RD, &sc->tx_queue_not_empty[0], 0,
		"Tx queue 0 not empty");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_queue_1_not_empty", CTLFLAG_RD, &sc->tx_queue_not_empty[1], 0,
		"Tx queue 1 not empty");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_beacons", CTLFLAG_RD, &sc->tx_beacons, 0,
		"Tx beacons");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_noretryok", CTLFLAG_RD, &sc->tx_noretryok, 0,
		"Tx successfull without retries");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_retryok", CTLFLAG_RD, &sc->tx_retryok, 0,
		"Tx successfull with retries");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_failed", CTLFLAG_RD, &sc->tx_failed, 0,
		"Tx failed");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_underflows", CTLFLAG_RD, &sc->tx_underflows, 0,
		"Tx underflows");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_zerolen", CTLFLAG_RD, &sc->tx_zerolen, 0,
		"Tx zero length");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_nonagg", CTLFLAG_RD, &sc->tx_nonagg, 0,
		"Tx non-aggregated");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_agg", CTLFLAG_RD, &sc->tx_agg, 0,
		"Tx aggregated");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ampdu", CTLFLAG_RD, &sc->tx_ampdu, 0,
		"Tx A-MPDU");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_mpdu_zero_density", CTLFLAG_RD, &sc->tx_mpdu_zero_density, 0,
		"Tx MPDU with zero density");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"tx_ampdu_sessions", CTLFLAG_RD, &sc->tx_ampdu_sessions, 0,
		"Tx A-MPDU sessions");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_packets", CTLFLAG_RD, &sc->rx_packets, 0,
		"Rx packets");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_ampdu", CTLFLAG_RD, &sc->rx_ampdu, 0,
		"Rx A-MPDU");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_ampdu_retries", CTLFLAG_RD, &sc->rx_ampdu_retries, 0,
		"Rx A-MPDU retries");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_mpdu_zero_density", CTLFLAG_RD, &sc->rx_mpdu_zero_density, 0,
		"Rx MPDU with zero density");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_ampdu_sessions", CTLFLAG_RD, &sc->rx_ampdu_sessions, 0,
		"Rx A-MPDU sessions");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_amsdu", CTLFLAG_RD, &sc->rx_amsdu, 0,
		"Rx A-MSDU");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_crc_errors", CTLFLAG_RD, &sc->rx_crc_errors, 0,
		"Rx CRC errors");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_phy_errors", CTLFLAG_RD, &sc->rx_phy_errors, 0,
		"Rx PHY errors");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_false_ccas", CTLFLAG_RD, &sc->rx_false_ccas, 0,
		"Rx false CCAs");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_plcp_errors", CTLFLAG_RD, &sc->rx_plcp_errors, 0,
		"Rx PLCP errors");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_dup_packets", CTLFLAG_RD, &sc->rx_dup_packets, 0,
		"Rx duplicate packets");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_fifo_overflows", CTLFLAG_RD, &sc->rx_fifo_overflows, 0,
		"Rx FIFO overflows");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_cipher_no_errors", CTLFLAG_RD, &sc->rx_cipher_no_errors, 0,
		"Rx cipher no errors");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_cipher_icv_errors", CTLFLAG_RD, &sc->rx_cipher_icv_errors, 0,
		"Rx cipher ICV errors");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_cipher_mic_errors", CTLFLAG_RD, &sc->rx_cipher_mic_errors, 0,
		"Rx cipher MIC errors");

	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(stats), OID_AUTO,
		"rx_cipher_invalid_key_errors", CTLFLAG_RD, &sc->rx_cipher_invalid_key_errors, 0,
		"Rx cipher invalid key errors");
}


