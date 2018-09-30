/*
 * Copyright 2012-2014, Artem Falcon <lomka@gero.in>
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <KernelExport.h>

#include "intel_extreme.h"


#define TRACE_BIOS
#ifdef TRACE_BIOS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static const off_t kVbtPointer = 0x1A;


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
	uint8 table_size; /* unapproved */
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


static struct vbios {
	area_id			area;
	uint8*			memory;
	display_mode*	shared_info;
	struct {
		uint16		hsync_start;
		uint16		hsync_end;
		uint16		hsync_total;
		uint16		vsync_start;
		uint16		vsync_end;
		uint16		vsync_total;
	}				timings_common;

	uint16_t ReadWord(off_t address)
	{
		return memory[address] | memory[address + 1] << 8;
	}
} vbios;


/*!	This is reimplementation, Haiku uses BIOS call and gets most current panel
	info, we're, otherwise, digging in VBIOS memory and parsing VBT tables to
	get native panel timings. This will allow to get non-updated,
	PROM-programmed timings info when compensation mode is off on your machine.
*/
static bool
get_bios(void)
{
	static const uint64_t kVBIOSAddress = 0xc0000;
	static const int kVBIOSSize = 64 * 1024;
		// FIXME: is this the same for all cards?

	/* !!!DANGER!!!: mapping of BIOS using legacy location for now,
	hence, if panel mode will be set using info from VBT, it will
	be taken from primary card's VBIOS */
	vbios.area = map_physical_memory("VBIOS mapping", kVBIOSAddress,
		kVBIOSSize, B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA, (void**)&vbios.memory);

	if (vbios.area < 0)
		return false;

	TRACE((DEVICE_NAME ": mapping VBIOS: 0x%" B_PRIx64 " -> %p\n",
		kVBIOSAddress, vbios.memory));

	int vbtOffset = vbios.ReadWord(kVbtPointer);
	if ((vbtOffset + (int)sizeof(vbt_header)) >= kVBIOSSize) {
		TRACE((DEVICE_NAME": bad VBT offset : 0x%x\n", vbtOffset));
		delete_area(vbios.area);
		return false;
	}

	struct vbt_header* vbt = (struct vbt_header*)(vbios.memory + vbtOffset);
	if (memcmp(vbt->signature, "$VBT", 4) != 0) {
		TRACE((DEVICE_NAME": bad VBT signature: %20s\n", vbt->signature));
		delete_area(vbios.area);
		return false;
	}
	return true;
}


static bool
feed_shared_info(uint8* data)
{
	bool bogus = false;

	/* handle bogus h/vtotal values, if got such */
	if (vbios.timings_common.hsync_end > vbios.timings_common.hsync_total) {
		vbios.timings_common.hsync_total = vbios.timings_common.hsync_end + 1;
		bogus = true;
		TRACE((DEVICE_NAME": got bogus htotal. Fixing\n"));
	}
	if (vbios.timings_common.vsync_end > vbios.timings_common.vsync_total) {
		vbios.timings_common.vsync_total = vbios.timings_common.vsync_end + 1;
		bogus = true;
		TRACE((DEVICE_NAME": got bogus vtotal. Fixing\n"));
	}

	if (bogus) {
		TRACE((DEVICE_NAME": adjusted LFP modeline: x%d Hz,\t"
			"%d %d %d %d   %d %d %d %d\n",
			_PIXEL_CLOCK(data) / ((_H_ACTIVE(data) + _H_BLANK(data))
				* (_V_ACTIVE(data) + _V_BLANK(data))),
			_H_ACTIVE(data), vbios.timings_common.hsync_start,
			vbios.timings_common.hsync_end, vbios.timings_common.hsync_total,
			_V_ACTIVE(data), vbios.timings_common.vsync_start,
			vbios.timings_common.vsync_end, vbios.timings_common.vsync_total));
	}

	/* TODO: add retrieved info to edid info struct, not fixed mode struct */

	/* struct display_timing is not packed, so we have to set elements
	individually */
	vbios.shared_info->timing.pixel_clock = _PIXEL_CLOCK(data) / 1000;
	vbios.shared_info->timing.h_display = vbios.shared_info->virtual_width
		= _H_ACTIVE(data);
	vbios.shared_info->timing.h_sync_start = vbios.timings_common.hsync_start;
	vbios.shared_info->timing.h_sync_end = vbios.timings_common.hsync_end;
	vbios.shared_info->timing.h_total = vbios.timings_common.hsync_total;
	vbios.shared_info->timing.v_display = vbios.shared_info->virtual_height
		= _V_ACTIVE(data);
	vbios.shared_info->timing.v_sync_start = vbios.timings_common.vsync_start;
	vbios.shared_info->timing.v_sync_end = vbios.timings_common.vsync_end;
	vbios.shared_info->timing.v_total = vbios.timings_common.vsync_total;

	delete_area(vbios.area);
	return true;
}


bool
get_lvds_mode_from_bios(display_mode* sharedInfo)
{
	if (!get_bios())
		return false;

	int vbtOffset = vbios.ReadWord(kVbtPointer);
	struct vbt_header* vbt = (struct vbt_header*)(vbios.memory + vbtOffset);
	int bdbOffset = vbtOffset + vbt->bdb_offset;

	struct bdb_header* bdb = (struct bdb_header*)(vbios.memory + bdbOffset);
	if (memcmp(bdb->signature, "BIOS_DATA_BLOCK ", 16) != 0) {
		TRACE((DEVICE_NAME": bad BDB signature\n"));
		delete_area(vbios.area);
	}

	TRACE((DEVICE_NAME": parsing BDB blocks\n"));
	int blockSize;
	int panelType = -1;

	for (int bdbBlockOffset = bdb->header_size; bdbBlockOffset < bdb->bdb_size;
			bdbBlockOffset += blockSize) {
		int start = bdbOffset + bdbBlockOffset;

		int id = vbios.memory[start];
		blockSize = vbios.ReadWord(start + 1) + 3;
		// TRACE((DEVICE_NAME": found BDB block type %d\n", id));
		switch (id) {
			case 40: // FIXME magic numbers
			{
				struct lvds_bdb1 *lvds1;
				lvds1 = (struct lvds_bdb1 *)(vbios.memory + start);
				panelType = lvds1->panel_type;
				break;
			}
			case 41:
			{
				if (panelType == -1)
					break;
				struct lvds_bdb2 *lvds2;
				struct lvds_bdb2_lfp_info *lvds2_lfp_info;

				lvds2 = (struct lvds_bdb2 *)(vbios.memory + start);
				lvds2_lfp_info = (struct lvds_bdb2_lfp_info *)
					(vbios.memory + bdbOffset
					+ lvds2->panels[panelType].lfp_info_offset);
				/* found bad one terminator */
				if (lvds2_lfp_info->terminator != 0xffff) {
					delete_area(vbios.area);
					return false;
				}
				uint8_t* timing_data = vbios.memory + bdbOffset
					+ lvds2->panels[panelType].lfp_edid_dtd_offset;
				TRACE((DEVICE_NAME": found LFP of size %d x %d "
					"in BIOS VBT tables\n",
					lvds2_lfp_info->x_res, lvds2_lfp_info->y_res));

				vbios.timings_common.hsync_start = _H_ACTIVE(timing_data)
					+ _H_SYNC_OFF(timing_data);
				vbios.timings_common.hsync_end
					= vbios.timings_common.hsync_start
					+ _H_SYNC_WIDTH(timing_data);
				vbios.timings_common.hsync_total = _H_ACTIVE(timing_data)
					+ _H_BLANK(timing_data);
				vbios.timings_common.vsync_start = _V_ACTIVE(timing_data)
					+ _V_SYNC_OFF(timing_data);
				vbios.timings_common.vsync_end
					= vbios.timings_common.vsync_start
					+ _V_SYNC_WIDTH(timing_data);
				vbios.timings_common.vsync_total = _V_ACTIVE(timing_data)
					+ _V_BLANK(timing_data);

				vbios.shared_info = sharedInfo;
				return feed_shared_info(timing_data);
			}
		}
	}

	delete_area(vbios.area);
	return true;
}
