/* Written by Artem Falcon <lomka@gero.in> */

#include <KernelExport.h>
#include "intel_extreme.h"
#include <string.h>

#define TRACE_BIOS 1 
#ifdef TRACE_BIOS
#       define TRACE(x) dprintf x 
#else 
#	define TRACE(x) ;
#endif

/* for moving into intel_extreme_private.h */
#define VBIOS 0xc0000
#define INTEL_VBIOS_SIZE (64 * 1024) /* XXX */

#define INTEL_BIOS_16(_addr) (vbios.memory[_addr] | \
        (vbios.memory[_addr + 1] << 8))
/* */

/* subject of including into private/graphics/common/edid* */ 
#define _PIXEL_CLOCK_MHZ(x) (x[0] + (x[1] << 8)) / 100

#define _PIXEL_CLOCK(x) (x[0] + (x[1] << 8)) * 10000
#define _H_ACTIVE(x) (x[2] + ((x[4] & 0xF0) << 4))
#define _H_BLANK(x) (x[3] + ((x[4] & 0x0F) << 8))
#define _H_SYNC_OFF(x) (x[8] + ((x[11] & 0xC0) << 2))
#define _H_SYNC_WIDTH(x) (x[9] + ((x[11] & 0x30) << 4))
#define _V_ACTIVE(x) (x[5] + ((x[7] & 0xF0) << 4))
#define _V_BLANK(x) (x[6] + ((x[7] & 0x0F) << 8))
#define _V_SYNC_OFF(x) ((x[10] >> 4) + ((x[11] & 0x0C) << 2))
#define _V_SYNC_WIDTH(x) ((x[10] & 0x0F) + ((x[11] & 0x03) << 4))
/* */

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
	/* cutted */
} __attribute__((packed));

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
	area_id area;
	uint8* memory;
	display_mode *shared_info;
	struct {
		uint16 hsync_start;
		uint16 hsync_end;
		uint16 hsync_total;
		uint16 vsync_start;
		uint16 vsync_end;
		uint16 vsync_total;
	} timings_common;
} vbios;

static inline bool unmap_bios(area_id area, uint8* mem) {
        delete_area(area);
        mem = NULL;

	return false;
}

/* TO-DO: move to accelerant code, if possible */

/* this is reimplementation, Haiku uses BIOS call and gets most
 * current panel info, we're, otherwise, digging in VBIOS memory
 * and parsing VBT tables to get native panel timings. This will
 * allow to get non-updated, PROM-programmed timings info when
 * compensation mode is off on your machine */
static bool
get_bios(void)
{
	int vbt_offset;
	struct vbt_header *vbt;

	/* !!!DANGER!!!: mapping of BIOS using legacy location for now,
	 * hence, if panel mode will be set using info from VBT, it will
	 * be taken from primary card's VBIOS */
	vbios.area = map_physical_memory("VBIOS mapping",
		VBIOS, INTEL_VBIOS_SIZE, B_ANY_KERNEL_ADDRESS,
		B_READ_AREA, (void**)&(vbios.memory));

	if (vbios.area < 0)
		return false;

	TRACE((DEVICE_NAME ": mapping VBIOS: 0x%x -> %p\n",
		VBIOS, vbios.memory));

	vbt_offset = INTEL_BIOS_16(0x1a);
	if (vbt_offset >= INTEL_VBIOS_SIZE) {
		TRACE(("intel_extreme: bad VBT offset : 0x%x\n",
			vbt_offset));
		return unmap_bios(vbios.area, vbios.memory);
	}

	vbt = (struct vbt_header *)(vbios.memory + vbt_offset);
	if (memcmp(vbt->signature, "$VBT", 4) != 0) {
		TRACE(("intel_extreme: bad VBT signature: %20s\n",
			vbt->signature));
		return unmap_bios(vbios.area, vbios.memory);
	}

	return true;
}

static bool feed_shared_info(uint8* data)
{
	bool bogus = false;	

	/* handle bogus h/vtotal values, if got such */
	if (vbios.timings_common.hsync_end > vbios.timings_common.hsync_total) {
		vbios.timings_common.hsync_total =
			vbios.timings_common.hsync_end + 1;
		bogus = true;
		TRACE(("intel_extreme: got bogus htotal. Fixing\n"));
	}
	if (vbios.timings_common.vsync_end > vbios.timings_common.vsync_total) {
		vbios.timings_common.vsync_total =
			vbios.timings_common.vsync_end + 1;
		bogus = true;
		TRACE(("intel_extreme: got bogus vtotal. Fixing\n"));
	}
	/* */
	if (bogus)
		TRACE(("intel_extreme: adjusted LFP modeline: x%d Hz,\t%d "
			"%d %d %d %d   %d %d %d %d\n",
				_PIXEL_CLOCK(data) / (
					(_H_ACTIVE(data) + _H_BLANK(data))
				      * (_V_ACTIVE(data) + _V_BLANK(data))
					),
				_PIXEL_CLOCK_MHZ(data),
				_H_ACTIVE(data), vbios.timings_common.hsync_start,
				vbios.timings_common.hsync_end, vbios.timings_common.hsync_total,
				_V_ACTIVE(data), vbios.timings_common.vsync_start,
				vbios.timings_common.vsync_end, vbios.timings_common.vsync_total
		));

	/* TO-DO: add retrieved info to edid info struct, not fixed mode struct */
	
	/* struct display_timing is not packed, so we're end setting of every single elm of it */
	vbios.shared_info->timing.pixel_clock = _PIXEL_CLOCK(data) / 1000;
	vbios.shared_info->timing.h_display = vbios.shared_info->virtual_width = _H_ACTIVE(data);
	vbios.shared_info->timing.h_sync_start = vbios.timings_common.hsync_start;
	vbios.shared_info->timing.h_sync_end = vbios.timings_common.hsync_end;
	vbios.shared_info->timing.h_total = vbios.timings_common.hsync_total;
	vbios.shared_info->timing.v_display = vbios.shared_info->virtual_height = _V_ACTIVE(data);
	vbios.shared_info->timing.v_sync_start = vbios.timings_common.vsync_start;
	vbios.shared_info->timing.v_sync_end = vbios.timings_common.vsync_end;
	vbios.shared_info->timing.v_total = vbios.timings_common.vsync_total;

	unmap_bios(vbios.area, vbios.memory);
	return true;
}

bool get_lvds_mode_from_bios(display_mode *shared_info) 
{
	struct vbt_header *vbt;
	struct bdb_header *bdb;
	int vbt_offset, bdb_offset, bdb_block_offset, block_size;
	int panel_type = -1;

        if (!get_bios())
                return false;

	vbt_offset = INTEL_BIOS_16(0x1a);
	vbt = (struct vbt_header *)(vbios.memory + vbt_offset);
	bdb_offset = vbt_offset + vbt->bdb_offset;
	
	bdb = (struct bdb_header *)(vbios.memory + bdb_offset);
	if (memcmp(bdb->signature, "BIOS_DATA_BLOCK ", 16) != 0) {
		TRACE(("intel_extreme: bad BDB signature\n"));
		return unmap_bios(vbios.area, vbios.memory);
	}

	TRACE(("intel_extreme: parsing BDB blocks\n"));
	for (bdb_block_offset = bdb->header_size; bdb_block_offset < bdb->bdb_size;
		bdb_block_offset += block_size) {
		int start = bdb_offset + bdb_block_offset;
		int id;
		struct lvds_bdb1 *lvds1;
		struct lvds_bdb2 *lvds2;
		struct lvds_bdb2_lfp_info *lvds2_lfp_info;
		uint8_t *timing_data;

		id = vbios.memory[start];
		block_size = INTEL_BIOS_16(start + 1) + 3;
		//TRACE(("intel_extreme: found BDB block type %d\n", id));
		switch (id) {
			case 40:
				lvds1 = (struct lvds_bdb1 *)(vbios.memory + start);
				panel_type = lvds1->panel_type;
			break;
			case 41:
				if (panel_type == -1)
					break;
				lvds2 = (struct lvds_bdb2 *)(vbios.memory + start);
				lvds2_lfp_info = (struct lvds_bdb2_lfp_info *)
					(vbios.memory + bdb_offset +
					lvds2->panels[panel_type].lfp_info_offset);
				/* found bad one terminator */
				if (lvds2_lfp_info->terminator != 0xffff) {
					return unmap_bios(vbios.area, vbios.memory);
				}
				timing_data = vbios.memory + bdb_offset +
					lvds2->panels[panel_type].lfp_edid_dtd_offset;
				TRACE(("intel_extreme: found LFP of size %d x %d "
					"in BIOS VBT tables\n",
					lvds2_lfp_info->x_res, lvds2_lfp_info->y_res));

				vbios.timings_common.hsync_start = _H_ACTIVE(timing_data) +
					_H_SYNC_OFF(timing_data);
				vbios.timings_common.hsync_end = vbios.timings_common.hsync_start
					+ _H_SYNC_WIDTH(timing_data);
				vbios.timings_common.hsync_total = _H_ACTIVE(timing_data) +
					_H_BLANK(timing_data);
				vbios.timings_common.vsync_start = _V_ACTIVE(timing_data) +
					_V_SYNC_OFF(timing_data);
				vbios.timings_common.vsync_end = vbios.timings_common.vsync_start
                                        + _V_SYNC_WIDTH(timing_data);
				vbios.timings_common.vsync_total = _V_ACTIVE(timing_data) +
                                        _V_BLANK(timing_data);

				/* Xfree86 compatible modeline */
				/*TRACE(("intel_extreme: LFP modeline: x%d Hz,\t%d "
					"%d %d %d %d   %d %d %d %d\n",
					_PIXEL_CLOCK(timing_data) / (
						(_H_ACTIVE(timing_data) + _H_BLANK(timing_data))
					      * (_V_ACTIVE(timing_data) + _V_BLANK(timing_data))
						),
					_PIXEL_CLOCK_MHZ(timing_data),
					_H_ACTIVE(timing_data), vbios.timings_common.hsync_start,
					vbios.timings_common.hsync_end, vbios.timings_common.hsync_total,
					_V_ACTIVE(timing_data), vbios.timings_common.vsync_start,
					vbios.timings_common.vsync_end, vbios.timings_common.vsync_total
				));*/

				vbios.shared_info = shared_info;
				return feed_shared_info(timing_data);
			break;
		}
	}

	unmap_bios(vbios.area, vbios.memory);
        return true;
}
