#include <PCI.h>
#include <Drivers.h>
#include <malloc.h>
#include <ether_driver.h>

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include <net/if_media.h>
#endif

#include "mm.h"
#include "lm.h"
#include "mempool.h"

struct pci_module_info *pci = NULL;

const char *dev_list[11];
struct be_b57_dev be_b57_dev_cards[10];
int cards_found = 0;

int b57_Packet_Desc_Size = sizeof(struct B_UM_PACKET);

#define ROUND_UP_TO_PAGE(size) ((size % 4096 != 0) ? 4096 - (size % 4096) + size : size)

struct pci_device_id {
	unsigned int vendor, device;		/* Vendor and device ID or PCI_ANY_ID */
	unsigned int subvendor, subdevice;	/* Subsystem ID's or PCI_ANY_ID */
	unsigned int class, class_mask;		/* (class,subclass,prog-if) triplet */
	unsigned long driver_data;		/* Data private to the driver */
};

#define PCI_ANY_ID		0

typedef enum {
	BCM5700A6 = 0,
	BCM5700T6,
	BCM5700A9,
	BCM5700T9,
	BCM5700,
	BCM5701A5,
	BCM5701T1,
	BCM5701T8,
	BCM5701A7,
	BCM5701A10,
	BCM5701A12,
	BCM5701,
	BCM5702,
	BCM5703,
	BCM5703A31,
	BCM5703ARBUCKLE,
	TC996T,
	TC996ST,
	TC996SSX,
	TC996SX,
	TC996BT,
	TC997T,
	TC997SX,
	TC1000T,
	TC1000BT,
	TC940BR01,
	TC942BR01,
	TC998T,
	TC998SX,
	TC999T,
	NC6770,
	NC1020,
	NC150T,
	NC7760,
	NC7761,
	NC7770,
	NC7771,
	NC7780,
	NC7781,
	NC7772,
	NC7782,
	NC7783,
	NC320T,
	BCM5704CIOBE,
	BCM5704,
	BCM5704S,
	BCM5705,
	BCM5705M,
	BCM5705F,
	BCM5901,
	BCM5782,
	BCM5788,
	BCM5789,
	BCM5750,
	BCM5750M,
	BCM5720,
	BCM5751,
	BCM5751M,
	BCM5751F,
	BCM5721,
} board_t;


/* indexed by board_t, above */
static struct {
	char *name;
} board_info[] = {
	{ "Broadcom BCM5700 1000Base-T" },
	{ "Broadcom BCM5700 1000Base-SX" },
	{ "Broadcom BCM5700 1000Base-SX" },
	{ "Broadcom BCM5700 1000Base-T" },
	{ "Broadcom BCM5700" },
	{ "Broadcom BCM5701 1000Base-T" },
	{ "Broadcom BCM5701 1000Base-T" },
	{ "Broadcom BCM5701 1000Base-T" },
	{ "Broadcom BCM5701 1000Base-SX" },
	{ "Broadcom BCM5701 1000Base-T" },
	{ "Broadcom BCM5701 1000Base-T" },
	{ "Broadcom BCM5701" },
	{ "Broadcom BCM5702 1000Base-T" },
	{ "Broadcom BCM5703 1000Base-T" },
	{ "Broadcom BCM5703 1000Base-SX" },
	{ "Broadcom B5703 1000Base-SX" },
	{ "3Com 3C996 10/100/1000 Server NIC" },
	{ "3Com 3C996 10/100/1000 Server NIC" },
	{ "3Com 3C996 Gigabit Fiber-SX Server NIC" },
	{ "3Com 3C996 Gigabit Fiber-SX Server NIC" },
	{ "3Com 3C996B Gigabit Server NIC" },
	{ "3Com 3C997 Gigabit Server NIC" },
	{ "3Com 3C997 Gigabit Fiber-SX Server NIC" },
	{ "3Com 3C1000 Gigabit NIC" },
	{ "3Com 3C1000B-T 10/100/1000 PCI" },
	{ "3Com 3C940 Gigabit LOM (21X21)" },
	{ "3Com 3C942 Gigabit LOM (31X31)" },
	{ "3Com 3C998-T Dual Port 10/100/1000 PCI-X Server NIC" },
	{ "3Com 3C998-SX Dual Port 1000-SX PCI-X Server NIC" },
	{ "3Com 3C999-T Quad Port 10/100/1000 PCI-X Server NIC" },
	{ "HP NC6770 Gigabit Server Adapter" },
	{ "NC1020 HP ProLiant Gigabit Server Adapter 32 PCI" },
	{ "HP ProLiant NC 150T PCI 4-port Gigabit Combo Switch Adapter" },
	{ "HP NC7760 Gigabit Server Adapter" },
	{ "HP NC7761 Gigabit Server Adapter" },
	{ "HP NC7770 Gigabit Server Adapter" },
	{ "HP NC7771 Gigabit Server Adapter" },
	{ "HP NC7780 Gigabit Server Adapter" },
	{ "HP NC7781 Gigabit Server Adapter" },
	{ "HP NC7772 Gigabit Server Adapter" },
	{ "HP NC7782 Gigabit Server Adapter" },
	{ "HP NC7783 Gigabit Server Adapter" },
	{ "HP ProLiant NC 320T PCI Express Gigabit Server Adapter" },
	{ "Broadcom BCM5704 CIOB-E 1000Base-T" },
	{ "Broadcom BCM5704 1000Base-T" },
	{ "Broadcom BCM5704 1000Base-SX" },
	{ "Broadcom BCM5705 1000Base-T" },
	{ "Broadcom BCM5705M 1000Base-T" },
	{ "Broadcom 570x 10/100 Integrated Controller" },
	{ "Broadcom BCM5901 100Base-TX" },
	{ "Broadcom NetXtreme Gigabit Ethernet for hp" },
	{ "Broadcom BCM5788 NetLink 1000Base-T" },
	{ "Broadcom BCM5789 NetLink 1000Base-T PCI Express" },
	{ "Broadcom BCM5750 1000Base-T PCI" },
	{ "Broadcom BCM5750M 1000Base-T PCI" },
	{ "Broadcom BCM5720 1000Base-T PCI" },
	{ "Broadcom BCM5751 1000Base-T PCI Express" },
	{ "Broadcom BCM5751M 1000Base-T PCI Express" },
	{ "Broadcom BCM5751F 100Base-TX PCI Express" },
	{ "Broadcom BCM5721 1000Base-T PCI Express" },
	{ 0 },
	};

static struct pci_device_id bcm5700_pci_tbl[] = {
	{0x14e4, 0x1644, 0x14e4, 0x1644, 0, 0, BCM5700A6 },
	{0x14e4, 0x1644, 0x14e4, 0x2, 0, 0, BCM5700T6 },
	{0x14e4, 0x1644, 0x14e4, 0x3, 0, 0, BCM5700A9 },
	{0x14e4, 0x1644, 0x14e4, 0x4, 0, 0, BCM5700T9 },
	{0x14e4, 0x1644, 0x1028, 0xd1, 0, 0, BCM5700 },
	{0x14e4, 0x1644, 0x1028, 0x0106, 0, 0, BCM5700 },
	{0x14e4, 0x1644, 0x1028, 0x0109, 0, 0, BCM5700 },
	{0x14e4, 0x1644, 0x1028, 0x010a, 0, 0, BCM5700 },
	{0x14e4, 0x1644, 0x10b7, 0x1000, 0, 0, TC996T },
	{0x14e4, 0x1644, 0x10b7, 0x1001, 0, 0, TC996ST },
	{0x14e4, 0x1644, 0x10b7, 0x1002, 0, 0, TC996SSX },
	{0x14e4, 0x1644, 0x10b7, 0x1003, 0, 0, TC997T },
	{0x14e4, 0x1644, 0x10b7, 0x1005, 0, 0, TC997SX },
	{0x14e4, 0x1644, 0x10b7, 0x1008, 0, 0, TC942BR01 },
	{0x14e4, 0x1644, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5700 },
	{0x14e4, 0x1645, 0x14e4, 1, 0, 0, BCM5701A5 },
	{0x14e4, 0x1645, 0x14e4, 5, 0, 0, BCM5701T1 },
	{0x14e4, 0x1645, 0x14e4, 6, 0, 0, BCM5701T8 },
	{0x14e4, 0x1645, 0x14e4, 7, 0, 0, BCM5701A7 },
	{0x14e4, 0x1645, 0x14e4, 8, 0, 0, BCM5701A10 },
	{0x14e4, 0x1645, 0x14e4, 0x8008, 0, 0, BCM5701A12 },
	{0x14e4, 0x1645, 0x0e11, 0xc1, 0, 0, NC6770 },
	{0x14e4, 0x1645, 0x0e11, 0x7c, 0, 0, NC7770 },
	{0x14e4, 0x1645, 0x0e11, 0x85, 0, 0, NC7780 },
	{0x14e4, 0x1645, 0x1028, 0x0121, 0, 0, BCM5701 },
	{0x14e4, 0x1645, 0x10b7, 0x1004, 0, 0, TC996SX },
	{0x14e4, 0x1645, 0x10b7, 0x1006, 0, 0, TC996BT },
	{0x14e4, 0x1645, 0x10b7, 0x1007, 0, 0, TC1000T },
	{0x14e4, 0x1645, 0x10b7, 0x1008, 0, 0, TC940BR01 },
	{0x14e4, 0x1645, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5701 },
	{0x14e4, 0x1646, 0x14e4, 0x8009, 0, 0, BCM5702 },
	{0x14e4, 0x1646, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5702 },
	{0x14e4, 0x16a6, 0x14e4, 0x8009, 0, 0, BCM5702 },
	{0x14e4, 0x16a6, 0x14e4, 0x000c, 0, 0, BCM5702 },
	{0x14e4, 0x16a6, 0x0e11, 0xbb, 0, 0, NC7760 },
	{0x14e4, 0x16a6, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5702 },
	{0x14e4, 0x16c6, 0x10b7, 0x1100, 0, 0, TC1000BT },
	{0x14e4, 0x16c6, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5702 },
	{0x14e4, 0x1647, 0x14e4, 0x0009, 0, 0, BCM5703 },
	{0x14e4, 0x1647, 0x14e4, 0x000a, 0, 0, BCM5703A31 },
	{0x14e4, 0x1647, 0x14e4, 0x000b, 0, 0, BCM5703 },
	{0x14e4, 0x1647, 0x14e4, 0x800a, 0, 0, BCM5703 },
	{0x14e4, 0x1647, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5703 },
	{0x14e4, 0x16a7, 0x14e4, 0x0009, 0, 0, BCM5703 },
	{0x14e4, 0x16a7, 0x14e4, 0x000a, 0, 0, BCM5703A31 },
	{0x14e4, 0x16a7, 0x14e4, 0x000b, 0, 0, BCM5703 },
	{0x14e4, 0x16a7, 0x14e4, 0x800a, 0, 0, BCM5703 },
	{0x14e4, 0x16a7, 0x0e11, 0xca, 0, 0, NC7771 },
	{0x14e4, 0x16a7, 0x0e11, 0xcb, 0, 0, NC7781 },
	{0x14e4, 0x16a7, 0x1014, 0x0281, 0, 0, BCM5703ARBUCKLE },
	{0x14e4, 0x16a7, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5703 },
	{0x14e4, 0x16c7, 0x14e4, 0x000a, 0, 0, BCM5703A31 },
	{0x14e4, 0x16c7, 0x0e11, 0xca, 0, 0, NC7771 },
	{0x14e4, 0x16c7, 0x0e11, 0xcb, 0, 0, NC7781 },
	{0x14e4, 0x16c7, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5703 },
	{0x14e4, 0x1648, 0x0e11, 0xcf, 0, 0, NC7772 },
	{0x14e4, 0x1648, 0x0e11, 0xd0, 0, 0, NC7782 },
	{0x14e4, 0x1648, 0x0e11, 0xd1, 0, 0, NC7783 },
	{0x14e4, 0x1648, 0x10b7, 0x2000, 0, 0, TC998T },
	{0x14e4, 0x1648, 0x10b7, 0x3000, 0, 0, TC999T },
	{0x14e4, 0x1648, 0x1166, 0x1648, 0, 0, BCM5704CIOBE },
	{0x14e4, 0x1648, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5704 },
	{0x14e4, 0x1649, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5704S },
	{0x14e4, 0x16a8, 0x14e4, 0x16a8, 0, 0, BCM5704S },
	{0x14e4, 0x16a8, 0x10b7, 0x2001, 0, 0, TC998SX },
	{0x14e4, 0x16a8, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5704S },
	{0x14e4, 0x1653, 0x0e11, 0x00e3, 0, 0, NC7761 },
	{0x14e4, 0x1653, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5705 },
	{0x14e4, 0x1654, 0x0e11, 0x00e3, 0, 0, NC7761 },
	{0x14e4, 0x1654, 0x103c, 0x3100, 0, 0, NC1020 },
	{0x14e4, 0x1654, 0x103c, 0x3226, 0, 0, NC150T },
	{0x14e4, 0x1654, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5705 },
	{0x14e4, 0x165d, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5705M },
	{0x14e4, 0x165e, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5705M },
	{0x14e4, 0x166e, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5705F },
	{0x14e4, 0x1696, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5782 },
	{0x14e4, 0x169c, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5788 },
	{0x14e4, 0x169d, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5789 },
	{0x14e4, 0x170d, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5901 },
	{0x14e4, 0x170e, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5901 },
	{0x14e4, 0x1676, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5750 },
	{0x14e4, 0x167c, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5750M },
	{0x14e4, 0x1677, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5751 },
	{0x14e4, 0x167d, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5751M },
	{0x14e4, 0x167e, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5751F },
	{0x14e4, 0x1658, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5720 },
	{0x14e4, 0x1659, 0x103c, 0x7031, 0, 0, NC320T },
	{0x14e4, 0x1659, PCI_ANY_ID, PCI_ANY_ID, 0, 0, BCM5721 },
	{0,}
};

/* -------- BeOS Driver Hooks ------------ */

static status_t b57_open(const char *name, uint32 flags, void **cookie);
static status_t b57_close(void *cookie);
static status_t b57_free(void *cookie);
static status_t b57_ioctl(void *cookie,uint32 op,void *data,size_t len);
static status_t b57_read(void *cookie,off_t pos,void *data,size_t *numBytes);
static status_t b57_write(void *cookie,off_t pos,const void *data,size_t *numBytes);
static int32 b57_interrupt(void *cookie);
static int32 tx_cleanup_thread(void *us);

device_hooks b57_hooks = {b57_open,b57_close,b57_free,b57_ioctl,b57_read,b57_write,NULL,NULL,NULL,NULL};
int32 api_version = B_CUR_DRIVER_API_VERSION;

//int debug_fd;


status_t
init_hardware(void)
{
	return B_OK;
}


const char **
publish_devices(void)
{
	return dev_list;
}


device_hooks *
find_device(const char *name)
{
	return &b57_hooks;
}


status_t
init_driver(void)
{
	int i = 0, j = 0, is_detected;
	pci_info dev_info;

	//debug_fd = open("/tmp/broadcom_traffic_log",O_RDWR | B_CREATE_FILE);

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci) != B_OK)
		return B_ERROR;

	while (pci->get_nth_pci_info(i++, &dev_info) == 0) {
		is_detected = 0;
		if ((dev_info.class_base == PCI_network) && (dev_info.class_sub == PCI_ethernet)) {
			for (j = 0; bcm5700_pci_tbl[j].vendor != 0; j++) {
				if ((dev_info.vendor_id == bcm5700_pci_tbl[j].vendor) && (dev_info.device_id == bcm5700_pci_tbl[j].device)) {
					is_detected = 1;
					break;
				}
			}
		}

		if (!is_detected)
			continue;

		if (cards_found >= 10)
			break;

		dev_list[cards_found] = (char *)malloc(16 /* net/bcm570x/xx */);
		sprintf(dev_list[cards_found],"net/bcm570x/%d",cards_found);
		be_b57_dev_cards[cards_found].pci_data = dev_info;
		be_b57_dev_cards[cards_found].packet_release_sem = create_sem(0,dev_list[cards_found]);
		be_b57_dev_cards[cards_found].mem_list_num = 0;
		be_b57_dev_cards[cards_found].lockmem_list_num = 0;
		be_b57_dev_cards[cards_found].opened = 0;
		be_b57_dev_cards[cards_found].block = 1;
		be_b57_dev_cards[cards_found].lock = 0;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		be_b57_dev_cards[cards_found].linkChangeSem = -1;
#endif

		if (LM_GetAdapterInfo(&be_b57_dev_cards[cards_found].lm_dev) != LM_STATUS_SUCCESS) {
			for (j = 0; j < cards_found; j++) {
				free(dev_list[j]);
				delete_sem(be_b57_dev_cards[j].packet_release_sem);
			}
			put_module(B_PCI_MODULE_NAME);
			return ENODEV;
		}

		QQ_InitQueue(&be_b57_dev_cards[cards_found].RxPacketReadQ.Container,MAX_RX_PACKET_DESC_COUNT);

		cards_found++;
	}

 	mempool_init((MAX_RX_PACKET_DESC_COUNT+MAX_TX_PACKET_DESC_COUNT) * cards_found);
 	dev_list[cards_found] = NULL;

	return B_OK;
}


void
uninit_driver(void)
{
	struct be_b57_dev *pUmDevice;
	int i, j;

	for (j = 0; j < cards_found; j++) {
		pUmDevice = &be_b57_dev_cards[j];

		for (i = 0; i < pUmDevice->mem_list_num; i++)
			free(pUmDevice->mem_list[i]);
		for (i = 0; i < pUmDevice->lockmem_list_num; i++)
			delete_area(pUmDevice->lockmem_list[i]);

		delete_area(pUmDevice->mem_base);
		delete_sem(be_b57_dev_cards[j].packet_release_sem);
		free((void *)dev_list[j]);
	}

	mempool_exit();
	put_module(B_PCI_MODULE_NAME);
}


//	#pragma mark -


static status_t
b57_open(const char *name, uint32 flags, void **cookie)
{
	struct be_b57_dev *pDevice = NULL;
	int i;

	*cookie = NULL;
	for (i = 0; i < cards_found; i++) {
		if (strcmp(dev_list[i],name) == 0) {
			*cookie = pDevice = &be_b57_dev_cards[i];
			break;
		}
	}

	if (*cookie == NULL)
		return B_FILE_NOT_FOUND;

	if (atomic_or(&pDevice->opened, 1)) {
		*cookie = pDevice = NULL;
		return B_BUSY;
	}

	install_io_interrupt_handler(pDevice->pci_data.u.h0.interrupt_line,
		b57_interrupt, *cookie, 0);
	if (LM_InitializeAdapter(&pDevice->lm_dev) != LM_STATUS_SUCCESS) {
		atomic_and(&pDevice->opened,0);
		remove_io_interrupt_handler(pDevice->pci_data.u.h0.interrupt_line, b57_interrupt, *cookie);
		*cookie = NULL;
		return B_ERROR;
	}

	/*QQ_InitQueue(&pDevice->rx_out_of_buf_q.Container,
        MAX_RX_PACKET_DESC_COUNT);*/

	//pDevice->lm_dev.PhyCrcCount = 0;
	LM_EnableInterrupt(&pDevice->lm_dev);

	dprintf("Broadcom 57xx adapter successfully inited at %s:\n", name);
	dprintf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
		pDevice->lm_dev.NodeAddress[0], pDevice->lm_dev.NodeAddress[1],
		pDevice->lm_dev.NodeAddress[2], pDevice->lm_dev.NodeAddress[3],
		pDevice->lm_dev.NodeAddress[4], pDevice->lm_dev.NodeAddress[5]);
	dprintf("PCI Data: 0x%08x\n", pDevice->pci_data.u.h0.base_registers[0]);
	dprintf("IRQ: %d\n", pDevice->pci_data.u.h0.interrupt_line);

	return B_OK;
}


static status_t
b57_close(void *cookie)
{
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *)(cookie);

	if (cookie == NULL)
		return B_OK;

	LM_DisableInterrupt(&pUmDevice->lm_dev);
	LM_Halt(&pUmDevice->lm_dev);
	pUmDevice->lm_dev.InitDone = 0;
	atomic_and(&pUmDevice->opened, 0);

	return B_OK;
}


static status_t
b57_free(void *cookie)
{
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *)(cookie);

	if (cookie == NULL)
		return B_OK;

	remove_io_interrupt_handler(pUmDevice->pci_data.u.h1.interrupt_line, b57_interrupt, cookie);
	return B_OK;
}


static status_t
b57_ioctl(void *cookie,uint32 op,void *data,size_t len)
{
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *)(cookie);

	switch (op) {
		case ETHER_INIT:
			return B_OK;
		case ETHER_GETADDR:
			if (data == NULL)
				return B_ERROR;

			memcpy(data, pUmDevice->lm_dev.NodeAddress, 6);
			return B_OK;
		case ETHER_NONBLOCK:
			pUmDevice->block = !*((uint8 *)(data));
			return B_OK;
		case ETHER_ADDMULTI:
			return (LM_MulticastAdd(&pUmDevice->lm_dev,
				(PLM_UINT8)(data)) == LM_STATUS_SUCCESS) ? B_OK : B_ERROR;
		case ETHER_REMMULTI:
			return (LM_MulticastDel(&pUmDevice->lm_dev,
				(PLM_UINT8)(data)) == LM_STATUS_SUCCESS) ? B_OK : B_ERROR;
		case ETHER_SETPROMISC:
			if (*((uint8 *)(data))) {
				LM_SetReceiveMask(&pUmDevice->lm_dev,
					pUmDevice->lm_dev.ReceiveMask | LM_PROMISCUOUS_MODE);
			} else {
				 LM_SetReceiveMask(&pUmDevice->lm_dev,
					pUmDevice->lm_dev.ReceiveMask & ~LM_PROMISCUOUS_MODE);
			}
			return B_OK;
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
		case ETHER_GETLINKSTATE:
		{
			ether_link_state_t *state_buffer = (ether_link_state_t *)(data);
			state_buffer->link_speed = pUmDevice->lm_dev.LineSpeed;
			state_buffer->link_quality = (pUmDevice->lm_dev.LinkStatus == LM_STATUS_LINK_DOWN) ? 0.0 : 1.0;
			state_buffer->duplex_mode = (pUmDevice->lm_dev.DuplexMode == LM_DUPLEX_MODE_FULL);
			return B_OK;
		}
#else
		case ETHER_GET_LINK_STATE:
		{
			ether_link_state_t state;
			state.media = (pUmDevice->lm_dev.LinkStatus
				== LM_STATUS_LINK_DOWN ? 0 : IFM_ACTIVE) | IFM_ETHER;
			switch (pUmDevice->lm_dev.LineSpeed) {
				case LM_LINE_SPEED_10MBPS:
					state.media |= IFM_10_T;
					state.speed = 10000000;
					break;
				case LM_LINE_SPEED_100MBPS:
					state.media |= IFM_100_TX;
					state.speed = 100000000;
					break;
				case LM_LINE_SPEED_1000MBPS:
					state.media |= IFM_1000_T;
					state.speed = 1000000000;
					break;
				default:
					state.speed = 0;
			}
			state.media |= (pUmDevice->lm_dev.DuplexMode
				== LM_DUPLEX_MODE_FULL ? IFM_FULL_DUPLEX : IFM_HALF_DUPLEX);
			state.quality = 1000;

			return user_memcpy(data, &state, sizeof(ether_link_state_t));
		}
		case ETHER_SET_LINK_STATE_SEM:
		{
			if (user_memcpy(&pUmDevice->linkChangeSem, data, sizeof(sem_id)) < B_OK) {
				pUmDevice->linkChangeSem = -1;
				return B_BAD_ADDRESS;
			}
			return B_OK;
		}

#endif
	}
	return B_ERROR;
}


static int32
b57_interrupt(void *cookie)
{
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *)cookie;
	PLM_DEVICE_BLOCK pDevice = (PLM_DEVICE_BLOCK) pUmDevice;
	unsigned int handled = 1;
	int i, max_intr_loop;
	LM_UINT32 oldtag, newtag;

	if (!pDevice->InitDone)
		return B_UNHANDLED_INTERRUPT;

	if (pDevice->pStatusBlkVirt->Status & STATUS_BLOCK_UPDATED) {
		max_intr_loop = 50;
		if (pDevice->Flags & USE_TAGGED_STATUS_FLAG) {
			MB_REG_WR(pDevice, Mailbox.Interrupt[0].Low, 1);
			oldtag = pDevice->pStatusBlkVirt->StatusTag;

			for (i = 0; ; i++) {
   				pDevice->pStatusBlkVirt->Status &=
					~STATUS_BLOCK_UPDATED;

				LM_ServiceInterrupts(pDevice);
				newtag = pDevice->pStatusBlkVirt->StatusTag;
				if ((newtag == oldtag) || (i > max_intr_loop)) {
					MB_REG_WR(pDevice,
						Mailbox.Interrupt[0].Low,
						oldtag << 24);
					pDevice->LastTag = oldtag;
					if (pDevice->Flags & UNDI_FIX_FLAG) {
						REG_WR(pDevice, Grc.LocalCtrl,
						pDevice->GrcLocalCtrl | 0x2);
					}
					break;
				}
				oldtag = newtag;
			}
		} else {
			i = 0;
			do {
				uint dummy;

				MB_REG_WR(pDevice, Mailbox.Interrupt[0].Low, 1);
   				pDevice->pStatusBlkVirt->Status &=
					~STATUS_BLOCK_UPDATED;
				LM_ServiceInterrupts(pDevice);
				MB_REG_WR(pDevice, Mailbox.Interrupt[0].Low, 0);
				dummy = MB_REG_RD(pDevice,
					Mailbox.Interrupt[0].Low);
				i++;
			}

			while ((pDevice->pStatusBlkVirt->Status & STATUS_BLOCK_UPDATED) != 0
				&& i < max_intr_loop)
				;

			if (pDevice->Flags & UNDI_FIX_FLAG) {
				REG_WR(pDevice, Grc.LocalCtrl,
					pDevice->GrcLocalCtrl | 0x2);
			}
		}
	} else
		handled = 0;

	if (QQ_GetEntryCnt(&pDevice->RxPacketFreeQ.Container)
		|| pDevice->QueueAgain) {
		LM_QueueRxPackets(pDevice);
	}

	return handled ? B_INVOKE_SCHEDULER : B_UNHANDLED_INTERRUPT;
}


static status_t
b57_read(void *cookie,off_t pos,void *data,size_t *numBytes)
{
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *)cookie;
	PLM_DEVICE_BLOCK pDevice = (PLM_DEVICE_BLOCK) pUmDevice;
	PLM_PACKET pPacket;
	struct B_UM_PACKET *pUmPacket;
	cpu_status cpu;

	if (pUmDevice->block)
		acquire_sem(pUmDevice->packet_release_sem);
	else {
		/* Decrement the receive sem anyway, but don't block
		   this is a horrible hack, but it works. */
		acquire_sem_etc(pUmDevice->packet_release_sem, 1, B_RELATIVE_TIMEOUT, 0);
	}

	cpu = disable_interrupts();
	acquire_spinlock(&pUmDevice->lock);

	pPacket = (PLM_PACKET)
		QQ_PopHead(&pUmDevice->RxPacketReadQ.Container);

	release_spinlock(&pUmDevice->lock);
	restore_interrupts(cpu);

	if (pPacket == 0) {
		*numBytes = -1;
		return B_ERROR;
	}

	pUmPacket = (struct B_UM_PACKET *) pPacket;
	if (pPacket->PacketStatus != LM_STATUS_SUCCESS
		|| pPacket->PacketSize > 1518) {
		cpu = disable_interrupts();
		acquire_spinlock(&pUmDevice->lock);

		QQ_PushTail(&pDevice->RxPacketFreeQ.Container, pPacket);

		release_spinlock(&pUmDevice->lock);
		restore_interrupts(cpu);
		*numBytes = -1;
		return B_ERROR;
	}

	if ((pPacket->PacketSize) < *numBytes)
		*numBytes = pPacket->PacketSize;

	memcpy(data,pUmPacket->data,*numBytes);
	cpu = disable_interrupts();
	acquire_spinlock(&pUmDevice->lock);

	QQ_PushTail(&pDevice->RxPacketFreeQ.Container, pPacket);

	release_spinlock(&pUmDevice->lock);
	restore_interrupts(cpu);

	return B_OK;
}


static status_t
b57_write(void *cookie,off_t pos,const void *data,size_t *numBytes)
{
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *)cookie;
	PLM_DEVICE_BLOCK pDevice = (PLM_DEVICE_BLOCK) pUmDevice;
	PLM_PACKET pPacket;
	struct B_UM_PACKET *pUmPacket;
	cpu_status cpu;

	/*if ((pDevice->LinkStatus == LM_STATUS_LINK_DOWN) || !pDevice->InitDone)
	{
		return ENETDOWN;
	}*/

	pPacket = (PLM_PACKET)
		QQ_PopHead(&pDevice->TxPacketFreeQ.Container);

	if (pPacket == 0)
		return B_ERROR;

	pUmPacket = (struct B_UM_PACKET *)pPacket;
	pUmPacket->data = chunk_pool_get();

	memcpy(pUmPacket->data,data,*numBytes); /* no guarantee data is contiguous, so we have to copy */
	pPacket->PacketSize = pUmPacket->size = *numBytes;

	pPacket->u.Tx.FragCount = 1;
	pPacket->Flags = 0;

	tx_cleanup_thread(pUmDevice);

	cpu = disable_interrupts();
	acquire_spinlock(&pUmDevice->lock);

	LM_SendPacket(pDevice, pPacket);

	release_spinlock(&pUmDevice->lock);
	restore_interrupts(cpu);

	return B_OK;
}


//	#pragma mark -
/* -------- Broadcom MM hooks ----------- */


LM_STATUS
MM_ReadConfig16(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT16 *pValue16)
{
	*pValue16 = (LM_UINT16)pci->read_pci_config(((struct be_b57_dev *)(pDevice))->pci_data.bus,
		((struct be_b57_dev *)(pDevice))->pci_data.device,
		((struct be_b57_dev *)(pDevice))->pci_data.function,
		(uchar)Offset, sizeof(LM_UINT16));
	return LM_STATUS_SUCCESS;
}


LM_STATUS
MM_WriteConfig16(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT16 Value16)
{
	pci->write_pci_config(((struct be_b57_dev *)(pDevice))->pci_data.bus,
		((struct be_b57_dev *)(pDevice))->pci_data.device,
		((struct be_b57_dev *)(pDevice))->pci_data.function,
		(uchar)Offset, sizeof(LM_UINT16), (uint32)Value16);
	return LM_STATUS_SUCCESS;
}


LM_STATUS
MM_ReadConfig32(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT32 *pValue32)
{
	*pValue32 = (LM_UINT32)pci->read_pci_config(((struct be_b57_dev *)(pDevice))->pci_data.bus,
		((struct be_b57_dev *)(pDevice))->pci_data.device,
		((struct be_b57_dev *)(pDevice))->pci_data.function,
		(uchar)Offset, sizeof(LM_UINT32));
	return LM_STATUS_SUCCESS;
}


LM_STATUS
MM_WriteConfig32(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT32 Value32)
{
	pci->write_pci_config(((struct be_b57_dev *)(pDevice))->pci_data.bus,
		((struct be_b57_dev *)(pDevice))->pci_data.device,
		((struct be_b57_dev *)(pDevice))->pci_data.function,
		(uchar)Offset, sizeof(LM_UINT32), (uint32)Value32);
	return LM_STATUS_SUCCESS;
}


LM_STATUS
MM_MapMemBase(PLM_DEVICE_BLOCK pDevice)
{
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *)(pDevice);
	size_t size = pUmDevice->pci_data.u.h0.base_register_sizes[0];

	size = ROUND_UP_TO_PAGE(size);
	pUmDevice->mem_base = map_physical_memory("broadcom_regs",
		pUmDevice->pci_data.u.h0.base_registers[0], size,
		B_ANY_KERNEL_BLOCK_ADDRESS, 0,
		(void **)(&pDevice->pMappedMemBase));

	return LM_STATUS_SUCCESS;
}

/*
LM_STATUS
MM_MapIoBase(PLM_DEVICE_BLOCK pDevice)
{
	pDevice->pMappedMemBase = pci->ram_address(((struct be_b57_dev *)(pDevice))->pci_data.memory_base);
	return LM_STATUS_SUCCESS;
}
*/

LM_STATUS
MM_IndicateRxPackets(PLM_DEVICE_BLOCK pDevice)
{
	struct be_b57_dev *dev = (struct be_b57_dev *)pDevice;
	PLM_PACKET pPacket;

	while (1) {
		pPacket = (PLM_PACKET)
			QQ_PopHead(&pDevice->RxPacketReceivedQ.Container);
		if (pPacket == 0)
			break;

		acquire_spinlock(&dev->lock);
		release_sem_etc(dev->packet_release_sem, 1, B_DO_NOT_RESCHEDULE);
		release_spinlock(&dev->lock);
		QQ_PushTail(&dev->RxPacketReadQ.Container, pPacket);
	}

	return LM_STATUS_SUCCESS;
}


LM_STATUS
MM_IndicateTxPackets(PLM_DEVICE_BLOCK pDevice)
{
	return LM_STATUS_SUCCESS;
}


int32
tx_cleanup_thread(void *us)
{
	PLM_PACKET pPacket;
	PLM_DEVICE_BLOCK pDevice = (PLM_DEVICE_BLOCK)(us);
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *)(us);
	struct B_UM_PACKET *pUmPacket;
	cpu_status cpu;

	while (1) {
		cpu = disable_interrupts();
		acquire_spinlock(&pUmDevice->lock);

		pPacket = (PLM_PACKET)
			QQ_PopHead(&pDevice->TxPacketXmittedQ.Container);

		release_spinlock(&pUmDevice->lock);
		restore_interrupts(cpu);
		if (pPacket == 0)
			break;
		pUmPacket = (struct B_UM_PACKET *)(pPacket);
		chunk_pool_put(pUmPacket->data);
		pUmPacket->data = NULL;

		cpu = disable_interrupts();
		acquire_spinlock(&pUmDevice->lock);
		QQ_PushTail(&pDevice->TxPacketFreeQ.Container, pPacket);
		release_spinlock(&pUmDevice->lock);
		restore_interrupts(cpu);
	}
	return LM_STATUS_SUCCESS;
}

/*LM_STATUS MM_StartTxDma(PLM_DEVICE_BLOCK pDevice, PLM_PACKET pPacket);
LM_STATUS MM_CompleteTxDma(PLM_DEVICE_BLOCK pDevice, PLM_PACKET pPacket);*/

LM_STATUS
MM_AllocateMemory(PLM_DEVICE_BLOCK pDevice, LM_UINT32 BlockSize,
	PLM_VOID *pMemoryBlockVirt)
{
	struct be_b57_dev *dev = (struct be_b57_dev *)(pDevice);

	if (dev->mem_list_num == 16)
		return LM_STATUS_FAILURE;

	*pMemoryBlockVirt = dev->mem_list[(dev->mem_list_num)++] = (void *)malloc(BlockSize);
	return LM_STATUS_SUCCESS;
}


LM_STATUS
MM_AllocateSharedMemory(PLM_DEVICE_BLOCK pDevice, LM_UINT32 BlockSize,
	PLM_VOID *pMemoryBlockVirt, PLM_PHYSICAL_ADDRESS pMemoryBlockPhy,
	LM_BOOL cached /* we ignore this */)
{
	struct be_b57_dev *dev;
	void *pvirt = NULL;
	area_id area_desc;
	physical_entry entry;

	dev = (struct be_b57_dev *)(pDevice);
	area_desc = dev->lockmem_list[dev->lockmem_list_num++] = create_area("broadcom_shared_mem",
		&pvirt, B_ANY_KERNEL_ADDRESS, ROUND_UP_TO_PAGE(BlockSize),
		B_CONTIGUOUS, 0);

	if (area_desc < B_OK)
		return LM_STATUS_FAILURE;

	memset(pvirt, 0, BlockSize);
	*pMemoryBlockVirt = (PLM_VOID) pvirt;

	get_memory_map(pvirt,BlockSize,&entry,1);
	pMemoryBlockPhy->Low = (uint32)entry.address;
	pMemoryBlockPhy->High = (uint32)(entry.address >> 32);
		/* We only support 32 bit */

	return LM_STATUS_SUCCESS;
}


LM_STATUS
MM_GetConfig(PLM_DEVICE_BLOCK pDevice)
{
	pDevice->DisableAutoNeg = FALSE;
	pDevice->RequestedLineSpeed = LM_LINE_SPEED_AUTO;
	pDevice->RequestedDuplexMode = LM_DUPLEX_MODE_FULL;
	pDevice->FlowControlCap = LM_FLOW_CONTROL_AUTO_PAUSE;
	pDevice->RxPacketDescCnt = DEFAULT_RX_PACKET_DESC_COUNT;
	pDevice->TxPacketDescCnt = DEFAULT_TX_PACKET_DESC_COUNT;
	pDevice->TxMaxCoalescedFrames = DEFAULT_TX_MAX_COALESCED_FRAMES;
	pDevice->RxMaxCoalescedFrames = DEFAULT_RX_MAX_COALESCED_FRAMES;
	pDevice->RxStdDescCnt = DEFAULT_STD_RCV_DESC_COUNT;
	pDevice->RxCoalescingTicks = DEFAULT_RX_COALESCING_TICKS;
	pDevice->TxCoalescingTicks = DEFAULT_TX_COALESCING_TICKS;
	pDevice->StatsCoalescingTicks = DEFAULT_STATS_COALESCING_TICKS;
	pDevice->TaskToOffload = LM_TASK_OFFLOAD_NONE;

	return LM_STATUS_SUCCESS;
}


LM_STATUS
MM_IndicateStatus(PLM_DEVICE_BLOCK pDevice, LM_STATUS Status)
{
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *)pDevice;

	if (pUmDevice->linkChangeSem != -1)
		release_sem_etc(pUmDevice->linkChangeSem, 1,
			B_DO_NOT_RESCHEDULE);
#endif

	return LM_STATUS_SUCCESS;
}


LM_STATUS
MM_InitializeUmPackets(PLM_DEVICE_BLOCK pDevice)
{
	int i;
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *) pDevice;
	struct B_UM_PACKET *pUmPacket;
	PLM_PACKET pPacket;

	for (i = 0; i < pDevice->RxPacketDescCnt; i++) {
		pPacket = QQ_PopHead(&pDevice->RxPacketFreeQ.Container);
		pUmPacket = (struct B_UM_PACKET *) pPacket;
		pUmPacket->data = chunk_pool_get();
		if (pUmPacket->data == 0) {
			//QQ_PushTail(&pUmDevice->rx_out_of_buf_q.Container, pPacket);
			// No pretty rx_out_of_buf_q, but we sure as hell don't want anything to do with these packets, so we leak them.
			// Probably not the best idea, but it works.
			continue;
		}
		QQ_PushTail(&pDevice->RxPacketFreeQ.Container, pPacket);
	}

	return LM_STATUS_SUCCESS;
}

LM_STATUS MM_FreeRxBuffer(PLM_DEVICE_BLOCK pDevice, PLM_PACKET pPacket) {
	struct B_UM_PACKET *pUmPacket;
	struct be_b57_dev *pUmDevice = (struct be_b57_dev *) pDevice;
	pUmPacket = (struct B_UM_PACKET *) pPacket;
	chunk_pool_put(pUmPacket->data);
	pUmPacket->data = NULL;
	return LM_STATUS_SUCCESS;
}


void
MM_UnmapRxDma(LM_DEVICE_BLOCK *pDevice, LM_PACKET *pPacket)
{
}


LM_STATUS
MM_CoalesceTxBuffer(PLM_DEVICE_BLOCK pDevice, PLM_PACKET pPacket)
{
	/* Our buffers are pre-coalesced (which slows things down a little) */
	return LM_STATUS_SUCCESS;
}


LM_DEVICE_BLOCK *
MM_FindPeerDev(LM_DEVICE_BLOCK *pDevice)
{
	/* I have no idea what this routine does. I think it's optional... */
	return 0;
}


LM_STATUS
MM_Sleep(LM_DEVICE_BLOCK *pDevice, LM_UINT32 msec)
{
	snooze(msec*1e3);
	return LM_STATUS_SUCCESS;
}
