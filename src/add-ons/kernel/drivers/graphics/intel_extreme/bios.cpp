/*
 * Copyright 2012-2014, Artem Falcon <lomka@gero.in>
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <KernelExport.h>
#include <directories.h>
#include <stdio.h>
#include <driver_settings.h>

#include "intel_extreme.h"
#include "driver.h"


#define TRACE_BIOS
#ifdef TRACE_BIOS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct vbt_header {
	uint8 signature[20];
	uint16 version;
	uint16 header_size;
	uint16 vbt_size;
	uint8 vbt_checksum;
	uint8 reserved0;
	uint32 bdb_offset;
	uint32 aim_offset[4];
} __attribute__((packed));


struct bdb_header {
	uint8 signature[16];
	uint16 version;
	uint16 header_size;
	uint16 bdb_size;
} __attribute__((packed));


enum bdb_block_id {
	BDB_GENERAL_FEATURES = 1,
	BDB_GENERAL_DEFINITIONS,
	BDB_LVDS_OPTIONS = 40,
	BDB_LVDS_LFP_DATA_PTRS = 41,
	BDB_LVDS_BACKLIGHT = 43,
	BDB_GENERIC_DTD = 58
};


struct bdb_general_features {
	uint8 id;
	uint16 size;

	uint8 panel_fitting: 2;
	bool flexaim: 1;
	bool msg_enable: 1;
	uint8 clear_screen: 3;
	bool color_flip: 1;

	bool download_ext_vbt: 1;
	bool enable_ssc: 1;
	bool ssc_freq: 1;
	bool enable_lfp_on_override: 1;
	bool disable_ssc_ddt: 1;
	bool underscan_vga_timings: 1;
	bool display_clock_mode: 1;
	bool vbios_hotplug_support: 1;

	bool disable_smooth_vision: 1;
	bool single_dvi: 1;
	bool rotate_180: 1;
	bool fdi_rx_polarity_inverted: 1;
	bool vbios_extended_mode: 1;
	bool copy_ilfp_dtd_to_svo_lvds_dtd: 1;
	bool panel_best_fit_timing: 1;
	bool ignore_strap_state: 1;

	uint8 legacy_monitor_detect;

	bool int_crt_support: 1;
	bool int_tv_support: 1;
	bool int_efp_support: 1;
	bool dp_ssc_enable: 1;
	bool dp_ssc_freq: 1;
	bool dp_ssc_dongle_supported: 1;
	uint8 rsvd11: 2;

	uint8 tc_hpd_retry_timeout: 7;
	bool rsvd12: 1;

	uint8 afc_startup_config: 2;
	uint8 rsvd13: 6;
} __attribute__((packed));


struct bdb_general_definitions {
	uint8 id;
	uint16 size;
	uint8 crt_ddc_gmbus_pin;
	uint8 dpms_bits;
	uint8 boot_display[2];
	uint8 child_device_size;
	uint8 devices[];
} __attribute__((packed));


// FIXME the struct definition for the bdb_header is not complete, so we rely
// on direct access with hardcoded offsets to get the timings out of it.
#define _PIXEL_CLOCK(x) (x[0] + (x[1] << 8)) * 10000
#define _H_ACTIVE(x) (x[2] + ((x[4] & 0xF0) << 4))
#define _H_BLANK(x) (x[3] + ((x[4] & 0x0F) << 8))
#define _H_SYNC_OFF(x) (x[8] + ((x[11] & 0xC0) << 2))
#define _H_SYNC_WIDTH(x) (x[9] + ((x[11] & 0x30) << 4))
#define _V_ACTIVE(x) (x[5] + ((x[7] & 0xF0) << 4))
#define _V_BLANK(x) (x[6] + ((x[7] & 0x0F) << 8))
#define _V_SYNC_OFF(x) ((x[10] >> 4) + ((x[11] & 0x0C) << 2))
#define _V_SYNC_WIDTH(x) ((x[10] & 0x0F) + ((x[11] & 0x03) << 4))

#define BDB_BACKLIGHT_TYPE_NONE 0
#define BDB_BACKLIGHT_TYPE_PWM 2


struct lvds_bdb1 {
	uint8 id;
	uint16 size;
	uint8 panel_type;
	uint8 reserved0;
	uint16 caps;
} __attribute__((packed));


struct lvds_bdb2_entry {
	uint16 lfp_info_offset;
	uint8 lfp_info_size;
	uint16 lfp_edid_dtd_offset;
	uint8 lfp_edid_dtd_size;
	uint16 lfp_edid_pid_offset;
	uint8 lfp_edid_pid_size;
} __attribute__((packed));


struct lvds_bdb2 {
	uint8 id;
	uint16 size;
	uint8 table_size; /* followed by one or more lvds_data_ptr structs */
	struct lvds_bdb2_entry panels[16];
} __attribute__((packed));


struct lvds_bdb2_lfp_info {
	uint16 x_res;
	uint16 y_res;
	uint32 lvds_reg;
	uint32 lvds_reg_val;
	uint32 pp_on_reg;
	uint32 pp_on_reg_val;
	uint32 pp_off_reg;
	uint32 pp_off_reg_val;
	uint32 pp_cycle_reg;
	uint32 pp_cycle_reg_val;
	uint32 pfit_reg;
	uint32 pfit_reg_val;
	uint16 terminator;
} __attribute__((packed));



struct generic_dtd_entry {
	uint32 pixel_clock;
	uint16 hactive;
	uint16 hblank;
	uint16 hfront_porch;
	uint16 hsync;
	uint16 vactive;
	uint16 vblank;
	uint16 vfront_porch;
	uint16 vsync;
	uint16 width_mm;
	uint16 height_mm;

	uint8 rsvd_flags:6;
	uint8 vsync_positive_polarity:1;
	uint8 hsync_positive_polarity:1;

	uint8 rsvd[3];
} __attribute__((packed));


struct bdb_generic_dtd {
	uint8 id;
	uint16 size;
	uint16 gdtd_size;
	struct generic_dtd_entry dtd[];
} __attribute__((packed));


struct bdb_lfp_backlight_data_entry {
	uint8 type: 2;
	uint8 active_low_pwm: 1;
	uint8 reserved1: 5;
	uint16 pwm_freq_hz;
	uint8 min_brightness; // Versions < 234
	uint8 reserved2;
	uint8 reserved3;
} __attribute__((packed));


struct bdb_lfp_backlight_control_method {
	uint8 type: 4;
	uint8 controller: 4;
} __attribute__((packed));


struct lfp_brightness_level {
	uint16 level;
	uint16 reserved;
} __attribute__((packed));


struct bdb_lfp_backlight_data {
	uint8 entry_size;
	struct bdb_lfp_backlight_data_entry data[16];
	uint8 level [16]; // Only for versions < 234
	struct bdb_lfp_backlight_control_method backlight_control[16];
	struct lfp_brightness_level brightness_level[16]; // Versions >= 234
	struct lfp_brightness_level brightness_min_level[16]; // Versions >= 234
	uint8 brightness_precision_bits[16]; // Versions >= 236
} __attribute__((packed));


static struct vbios {
	area_id			area;
	uint8*			memory;
	uint16_t ReadWord(off_t address)
	{
		return memory[address] | memory[address + 1] << 8;
	}
} vbios;


static bool
read_settings_dumpRom(void)
{
	bool dumpRom = false;

	void* settings = load_driver_settings("intel_extreme");
	if (settings != NULL) {
		dumpRom = get_driver_boolean_parameter(settings,
			"dump_rom", false, false);

		unload_driver_settings(settings);
	}
	return dumpRom;
}


static void
dumprom(void *rom, uint32 size, intel_info &info)
{
	int fd;
	uint32 cnt;
	char fname[64];

	/* determine the romfile name: we need split-up per card in the system */
	sprintf (fname, kUserDirectory "//intel_extreme.%04x_%04x_%02x%02x%02x.rom",
		info.pci->vendor_id, info.pci->device_id, info.pci->bus, info.pci->device, info.pci->function);

	fd = open (fname, O_WRONLY | O_CREAT, 0666);
	if (fd < 0) return;

	/* The ROM size is a multiple of 1kb.. */
	for (cnt = 0; (cnt < size); cnt += 1024)
		write (fd, ((void *)(((uint8 *)rom) + cnt)), 1024);
	close (fd);
}


/*!	This is reimplementation, Haiku uses BIOS call and gets most current panel
	info, we're, otherwise, digging in VBIOS memory and parsing VBT tables to
	get native panel timings. This will allow to get non-updated,
	PROM-programmed timings info when compensation mode is off on your machine.
*/

// outdated: https://01.org/sites/default/files/documentation/skl_opregion_rev0p5.pdf

#define ASLS 0xfc // ASL Storage.
#define OPREGION_SIGNATURE "IntelGraphicsMem"
#define OPREGION_ASLE_OFFSET   0x300
#define OPREGION_VBT_OFFSET   0x400
#define MBOX_ACPI (1 << 0)
#define MBOX_SWSCI (1 << 1)
#define MBOX_ASLE (1 << 2)
#define MBOX_ASLE_EXT (1 << 3)


struct opregion_header {
	uint8 signature[16];
	uint32 size;
	uint8 reserved;
	uint8 revision_version;
	uint8 minor_version;
	uint8 major_version;
	uint8 sver[32];
	uint8 vver[16];
	uint8 gver[16];
	uint32 mboxes;
	uint32 driver_model;
	uint32 platform_configuration;
	uint8 gop_version[32];
	uint8 rsvd[124];
} __attribute__((packed));


struct opregion_asle {
	uint32 ardy;
	uint32 aslc;
	uint32 tche;
	uint32 alsi;
	uint32 bclp;
	uint32 pfit;
	uint32 cblv;
	uint16 bclm[20];
	uint32 cpfm;
	uint32 epfm;
	uint8 plut[74];
	uint32 pfmb;
	uint32 cddv;
	uint32 pcft;
	uint32 srot;
	uint32 iuer;
	uint64 fdss;
	uint32 fdsp;
	uint32 stat;
	uint64 rvda;
	uint32 rvds;
	uint8 rsvd[58];
} __attribute__((packed));


static bool
get_bios(int* vbtOffset)
{
	STATIC_ASSERT(sizeof(opregion_header) == 0x100);
	STATIC_ASSERT(sizeof(opregion_asle) == 0x100);

	intel_info &info = *gDeviceInfo[0];
	// first try to fetch Intel OpRegion which should be populated by the BIOS at start
	uint32 kVBIOSSize;

	for (uint32 romMethod = 0; romMethod < 2; romMethod++) {
		switch(romMethod) {
			case 0:
			{
				// get OpRegion - see Intel ACPI IGD info in acpi_igd_opregion_spec_0.pdf
				uint64 kVBIOSAddress = get_pci_config(info.pci, ASLS, 4);
				if (kVBIOSAddress == 0) {
					TRACE((DEVICE_NAME ": ACPI OpRegion not supported!\n"));
					continue;
				}
				kVBIOSSize = 8 * 1024;
				TRACE((DEVICE_NAME ": Graphic OpRegion physical addr: 0x%" B_PRIx64
						"; size: 0x%" B_PRIx32 "\n", kVBIOSAddress, kVBIOSSize));
				vbios.area = map_physical_memory("ASLS mapping", kVBIOSAddress,
					kVBIOSSize, B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA, (void **)&vbios.memory);
				if (vbios.area < 0)
					continue;
				TRACE((DEVICE_NAME ": mapping OpRegion: 0x%" B_PRIx64 " -> %p\n",
					kVBIOSAddress, vbios.memory));
				// check if we got the OpRegion and signature
				if (memcmp(vbios.memory, OPREGION_SIGNATURE, 16) != 0) {
					TRACE((DEVICE_NAME ": OpRegion signature mismatch\n"));
					delete_area(vbios.area);
					vbios.area = -1;
					continue;
				}
				opregion_header* header = (opregion_header*)vbios.memory;
				opregion_asle* asle = (opregion_asle*)(vbios.memory + OPREGION_ASLE_OFFSET);
				if (header->major_version < 2 || (header->mboxes & MBOX_ASLE) == 0
					|| asle->rvda == 0 || asle->rvds == 0) {
					vbios.memory += OPREGION_VBT_OFFSET;
					kVBIOSSize -= OPREGION_VBT_OFFSET;
					break;
				}
				uint64 rvda = asle->rvda;
				kVBIOSSize = asle->rvds;
				if (header->major_version > 3 || header->minor_version >= 1) {
					rvda += kVBIOSAddress;
				}
				TRACE((DEVICE_NAME ": RVDA physical addr: 0x%" B_PRIx64
						"; size: 0x%" B_PRIx32 "\n", rvda, kVBIOSSize));
				delete_area(vbios.area);
				vbios.area = map_physical_memory("RVDA mapping", rvda,
					kVBIOSSize, B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA, (void **)&vbios.memory);
				if (vbios.area < 0)
					continue;
				break;
			}
			case 1:
			{
				uint64 kVBIOSAddress = 0xc0000;
				kVBIOSSize = 64 * 1024;
				/* !!!DANGER!!!: mapping of BIOS using legacy location as a fallback,
				  hence, if panel mode will be set using info from VBT this way, it will
				  be taken from primary card's VBIOS */
				vbios.area = map_physical_memory("VBIOS mapping", kVBIOSAddress,
					kVBIOSSize, B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA, (void**)&vbios.memory);
				if (vbios.area < 0)
					continue;

				TRACE((DEVICE_NAME ": mapping VBIOS: 0x%" B_PRIx64 " -> %p\n",
					kVBIOSAddress, vbios.memory));
				break;
			}
		}

		// dump ROM to file if selected in settings file
		if (read_settings_dumpRom())
			dumprom(vbios.memory, kVBIOSSize, info);

		// scan BIOS for VBT signature
		*vbtOffset = kVBIOSSize;
		for (uint32 i = 0; i + 4 < kVBIOSSize; i += 4) {
			if (memcmp(vbios.memory + i, "$VBT", 4) == 0) {
				*vbtOffset = i;
				break;
			}
		}

		if ((*vbtOffset + (uint32)sizeof(vbt_header)) >= kVBIOSSize) {
			TRACE((DEVICE_NAME": bad VBT offset : 0x%x\n", *vbtOffset));
			delete_area(vbios.area);
			continue;
		}

		struct vbt_header* vbt = (struct vbt_header*)(vbios.memory + *vbtOffset);
		if (memcmp(vbt->signature, "$VBT", 4) != 0) {
			TRACE((DEVICE_NAME": bad VBT signature: %20s\n", vbt->signature));
			delete_area(vbios.area);
			continue;
		}
		return true;
	}

	return false;
}


static void
sanitize_panel_timing(display_timing& timing)
{
	bool bogus = false;

	/* handle bogus h/vtotal values, if got such */
	if (timing.h_sync_end > timing.h_total) {
		timing.h_total = timing.h_sync_end + 1;
		bogus = true;
		TRACE((DEVICE_NAME": got bogus htotal. Fixing\n"));
	}
	if (timing.v_sync_end > timing.v_total) {
		timing.v_total = timing.v_sync_end + 1;
		bogus = true;
		TRACE((DEVICE_NAME": got bogus vtotal. Fixing\n"));
	}

	if (bogus) {
		TRACE((DEVICE_NAME": adjusted LFP modeline: %" B_PRIu32 " KHz,\t"
			"%d %d %d %d   %d %d %d %d\n",
			timing.pixel_clock / (timing.h_total * timing.v_total),
			timing.h_display, timing.h_sync_start,
			timing.h_sync_end, timing.h_total,
			timing.v_display, timing.v_sync_start,
			timing.v_sync_end, timing.v_total));
	}
}


bool
parse_vbt_from_bios(struct intel_shared_info* info)
{
	display_timing* panelTiming = &info->panel_timing;
	uint16* minBrightness = &info->min_brightness;

	int vbtOffset = 0;
	if (!get_bios(&vbtOffset))
		return false;

	struct vbt_header* vbt = (struct vbt_header*)(vbios.memory + vbtOffset);
	int bdbOffset = vbtOffset + vbt->bdb_offset;

	struct bdb_header* bdb = (struct bdb_header*)(vbios.memory + bdbOffset);
	if (memcmp(bdb->signature, "BIOS_DATA_BLOCK ", 16) != 0) {
		TRACE((DEVICE_NAME": bad BDB signature\n"));
		delete_area(vbios.area);
	}
	TRACE((DEVICE_NAME ": VBT signature \"%.*s\", BDB version %d\n",
			(int)sizeof(vbt->signature), vbt->signature, bdb->version));

	int blockSize;
	int panelType = -1;
	bool panelTimingFound = false;

	for (int bdbBlockOffset = bdb->header_size; bdbBlockOffset < bdb->bdb_size;
			bdbBlockOffset += blockSize) {
		int start = bdbOffset + bdbBlockOffset;

		int id = vbios.memory[start];
		blockSize = vbios.ReadWord(start + 1) + 3;
		switch (id) {
			case BDB_GENERAL_FEATURES:
			{
				struct bdb_general_features* features;
				features = (struct bdb_general_features*)(vbios.memory + start);
				if (bdb->version >= 155
					&& (info->device_type.HasDDI()
						|| info->device_type.InGroup(INTEL_GROUP_VLV))) {
					info->internal_crt_support = features->int_crt_support;
					TRACE((DEVICE_NAME ": internal_crt_support: 0x%x\n",
						info->internal_crt_support));
				}
				break;
			}
			case BDB_GENERAL_DEFINITIONS:
			{
				info->device_config_count = 0;
				struct bdb_general_definitions* defs;
				if (bdb->version < 111)
					break;
				defs = (struct bdb_general_definitions*)(vbios.memory + start);
				uint8 childDeviceSize = defs->child_device_size;
				uint32 device_config_count = (blockSize - sizeof(*defs)) / childDeviceSize;
				for (uint32 i = 0; i < device_config_count; i++) {
					child_device_config* config =
						(child_device_config*)(&defs->devices[i * childDeviceSize]);
					if (config->device_type == 0)
						continue;
					memcpy(&info->device_configs[info->device_config_count], config,
						min_c(sizeof(child_device_config), childDeviceSize));
					TRACE((DEVICE_NAME ": found child device type: 0x%x\n", config->device_type));
					info->device_config_count++;
					if (info->device_config_count > 10)
						break;
				}
				break;
			}
			case BDB_LVDS_OPTIONS:
			{
				struct lvds_bdb1 *lvds1;
				lvds1 = (struct lvds_bdb1 *)(vbios.memory + start);
				panelType = lvds1->panel_type;
				if (panelType > 0xf) {
					TRACE((DEVICE_NAME ": invalid panel type %d\n", panelType));
					panelType = -1;
					break;
				}
				TRACE((DEVICE_NAME ": panel type: %d\n", panelType));
				break;
			}
			case BDB_LVDS_LFP_DATA_PTRS:
			{
				// First make sure we found block BDB_LVDS_OPTIONS and the panel type
				if (panelType == -1)
					break;

				// on newer versions, check also generic DTD, use LFP panel DTD as a fallback
				if (bdb->version >= 229 && panelTimingFound)
					break;

				struct lvds_bdb2 *lvds2;
				struct lvds_bdb2_lfp_info *lvds2_lfp_info;

				lvds2 = (struct lvds_bdb2 *)(vbios.memory + start);
				lvds2_lfp_info = (struct lvds_bdb2_lfp_info *)
					(vbios.memory + bdbOffset
					+ lvds2->panels[panelType].lfp_info_offset);
				/* Show terminator: Check not done in drm i915 driver: Assuming chk not valid. */
				TRACE((DEVICE_NAME ": LFP info terminator %x\n", lvds2_lfp_info->terminator));

				uint8_t* timing_data = vbios.memory + bdbOffset
					+ lvds2->panels[panelType].lfp_edid_dtd_offset;
				TRACE((DEVICE_NAME ": found LFP of size %d x %d "
					"in BIOS VBT tables\n",
					lvds2_lfp_info->x_res, lvds2_lfp_info->y_res));

				panelTiming->pixel_clock = _PIXEL_CLOCK(timing_data) / 1000;
				panelTiming->h_sync_start = _H_ACTIVE(timing_data) + _H_SYNC_OFF(timing_data);
				panelTiming->h_sync_end = panelTiming->h_sync_start + _H_SYNC_WIDTH(timing_data);
				panelTiming->h_total = _H_ACTIVE(timing_data) + _H_BLANK(timing_data);
				panelTiming->h_display = _H_ACTIVE(timing_data);
				panelTiming->v_sync_start = _V_ACTIVE(timing_data) + _V_SYNC_OFF(timing_data);
				panelTiming->v_sync_end = panelTiming->v_sync_start + _V_SYNC_WIDTH(timing_data);
				panelTiming->v_total = _V_ACTIVE(timing_data) + _V_BLANK(timing_data);
				panelTiming->v_display = _V_ACTIVE(timing_data);
				panelTiming->flags = 0;

				sanitize_panel_timing(*panelTiming);

				panelTimingFound = true;
				break;

			}
			case BDB_GENERIC_DTD:
			{
				// First make sure we found block BDB_LVDS_OPTIONS and the panel type
				if (panelType == -1)
					break;

				bdb_generic_dtd* generic_dtd = (bdb_generic_dtd*)(vbios.memory + start);
				if (generic_dtd->gdtd_size < sizeof(bdb_generic_dtd)) {
					TRACE((DEVICE_NAME ": invalid gdtd_size %d\n", generic_dtd->gdtd_size));
					break;
				}
				int32 count = (blockSize - sizeof(bdb_generic_dtd)) / generic_dtd->gdtd_size;
				if (panelType >= count) {
					TRACE((DEVICE_NAME ": panel type not found %d in %" B_PRId32 " dtds\n",
						panelType, count));
					break;
				}
				generic_dtd_entry* dtd = &generic_dtd->dtd[panelType];
				TRACE((DEVICE_NAME ": pixel_clock %" B_PRId32 " "
					"hactive %d hfront_porch %d hsync %d hblank %d "
					"vactive %d vfront_porch %d vsync %d vblank %d\n",
						dtd->pixel_clock, dtd->hactive, dtd->hfront_porch, dtd->hsync, dtd->hblank,
						dtd->vactive, dtd->vfront_porch, dtd->vsync, dtd->vblank));

				TRACE((DEVICE_NAME ": found generic dtd entry of size %d x %d "
					"in BIOS VBT tables\n", dtd->hactive, dtd->vactive));

				panelTiming->pixel_clock = dtd->pixel_clock;
				panelTiming->h_sync_start = dtd->hactive + dtd->hfront_porch;
				panelTiming->h_sync_end = panelTiming->h_sync_start + dtd->hsync;
				panelTiming->h_total = dtd->hactive + dtd->hblank;
				panelTiming->h_display = dtd->hactive;
				panelTiming->v_sync_start = dtd->vactive + dtd->vfront_porch;
				panelTiming->v_sync_end = panelTiming->v_sync_start + dtd->vsync;
				panelTiming->v_total = dtd->vactive + dtd->vblank;
				panelTiming->v_display = dtd->vactive;
				panelTiming->flags = 0;
				if (dtd->hsync_positive_polarity)
					panelTiming->flags |= B_POSITIVE_HSYNC;
				if (dtd->vsync_positive_polarity)
					panelTiming->flags |= B_POSITIVE_VSYNC;

				sanitize_panel_timing(*panelTiming);
				panelTimingFound = true;
				break;
			}
			case BDB_LVDS_BACKLIGHT:
			{
				TRACE((DEVICE_NAME ": found bdb lvds backlight info\n"));
				// First make sure we found block BDB_LVDS_OPTIONS and the panel type
				if (panelType == -1)
					break;

				bdb_lfp_backlight_data* backlightData
					= (bdb_lfp_backlight_data*)(vbios.memory + start);

				const struct bdb_lfp_backlight_data_entry* entry = &backlightData->data[panelType];

				if (entry->type == BDB_BACKLIGHT_TYPE_PWM) {
					uint16 minLevel;
					if (bdb->version < 234) {
						minLevel = entry->min_brightness;
					} else {
						minLevel = backlightData->brightness_min_level[panelType].level;
						if (bdb->version >= 236
							&& backlightData->brightness_precision_bits[panelType] == 16) {
							TRACE((DEVICE_NAME ": divide level by 255\n"));
							minLevel /= 255;
						}
					}

					*minBrightness = minLevel;
					TRACE((DEVICE_NAME ": display %d min brightness level is %u\n", panelType,
						minLevel));
				} else {
					TRACE((DEVICE_NAME ": display %d does not have PWM\n", panelType));
				}
				break;
			}
		}
	}

	delete_area(vbios.area);
	return panelTimingFound;
}
