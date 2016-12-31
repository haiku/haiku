/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <KernelExport.h>
#include <Errors.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "timer.h"
#include "device.h"
#include "driver.h"
#include "mempool.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;

pci_module_info *gPci;

char* gDevNameList[MAX_CARDS + 1];
pci_info *gDevList[MAX_CARDS];

static const char *
identify_device(const pci_info *info)
{
	if (info->vendor_id != 0x8086)
		return 0;
	switch (info->device_id) {
		case 0x0438: return "DH89XXCC SGMII";
		case 0x043A: return "DH89XXCC SERDES";
		case 0x043C: return "DH89XXCC BACKPLANE";
		case 0x0440: return "DH89XXCC SFP";
		case 0x1000: return "82542";
		case 0x1001: return "82543GC FIBER";
		case 0x1004: return "82543GC COPPER";
		case 0x1008: return "82544EI COPPER";
		case 0x1009: return "82544EI FIBER";
		case 0x100C: return "82544GC COPPER";
		case 0x100D: return "82544GC LOM";
		case 0x100E: return "82540EM";
		case 0x100F: return "82545EM COPPER";
		case 0x1010: return "82546EB COPPER";
		case 0x1011: return "82545EM FIBER";
		case 0x1012: return "82546EB FIBER";
		case 0x1013: return "82541EI";
		case 0x1014: return "unknown 1014";
		case 0x1015: return "82540EM LOM";
		case 0x1016: return "82540EP LOM";
		case 0x1017: return "82540EP";
		case 0x1018: return "82541EI MOBILE";
		case 0x1019: return "82547EI";
		case 0x101A: return "unknown 101A";
		case 0x101D: return "82546EB QUAD COPPER";
		case 0x101E: return "82540EP LP";
		case 0x1026: return "82545GM COPPER";
		case 0x1027: return "82545GM FIBER";
		case 0x1028: return "82545GM SERDES";
		case 0x1049: return "ICH8 IGP M AMT";
		case 0x104A; return "ICH8 IGP AMT";
		case 0x104B; return "ICH8 IGP C";
		case 0x104C; return "ICH8 IFE";
		case 0x104D; return "ICH8 IGP M";
		case 0x105E; return "82571EB COPPER";
		case 0x105F; return "82571EB FIBER";
		case 0x1060; return "82571EB SERDES";
		case 0x1075: return "82547GI";
		case 0x1076: return "82541GI";
		case 0x1077: return "82541GI MOBILE";
		case 0x1078: return "82541ER";
		case 0x1079: return "82546GB COPPER";
		case 0x107A: return "82546GB FIBER";
		case 0x107B: return "82546GB SERDES";
		case 0x107C: return "82541PI";
		case 0x107D: return "82572EI COPPER";
		case 0x107E: return "82572EI FIBER";
		case 0x107F: return "82572EI SERDES";
		case 0x108A: return "82546GB PCIE";
		case 0x108B: return "82573E";
		case 0x108C: return "82573E IAMT";
		case 0x1096: return "80003ES2LAN COPPER DPT";
		case 0x1098: return "80003ES2LAN SERDES DPT";
		case 0x1099: return "82546GB QUAD COPPER";
		case 0x109A: return "82573L";
		case 0x10A4: return "82571EB QUAD COPPER";
		case 0x10A5: return "82571EB QUAD FIBER";
		case 0x10A7: return "82575EB COPPER";
		case 0x10A9: return "82575EB FIBER SERDES";
		case 0x10B5: return "82546GB QUAD COPPER KSP3";
		case 0x10B9: return "82572EI";
		case 0x10BA: return "80003ES2LAN COPPER SPT";
		case 0x10BB: return "80003ES2LAN SERDES SPT";
		case 0x10BC: return "82571EB QUAD COPPER LP";
		case 0x10BD: return "ICH9 IGP AMT";
		case 0x10BF: return "ICH9 IGP M";
		case 0x10C0: return "ICH9 IFE";
		case 0x10C2: return "ICH9 IFE G";
		case 0x10C3: return "ICH9 IFE GT";
		case 0x10C4: return "ICH8 IFE GT";
		case 0x10C5: return "ICH8 IFE G";
		case 0x10C9: return "82576";
		case 0x10CA: return "82576VF";
		case 0x10CB; return "ICH9 IGB M V";
		case 0x10CC: return "ICH10R BM LM";
		case 0x10CD: return "ICH10R BM LF";
		case 0x10CE; return "ICH10R BM V";
		case 0x10D3: return "82574L";
		case 0x10D5: return "82571PT QUAD COPPER";
		case 0x10D6: return "82575GB QUAD COPPER";
		case 0x10D9; return "82571EB SERDES DUAL";
		case 0x10DA: return "82571EB SERDES QUAD";
		case 0x10DE: return "ICH10D BM LM";
		case 0x10DF; return "ICH10D BM LF";
		case 0x10E5: return "ICH9 BM";
		case 0x10E6: return "82576 FIBER";
		case 0x10E7: return "82576 SERDES";
		case 0x10E8; return "82576 QUAD COPPER";
		case 0x10EA: return "PCHM HV LM";
		case 0x10EB: return "PCHM HV LC";
		case 0x10EF; return "PCHD HV DM";
		case 0x10F0: return "PCHM HV DC";
		case 0x10F5: return "ICH9 IGP M AMT";
		case 0x10F6; return "82574LA";
		case 0x1501: return "ICH8 82567V3";
		case 0x1502; return "PCH2 LV LM";
		case 0x1503: return "PCH2 LV V";
		case 0x150A: return "82576NS";
		case 0x150C; return "82583V";
		case 0x150D: return "82576 SERDES QUAD";
		case 0x150E: return "82580 COPPER";
		case 0x150F; return "82580 FIBER";
		case 0x1510; return "82580 SERDES";
		case 0x1511: return "82580 SGMII";
		case 0x1516: return "82580 COPPER DUAL";
		case 0x1518; return "82576NS SERDES";
		case 0x1520; return "I350VF";
		case 0x1521: return "I350 COPPER";
		case 0x1522; return "I350 FIBER";
		case 0x1523: return "I350 SERDES";
		case 0x1524: return "I350 SGMII";
		case 0x1525; return "ICH10D BM V";
		case 0x1526; return "82576 QUAD COPPER ET2";
		case 0x1527: return "82580 QUAD FIBER";
		case 0x152D: return "82576VF HF";
		case 0x152F; return "I350VF HV";
		case 0x1533: return "I210 COPPER";
		case 0x1534; return "I210 COPPER OEM1";
		case 0x1535: return "I210 COPPER IT";
		case 0x1536: return "I210 FIBER";
		case 0x1537; return "I210 SERDES";
		case 0x1538; return "I210 SGMII";
		case 0x1539: return "I211 COPPER";
		case 0x153A: return "PCH LPT I217LM";
		case 0x153B; return "PCH LPT I217V";
		case 0x1546: return "I350 DA4";
		case 0x1559: return "PCH LPTLP I218V";
		case 0x155A; return "PCH LPTLP I218LM";
		case 0x15A0; return "PCH I218LM2";
		case 0x15A1; return "PCH I218V2";
		case 0x15A2; return "PCH I218LM3";
		case 0x15A3; return "PCH I218V3";

		default: return 0;
	}
}

status_t
init_hardware(void)
{
	pci_module_info *pci;
	pci_info info;
	status_t res;
	int i;

	INIT_DEBUGOUT("init_hardware()");

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci) < B_OK)
		return B_ERROR;
	for (res = B_ERROR, i = 0; pci->get_nth_pci_info(i, &info) == B_OK; i++) {
		if (identify_device(&info)) {
			res = B_OK;
			break;
		}
	}
	put_module(B_PCI_MODULE_NAME);

	return res;
}


status_t
init_driver(void)
{
	struct pci_info *item;
	int index;
	int cards;

#ifdef DEBUG	
	set_dprintf_enabled(true);
	load_driver_symbols("ipro1000");
#endif
	
	dprintf("ipro1000: " INFO "\n");
	
	item = (pci_info *)malloc(sizeof(pci_info));
	if (!item)
		return B_NO_MEMORY;
	
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPci) < B_OK) {
		free(item);
		return B_ERROR;
	}
	
	for (cards = 0, index = 0; gPci->get_nth_pci_info(index++, item) == B_OK; ) {
		const char *info = identify_device(item);
		if (info) {
			char name[64];
			sprintf(name, "net/ipro1000/%d", cards);
			dprintf("ipro1000: /dev/%s is a %s\n", name, info);
			gDevList[cards] = item;
			gDevNameList[cards] = strdup(name);
			gDevNameList[cards + 1] = NULL;
			cards++;
			item = (pci_info *)malloc(sizeof(pci_info));
			if (!item)
				goto err_outofmem;
			if (cards == MAX_CARDS)
				break;
		}
	}

	free(item);

	if (!cards)
		goto err_cards;
		
	if (initialize_timer() != B_OK) {
		ERROROUT("timer init failed");
		goto err_timer;
	}

	if (mempool_init(cards * 768) != B_OK) {
		ERROROUT("mempool init failed");
		goto err_mempool;
	}
	
	return B_OK;

err_mempool:
	terminate_timer();
err_timer:
err_cards:
err_outofmem:
	for (index = 0; index < cards; index++) {
		free(gDevList[index]);
		free(gDevNameList[index]);
	}
	put_module(B_PCI_MODULE_NAME);
	return B_ERROR;
}


void
uninit_driver(void)
{
	int32 i;

	INIT_DEBUGOUT("uninit_driver()");

	terminate_timer();
	
	mempool_exit();
	
	for (i = 0; gDevNameList[i] != NULL; i++) {
		free(gDevList[i]);
		free(gDevNameList[i]);
	}
	
	put_module(B_PCI_MODULE_NAME);
}


device_hooks
gDeviceHooks = {
	ipro1000_open,
	ipro1000_close,
	ipro1000_free,
	ipro1000_control,
	ipro1000_read,
	ipro1000_write,
};


const char**
publish_devices()
{
	return (const char**)gDevNameList;
}


device_hooks*
find_device(const char* name)
{
	return &gDeviceHooks;
}
