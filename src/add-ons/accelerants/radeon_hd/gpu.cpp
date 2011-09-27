/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "bios.h"
#include "gpu.h"
#include "utility.h"

#include <Debug.h>


#undef TRACE

#define TRACE_GPU
#ifdef TRACE_GPU
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


status_t
radeon_gpu_reset()
{
	radeon_shared_info &info = *gInfo->shared_info;

	// Read GRBM Command Processor status
	if (!(Read32(OUT, GRBM_STATUS) & GUI_ACTIVE))
		return B_ERROR;

	TRACE("%s: GPU software reset in progress...\n", __func__);

	// Halt memory controller
	radeon_gpu_mc_halt();

	if (radeon_gpu_mc_idlecheck() > 0) {
		ERROR("%s: Timeout waiting for MC to idle!\n", __func__);
	}

	if (info.device_chipset < RADEON_R1000) {
		Write32(OUT, CP_ME_CNTL, CP_ME_HALT);
			// Disable Command Processor parsing / prefetching

		// Register busy masks for early Radeon HD cards

		// GRBM Command Processor Status
		uint32 grbm_busy_mask = VC_BUSY;
			// Vertex Cache Busy
		grbm_busy_mask |= VGT_BUSY_NO_DMA | VGT_BUSY;
			// Vertex Grouper Tessellator Busy
		grbm_busy_mask |= TA03_BUSY;
			// unknown
		grbm_busy_mask |= TC_BUSY;
			// Texture Cache Busy
		grbm_busy_mask |= SX_BUSY;
			// Shader Export Busy
		grbm_busy_mask |= SH_BUSY;
			// Sequencer Instruction Cache Busy
		grbm_busy_mask |= SPI_BUSY;
			// Shader Processor Interpolator Busy
		grbm_busy_mask |= SMX_BUSY;
			// Shader Memory Exchange
		grbm_busy_mask |= SC_BUSY;
			// Scan Converter Busy
		grbm_busy_mask |= PA_BUSY;
			// Primitive Assembler Busy
		grbm_busy_mask |= DB_BUSY;
			// Depth Block Busy
		grbm_busy_mask |= CR_BUSY;
			// unknown
		grbm_busy_mask |= CB_BUSY;
			// Color Block Busy
		grbm_busy_mask |= GUI_ACTIVE;
			// unknown (graphics pipeline active?)

		// GRBM Command Processor Detailed Status
		uint32 grbm2_busy_mask = SPI0_BUSY | SPI1_BUSY | SPI2_BUSY | SPI3_BUSY;
			// Shader Processor Interpolator 0 - 3 Busy
		grbm2_busy_mask |= TA0_BUSY | TA1_BUSY | TA2_BUSY | TA3_BUSY;
			// unknown 0 - 3 Busy
		grbm2_busy_mask |= DB0_BUSY | DB1_BUSY | DB2_BUSY | DB3_BUSY;
			// Depth Block 0 - 3 Busy
		grbm2_busy_mask |= CB0_BUSY | CB1_BUSY | CB2_BUSY | CB3_BUSY;
			// Color Block 0 - 3 Busy

		uint32 tmp;
		/* Check if any of the rendering block is busy and reset it */
		if ((Read32(OUT, GRBM_STATUS) & grbm_busy_mask)
			|| (Read32(OUT, GRBM_STATUS2) & grbm2_busy_mask)) {
			tmp = SOFT_RESET_CR
				| SOFT_RESET_DB
				| SOFT_RESET_CB
				| SOFT_RESET_PA
				| SOFT_RESET_SC
				| SOFT_RESET_SMX
				| SOFT_RESET_SPI
				| SOFT_RESET_SX
				| SOFT_RESET_SH
				| SOFT_RESET_TC
				| SOFT_RESET_TA
				| SOFT_RESET_VC
				| SOFT_RESET_VGT;
			Write32(OUT, GRBM_SOFT_RESET, tmp);
			Read32(OUT, GRBM_SOFT_RESET);
			snooze(15000);
			Write32(OUT, GRBM_SOFT_RESET, 0);
		}

		// Reset CP
		tmp = SOFT_RESET_CP;
		Write32(OUT, GRBM_SOFT_RESET, tmp);
		Read32(OUT, GRBM_SOFT_RESET);
		snooze(15000);
		Write32(OUT, GRBM_SOFT_RESET, 0);

		// Let things settle
		snooze(1000);
	} else {
		// Northern Islands and higher

		Write32(OUT, CP_ME_CNTL, CP_ME_HALT | CP_PFP_HALT);
			// Disable Command Processor parsing / prefetching

		// reset the graphics pipeline components
		uint32 grbm_reset = (SOFT_RESET_CP
			| SOFT_RESET_CB
			| SOFT_RESET_DB
			| SOFT_RESET_GDS
			| SOFT_RESET_PA
			| SOFT_RESET_SC
			| SOFT_RESET_SPI
			| SOFT_RESET_SH
			| SOFT_RESET_SX
			| SOFT_RESET_TC
			| SOFT_RESET_TA
			| SOFT_RESET_VGT
			| SOFT_RESET_IA);

		Write32(OUT, GRBM_SOFT_RESET, grbm_reset);
		Read32(OUT, GRBM_SOFT_RESET);

		snooze(50);
		Write32(OUT, GRBM_SOFT_RESET, 0);
		Read32(OUT, GRBM_SOFT_RESET);
		snooze(50);
	}

	// Resume memory controller
	radeon_gpu_mc_resume();
	return B_OK;
}


void
radeon_gpu_mc_halt()
{
	// Backup current memory controller state
	gInfo->mc_info->d1vga_control = Read32(OUT, D1VGA_CONTROL);
	gInfo->mc_info->d2vga_control = Read32(OUT, D2VGA_CONTROL);
	gInfo->mc_info->vga_render_control = Read32(OUT, VGA_RENDER_CONTROL);
	gInfo->mc_info->vga_hdp_control = Read32(OUT, VGA_HDP_CONTROL);
	gInfo->mc_info->d1crtc_control = Read32(OUT, D1CRTC_CONTROL);
	gInfo->mc_info->d2crtc_control = Read32(OUT, D2CRTC_CONTROL);

	// halt all memory controller actions
	Write32(OUT, D2CRTC_UPDATE_LOCK, 0);
	Write32(OUT, VGA_RENDER_CONTROL, 0);
	Write32(OUT, D1CRTC_UPDATE_LOCK, 1);
	Write32(OUT, D2CRTC_UPDATE_LOCK, 1);
	Write32(OUT, D1CRTC_CONTROL, 0);
	Write32(OUT, D2CRTC_CONTROL, 0);
	Write32(OUT, D1CRTC_UPDATE_LOCK, 0);
	Write32(OUT, D2CRTC_UPDATE_LOCK, 0);
	Write32(OUT, D1VGA_CONTROL, 0);
	Write32(OUT, D2VGA_CONTROL, 0);
}


void
radeon_gpu_mc_resume()
{
	// TODO : do surface addresses disappear on mc halt?
	//Write32(OUT, D1GRPH_PRIMARY_SURFACE_ADDRESS, rdev->mc.vram_start);
	//Write32(OUT, D1GRPH_SECONDARY_SURFACE_ADDRESS, rdev->mc.vram_start);
	//Write32(OUT, D2GRPH_PRIMARY_SURFACE_ADDRESS, rdev->mc.vram_start);
	//Write32(OUT, D2GRPH_SECONDARY_SURFACE_ADDRESS, rdev->mc.vram_start);
	//Write32(OUT, VGA_MEMORY_BASE_ADDRESS, rdev->mc.vram_start);

	// Rnlock host access
	Write32(OUT, VGA_HDP_CONTROL, gInfo->mc_info->vga_hdp_control);
	snooze(1);

	// Restore memory controller state
	Write32(OUT, D1VGA_CONTROL, gInfo->mc_info->d1vga_control);
	Write32(OUT, D2VGA_CONTROL, gInfo->mc_info->d2vga_control);
	Write32(OUT, D1CRTC_UPDATE_LOCK, 1);
	Write32(OUT, D2CRTC_UPDATE_LOCK, 1);
	Write32(OUT, D1CRTC_CONTROL, gInfo->mc_info->d1crtc_control);
	Write32(OUT, D2CRTC_CONTROL, gInfo->mc_info->d2crtc_control);
	Write32(OUT, D1CRTC_UPDATE_LOCK, 0);
	Write32(OUT, D2CRTC_UPDATE_LOCK, 0);
	Write32(OUT, VGA_RENDER_CONTROL, gInfo->mc_info->vga_render_control);
}


uint32
radeon_gpu_mc_idlecheck()
{
	uint32 idleStatus;
	if (!((idleStatus = Read32(MC, SRBM_STATUS)) &
		(VMC_BUSY | MCB_BUSY |
			MCDZ_BUSY | MCDY_BUSY | MCDX_BUSY | MCDW_BUSY)))
		return 0;

	return idleStatus;
}


status_t
radeon_gpu_mc_setup()
{
	uint32 fb_location_int = gInfo->shared_info->frame_buffer_int;

	uint32 fb_location = Read32(OUT, R600_MC_VM_FB_LOCATION);
	uint16 fb_size = (fb_location >> 16) - (fb_location & 0xFFFF);
	uint32 fb_location_tmp = fb_location_int >> 24;
	fb_location_tmp |= (fb_location_tmp + fb_size) << 16;
	uint32 fb_offset_tmp = (fb_location_int >> 8) & 0xff0000;

	radeon_gpu_mc_halt();

	uint32 idleState = radeon_gpu_mc_idlecheck();
	if (idleState > 0) {
		TRACE("%s: Cannot modify non-idle MC! idleState: 0x%" B_PRIX32 "\n",
			__func__, idleState);
		return B_ERROR;
	}

	TRACE("%s: Setting frame buffer from 0x%" B_PRIX32
		" to 0x%" B_PRIX32 " [size 0x%" B_PRIX16 "]\n",
		__func__, fb_location, fb_location_tmp, fb_size);

	// The MC Write32 will handle cards needing a special MC read/write register
	Write32(MC, R600_MC_VM_FB_LOCATION, fb_location_tmp);
	Write32(MC, R600_HDP_NONSURFACE_BASE, fb_offset_tmp);

	radeon_gpu_mc_resume();

	return B_OK;
}


status_t
radeon_gpu_irq_setup()
{
	// TODO : Stub for IRQ setup

	// allocate rings via r600_ih_ring_alloc

	// disable irq's via r600_disable_interrupts

	// r600_rlc_init

	// setup interrupt control

	return B_ERROR;
}


static void
lock_i2c(void* cookie, bool lock)
{
	gpio_info *info = (gpio_info*)cookie;
	radeon_shared_info &sinfo = *gInfo->shared_info;

	uint32 buffer = 0;

	if (lock == true) {
		// hw_capable and > DCE3
		if (info->hw_capable == true
			&& sinfo.device_chipset >= (RADEON_R600 | 0x20)) {
			// Switch GPIO pads to ddc mode
			buffer = Read32(OUT, info->mask_scl_reg);
			buffer &= ~(1 << 16);
			Write32(OUT, info->mask_scl_reg, buffer);
		}

		// Clear pins
		buffer = Read32(OUT, info->a_scl_reg) & ~info->a_scl_mask;
		Write32(OUT, info->a_scl_reg, buffer);
		buffer = Read32(OUT, info->a_sda_reg) & ~info->a_sda_mask;
		Write32(OUT, info->a_sda_reg, buffer);
	}

	// Set pins to input
	buffer = Read32(OUT, info->en_scl_reg) & ~info->en_scl_mask;
	Write32(OUT, info->en_scl_reg, buffer);
	buffer = Read32(OUT, info->en_sda_reg) & ~info->en_sda_mask;
	Write32(OUT, info->en_sda_reg, buffer);

	// mask GPIO pins for software use
	buffer = Read32(OUT, info->mask_scl_reg);
	if (lock == true) {
		buffer |= info->mask_scl_mask;
	} else {
		buffer &= ~info->mask_scl_mask;
	}
	Write32(OUT, info->mask_scl_reg, buffer);
	Read32(OUT, info->mask_scl_reg);

	buffer = Read32(OUT, info->mask_sda_reg);
	if (lock == true) {
		buffer |= info->mask_sda_mask;
	} else {
		buffer &= ~info->mask_sda_mask;
	}
	Write32(OUT, info->mask_sda_reg, buffer);
	Read32(OUT, info->mask_sda_reg);
}


static status_t
get_i2c_signals(void* cookie, int* _clock, int* _data)
{
	gpio_info *info = (gpio_info*)cookie;

	uint32 scl = Read32(OUT, info->y_scl_reg);
	scl &= info->y_scl_mask;
	uint32 sda = Read32(OUT, info->y_sda_reg);
	sda &= info->y_sda_mask;

	*_clock = (scl != 0);
	*_data = (sda != 0);

	return B_OK;
}


static status_t
set_i2c_signals(void* cookie, int clock, int data)
{
	gpio_info* info = (gpio_info*)cookie;

	uint32 scl = Read32(OUT, info->en_scl_reg)
		& ~info->en_scl_mask;
	scl |= clock ? 0 : info->en_scl_mask;
	Write32(OUT, info->en_scl_reg, scl);
	Read32(OUT, info->en_scl_reg);

	uint32 sda = Read32(OUT, info->en_sda_reg)
		& ~info->en_sda_mask;
	sda |= data ? 0 : info->en_sda_mask;
	Write32(OUT, info->en_sda_reg, sda);
	Read32(OUT, info->en_sda_reg);

	return B_OK;
}


bool
radeon_gpu_read_edid(uint32 connector, edid1_info *edid)
{
	// ensure things are sane
	uint32 gpio_id = gConnector[connector]->connector_gpio_id;
	if (gGPIOInfo[gpio_id]->valid == false)
		return false;

	i2c_bus bus;

	ddc2_init_timing(&bus);
	//bus.cookie = (void*)&gConnector[connector]->connector_gpio;
	bus.cookie = (void*)gGPIOInfo[gpio_id];
	bus.set_signals = &set_i2c_signals;
	bus.get_signals = &get_i2c_signals;

	lock_i2c(bus.cookie, true);
	status_t edid_result = ddc2_read_edid1(&bus, edid, NULL, NULL);
	lock_i2c(bus.cookie, false);

	if (edid_result != B_OK)
		return false;

	TRACE("%s: found edid monitor on connector #%" B_PRId32 "\n",
		__func__, connector);

	return true;
}


status_t
radeon_gpu_i2c_attach(uint32 id, uint8 hw_line)
{
	gConnector[id]->connector_gpio_id = 0;
	for (uint32 i = 0; i < ATOM_MAX_SUPPORTED_DEVICE; i++) {
		if (gGPIOInfo[i]->hw_line != hw_line)
			continue;
		gConnector[id]->connector_gpio_id = i;
		return B_OK;
	}

	TRACE("%s: couldn't find GPIO for connector %" B_PRIu32 "\n",
		__func__, id);
	return B_ERROR;
}


status_t
radeon_gpu_gpio_setup()
{
	int index = GetIndexIntoMasterTable(DATA, GPIO_I2C_Info);
	uint8 frev;
	uint8 crev;
	uint16 offset;
	uint16 size;

	if (atom_parse_data_header(gAtomContext, index, &size, &frev, &crev,
		&offset) != B_OK) {
		ERROR("%s: could't read GPIO_I2C_Info table from AtomBIOS index %d!\n",
			__func__, index);
		return B_ERROR;
	}

	struct _ATOM_GPIO_I2C_INFO *i2c_info
		= (struct _ATOM_GPIO_I2C_INFO *)(gAtomContext->bios + offset);

	uint32 num_indices = (size - sizeof(ATOM_COMMON_TABLE_HEADER))
		/ sizeof(ATOM_GPIO_I2C_ASSIGMENT);

	if (num_indices > ATOM_MAX_SUPPORTED_DEVICE) {
		ERROR("%s: ERROR: AtomBIOS contains more GPIO_Info items then I"
			"was prepared for! (seen: %" B_PRIu32 "; max: %" B_PRIu32 ")\n",
			__func__, num_indices, (uint32)ATOM_MAX_SUPPORTED_DEVICE);
		return B_ERROR;
	}

	for (uint32 i = 0; i < num_indices; i++) {
		ATOM_GPIO_I2C_ASSIGMENT *gpio = &i2c_info->asGPIO_Info[i];

		// TODO : if DCE 4 and i == 7 ... manual override for evergreen
		// TODO : if DCE 3 and i == 4 ... manual override

		// populate gpio information
		gGPIOInfo[i]->hw_line
			= gpio->sucI2cId.ucAccess;
		gGPIOInfo[i]->hw_capable
			= (gpio->sucI2cId.sbfAccess.bfHW_Capable) ? true : false;

		// GPIO mask (Allows software to control the GPIO pad)
		// 0 = chip access; 1 = only software;
		gGPIOInfo[i]->mask_scl_reg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkMaskRegisterIndex) * 4;
		gGPIOInfo[i]->mask_sda_reg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataMaskRegisterIndex) * 4;
		gGPIOInfo[i]->mask_scl_mask
			= (1 << gpio->ucClkMaskShift);
		gGPIOInfo[i]->mask_sda_mask
			= (1 << gpio->ucDataMaskShift);

		// GPIO output / write (A) enable
		// 0 = GPIO input (Y); 1 = GPIO output (A);
		gGPIOInfo[i]->en_scl_reg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkEnRegisterIndex) * 4;
		gGPIOInfo[i]->en_sda_reg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataEnRegisterIndex) * 4;
		gGPIOInfo[i]->en_scl_mask
			= (1 << gpio->ucClkEnShift);
		gGPIOInfo[i]->en_sda_mask
			= (1 << gpio->ucDataEnShift);

		// GPIO output / write (A)
		gGPIOInfo[i]->a_scl_reg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkA_RegisterIndex) * 4;
		gGPIOInfo[i]->a_sda_reg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataA_RegisterIndex) * 4;
		gGPIOInfo[i]->a_scl_mask
			= (1 << gpio->ucClkA_Shift);
		gGPIOInfo[i]->a_sda_mask
			= (1 << gpio->ucDataA_Shift);

		// GPIO input / read (Y)
		gGPIOInfo[i]->y_scl_reg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkY_RegisterIndex) * 4;
		gGPIOInfo[i]->y_sda_reg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataY_RegisterIndex) * 4;
		gGPIOInfo[i]->y_scl_mask
			= (1 << gpio->ucClkY_Shift);
		gGPIOInfo[i]->y_sda_mask
			= (1 << gpio->ucDataY_Shift);

		// ensure data is valid
		gGPIOInfo[i]->valid = (gGPIOInfo[i]->mask_scl_reg) ? true : false;

		TRACE("%s: GPIO @ %" B_PRIu32 ", valid: %s, hw_line: 0x%" B_PRIX32 "\n",
			__func__, i, gGPIOInfo[i]->valid ? "true" : "false",
			gGPIOInfo[i]->hw_line);
	}

	return B_OK;
}
