/*
 * i2c interface.
 * Bus should be run at max. 100kHz: see original Philips I2C specification
 *	
 * Rudolf Cornelissen 12/2002-10/2009
 */

#define MODULE_BIT 0x00004000

#include "nv_std.h"

static void i2c_DumpSpecsEDID(edid_specs* specs);

char i2c_flag_error (char ErrNo)
//error code list:
//0 - OK status
//1 - SCL locked low by device (bus is still busy)
//2 - SDA locked low by device (bus is still busy)
//3 - No Acknowledge from device (no handshake)
//4 - SDA not released for master to generate STOP bit
{
	static char I2CError = 0;

	if (!I2CError) I2CError = ErrNo;
	if (ErrNo == -1) I2CError = 0;
	return I2CError;
}

static void i2c_select_bus_set(bool set)
{
	/* I/O pins set selection is only valid on dualhead cards */
	if (!si->ps.secondary_head) return;

	/* select GPU I/O pins set to connect to I2C 'registers' */
	if (set) {
		/* this setup wires the 'I2C registers' to unknown I/O pins on the GPU? */
		NV_REG32(NV32_FUNCSEL) &= ~0x00000010;
		NV_REG32(NV32_2FUNCSEL) |= 0x00000010;
	} else {
		/* this setup wires the 'I2C registers' to the I2C buses */
		NV_REG32(NV32_2FUNCSEL) &= ~0x00000010;
		NV_REG32(NV32_FUNCSEL) |= 0x00000010;
	}
}

static void OutSCL(uint8 BusNR, bool Bit)
{
	uint8 data;

	switch (BusNR) {
	case 0:
		data = (CRTCR(WR_I2CBUS_0) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_0, (data | 0x20));
		else
			CRTCW(WR_I2CBUS_0, (data & ~0x20));
		break;
	case 1:
		data = (CRTCR(WR_I2CBUS_1) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_1, (data | 0x20));
		else
			CRTCW(WR_I2CBUS_1, (data & ~0x20));
		break;
	case 2:
		data = (CRTCR(WR_I2CBUS_2) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_2, (data | 0x20));
		else
			CRTCW(WR_I2CBUS_2, (data & ~0x20));
		break;
	}
}

static void OutSDA(uint8 BusNR, bool Bit)
{
	uint8 data;
	
	switch (BusNR) {
	case 0:
		data = (CRTCR(WR_I2CBUS_0) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_0, (data | 0x10));
		else
			CRTCW(WR_I2CBUS_0, (data & ~0x10));
		break;
	case 1:
		data = (CRTCR(WR_I2CBUS_1) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_1, (data | 0x10));
		else
			CRTCW(WR_I2CBUS_1, (data & ~0x10));
		break;
	case 2:
		data = (CRTCR(WR_I2CBUS_2) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_2, (data | 0x10));
		else
			CRTCW(WR_I2CBUS_2, (data & ~0x10));
		break;
	}
}

static bool InSCL(uint8 BusNR)
{
	switch (BusNR) {
	case 0:
		if ((CRTCR(RD_I2CBUS_0) & 0x04)) return true;
		break;
	case 1:
		if ((CRTCR(RD_I2CBUS_1) & 0x04)) return true;
		break;
	case 2:
		if ((CRTCR(RD_I2CBUS_2) & 0x04)) return true;
		break;
	}

	return false;
}

static bool InSDA(uint8 BusNR)
{
	switch (BusNR) {
	case 0:
		if ((CRTCR(RD_I2CBUS_0) & 0x08)) return true;
		break;
	case 1:
		if ((CRTCR(RD_I2CBUS_1) & 0x08)) return true;
		break;
	case 2:
		if ((CRTCR(RD_I2CBUS_2) & 0x08)) return true;
		break;
	}

	return false;
}

static void TXBit (uint8 BusNR, bool Bit)
{
	/* send out databit */
	if (Bit) {
		OutSDA(BusNR, true);
		snooze(3);
		if (!InSDA(BusNR)) i2c_flag_error (2);
	} else {
		OutSDA(BusNR, false);
	}
	/* generate clock pulse */
	snooze(6);
	OutSCL(BusNR, true);
	snooze(3);
	if (!InSCL(BusNR)) i2c_flag_error (1);
	snooze(6);
	OutSCL(BusNR, false);
	snooze(6);
}

static uint8 RXBit (uint8 BusNR)
{
	uint8 Bit = 0;

	/* set SDA so input is possible */
	OutSDA(BusNR, true);
	/* generate clock pulse */
	snooze(6);
	OutSCL(BusNR, true);
	snooze(3);
	if (!InSCL(BusNR)) i2c_flag_error (1);
	snooze(3);
	/* read databit */
	if (InSDA(BusNR)) Bit = 1;
	/* finish clockpulse */
	OutSCL(BusNR, false);
	snooze(6);

	return Bit;
}

void i2c_bstart (uint8 BusNR)
{
	/* enable access to primary head */
	set_crtc_owner(0);

	/* make sure SDA is high */
	OutSDA(BusNR, true);
	snooze(3);
	OutSCL(BusNR, true);
	snooze(3);
	if (!InSCL(BusNR)) i2c_flag_error (1);
	snooze(6);
	/* clear SDA while SCL set (bus-start condition) */
	OutSDA(BusNR, false);
	snooze(6);
	OutSCL(BusNR, false);
	snooze(6);

	LOG(4,("I2C: START condition generated on bus %d; status is %d\n",
		BusNR, i2c_flag_error (0)));
}

void i2c_bstop (uint8 BusNR)
{
	/* enable access to primary head */
	set_crtc_owner(0);

	/* make sure SDA is low */
	OutSDA(BusNR, false);
	snooze(3);
	OutSCL(BusNR, true);
	snooze(3);
	if (!InSCL(BusNR)) i2c_flag_error (1);
	snooze(6);
	/* set SDA while SCL set (bus-stop condition) */
	OutSDA(BusNR, true);
	snooze(3);
	if (!InSDA(BusNR)) i2c_flag_error (4);
	snooze(3);

	LOG(4,("I2C: STOP condition generated on bus %d; status is %d\n",
		BusNR, i2c_flag_error (0)));
}

uint8 i2c_readbyte(uint8 BusNR, bool Ack)
{
	uint8 cnt, bit, byte = 0;

	/* enable access to primary head */
	set_crtc_owner(0);

	/* read data */
	for (cnt = 8; cnt > 0; cnt--) {
		byte <<= 1;
		bit = RXBit (BusNR);
		byte += bit;
	}
	/* send acknowledge */
	TXBit (BusNR, Ack);

	LOG(4,("I2C: read byte ($%02x) from bus #%d; status is %d\n",
		byte, BusNR, i2c_flag_error(0)));

	return byte;
}

bool i2c_writebyte (uint8 BusNR, uint8 byte)
{
	uint8 cnt;
	bool bit;
	uint8 tmp = byte;

	/* enable access to primary head */
	set_crtc_owner(0);

	/* write data */
	for (cnt = 8; cnt > 0; cnt--) {
		bit = (tmp & 0x80);
		TXBit (BusNR, bit);
		tmp <<= 1;
	}
	/* read acknowledge */
	bit = RXBit (BusNR);
	if (bit) i2c_flag_error (3);

	LOG(4,("I2C: written byte ($%02x) to bus #%d; status is %d\n",
		byte, BusNR, i2c_flag_error(0)));

	return bit;
}

void i2c_readbuffer (uint8 BusNR, uint8* buf, uint8 size)
{
	uint8 cnt;

	for (cnt = 0; cnt < size; cnt++)
		buf[cnt] = i2c_readbyte(BusNR, buf[cnt]);
}

void i2c_writebuffer (uint8 BusNR, uint8* buf, uint8 size)
{
	uint8 cnt;

	for (cnt = 0; cnt < size; cnt++)
		i2c_writebyte(BusNR, buf[cnt]);
}

status_t i2c_init(void)
{
	uint8 bus, buses;
	bool *i2c_bus = &(si->ps.i2c_bus0);
	status_t result = B_ERROR;

	LOG(4,("I2C: searching for wired I2C buses...\n"));

	/* select GPU I/O pins for I2C buses */
	i2c_select_bus_set(false);

	/* enable access to primary head */
	set_crtc_owner(0);

	/* on some NV40 architecture cards the i2c busses can be disabled: enable them */
	if (si->ps.card_arch == NV40A)
		CRTCW(I2C_LOCK ,(CRTCR(I2C_LOCK) | 0x04));

	/* preset no board wired buses */
	si->ps.i2c_bus0 = false;
	si->ps.i2c_bus1 = false;
	si->ps.i2c_bus2 = false;

	/* set number of buses to test for */
	buses = 2;

	/* newer cards (can) have a third bus.. */
	if (((si->ps.card_arch == NV10A) && (si->ps.card_type >= NV17)) || (si->ps.card_arch >= NV30A))
		buses = 3;

	/* find existing buses */
	for (bus = 0; bus < buses; bus++) {
		/* reset status */
		i2c_flag_error (-1);
		snooze(6);
		/* init and/or stop I2C bus */
		i2c_bstop(bus);
		/* check for hardware coupling of SCL and SDA -out and -in lines */
		snooze(6);
		OutSCL(bus, false);
		snooze(3);
		OutSDA(bus, true);
		snooze(3);
		if (InSCL(bus) || !InSDA(bus)) continue;
		snooze(3);
		OutSCL(bus, true);
		snooze(3);
		OutSDA(bus, false);
		snooze(3);
		if (!InSCL(bus) || InSDA(bus)) continue;
		i2c_bus[bus] = true;
		snooze(3);
		/* re-init bus */
		i2c_bstop(bus);
	}

	for (bus = 0; bus < buses; bus++) {
		if (i2c_bus[bus]) {
			LOG(4,("I2C: bus #%d wiring check: passed\n", bus));
			result = B_OK;
		} else {
			LOG(4,("I2C: bus #%d wiring check: failed\n", bus));
		}
	}

	i2c_DetectScreens();
	LOG(4,("I2C: dumping EDID specs for connector 1:\n"));
	i2c_DumpSpecsEDID(&si->ps.con1_screen);
	LOG(4,("I2C: dumping EDID specs for connector 2:\n"));
	i2c_DumpSpecsEDID(&si->ps.con2_screen);

	return result;
}

/*** DDC/EDID library use ***/
typedef struct {
	uint8 port;
} ddc_port_info;

/* Dump EDID info in driver's logfile */
static void
i2c_DumpEDID(edid1_info *edid)
{
	int i, j;

	LOG(4,("Vendor: %s\n", edid->vendor.manufacturer));
	LOG(4,("Product ID: %d\n", (int)edid->vendor.prod_id));
	LOG(4,("Serial #: %d\n", (int)edid->vendor.serial));
	LOG(4,("Produced in week/year: %d/%d\n", edid->vendor.week, edid->vendor.year));

	LOG(4,("EDID version: %d.%d\n", edid->version.version, edid->version.revision));

	LOG(4,("Type: %s\n", edid->display.input_type ? "Digital" : "Analog"));
	LOG(4,("Size: %d cm x %d cm\n", edid->display.h_size, edid->display.v_size));
	LOG(4,("Gamma=%.3f\n", (edid->display.gamma + 100) / 100.0));
	LOG(4,("White (X,Y)=(%.3f,%.3f)\n", edid->display.white_x / 1024.0,
		edid->display.white_y / 1024.0));

	LOG(4,("Supported Future Video Modes:\n"));
	for (i = 0; i < EDID1_NUM_STD_TIMING; ++i) {
		if (edid->std_timing[i].h_size <= 256)
			continue;

		LOG(4,("%dx%d@%dHz (id=%d)\n", 
			edid->std_timing[i].h_size, edid->std_timing[i].v_size,
			edid->std_timing[i].refresh, edid->std_timing[i].id));
	}

	LOG(4,("Supported VESA Video Modes:\n"));
	if (edid->established_timing.res_720x400x70)
		LOG(4,("720x400@70\n"));
	if (edid->established_timing.res_720x400x88)
		LOG(4,("720x400@88\n"));
	if (edid->established_timing.res_640x480x60)
		LOG(4,("640x480@60\n"));
	if (edid->established_timing.res_640x480x67)
		LOG(4,("640x480x67\n"));
	if (edid->established_timing.res_640x480x72)
		LOG(4,("640x480x72\n"));
	if (edid->established_timing.res_640x480x75)
		LOG(4,("640x480x75\n"));
	if (edid->established_timing.res_800x600x56)
		LOG(4,("800x600@56\n"));
	if (edid->established_timing.res_800x600x60)
		LOG(4,("800x600@60\n"));

	if (edid->established_timing.res_800x600x72)
		LOG(4,("800x600@72\n"));
	if (edid->established_timing.res_800x600x75)
		LOG(4,("800x600@75\n"));
	if (edid->established_timing.res_832x624x75)
		LOG(4,("832x624@75\n"));
	if (edid->established_timing.res_1024x768x87i)
		LOG(4,("1024x768@87 interlaced\n"));
	if (edid->established_timing.res_1024x768x60)
		LOG(4,("1024x768@60\n"));
	if (edid->established_timing.res_1024x768x70)
		LOG(4,("1024x768@70\n"));
	if (edid->established_timing.res_1024x768x75)
		LOG(4,("1024x768@75\n"));
	if (edid->established_timing.res_1280x1024x75)
		LOG(4,("1280x1024@75\n"));

	if (edid->established_timing.res_1152x870x75)
		LOG(4,("1152x870@75\n"));

	for (i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i) {
		edid1_detailed_monitor *monitor = &edid->detailed_monitor[i];

		switch(monitor->monitor_desc_type) {
			case EDID1_SERIAL_NUMBER:
				LOG(4,("Serial Number: %s\n", monitor->data.serial_number));
				break;

			case EDID1_ASCII_DATA:
				LOG(4,("Ascii Data: %s\n", monitor->data.ascii_data));
				break;

			case EDID1_MONITOR_RANGES:
			{
				edid1_monitor_range monitor_range = monitor->data.monitor_range;

				LOG(4,("Horizontal frequency range = %d..%d kHz\n",
					monitor_range.min_h, monitor_range.max_h));
				LOG(4,("Vertical frequency range = %d..%d Hz\n",
					monitor_range.min_v, monitor_range.max_v));
				LOG(4,("Maximum pixel clock = %d MHz\n", (uint16)monitor_range.max_clock * 10));
				break;
			}

			case EDID1_MONITOR_NAME:
				LOG(4,("Monitor Name: %s\n", monitor->data.monitor_name));
				break;

			case EDID1_ADD_COLOUR_POINTER:
			{
				for (j = 0; j < EDID1_NUM_EXTRA_WHITEPOINTS; ++j) {
					edid1_whitepoint *whitepoint = &monitor->data.whitepoint[j];

					if (whitepoint->index == 0)
						continue;

					LOG(4,("Additional whitepoint: (X,Y)=(%f,%f) gamma=%f index=%i\n",
						whitepoint->white_x / 1024.0, 
						whitepoint->white_y / 1024.0, 
						(whitepoint->gamma + 100) / 100.0, 
						whitepoint->index));
				}
				break;
			}

			case EDID1_ADD_STD_TIMING:
			{		
				for (j = 0; j < EDID1_NUM_EXTRA_STD_TIMING; ++j) {
					edid1_std_timing *timing = &monitor->data.std_timing[j];

					if (timing->h_size <= 256)
						continue;

					LOG(4,("%dx%d@%dHz (id=%d)\n", 
						timing->h_size, timing->v_size,
						timing->refresh, timing->id));
				}
				break;
			}

			case EDID1_IS_DETAILED_TIMING:
			{
				edid1_detailed_timing *timing = &monitor->data.detailed_timing;

				LOG(4,("Additional Video Mode:\n"));
				LOG(4,("clock=%f MHz\n", timing->pixel_clock / 100.0));
				LOG(4,("h: (%d, %d, %d, %d)\n", 
					timing->h_active, timing->h_active + timing->h_sync_off,
					timing->h_active + timing->h_sync_off + timing->h_sync_width,
					timing->h_active + timing->h_blank));
				LOG(4,("v: (%d, %d, %d, %d)\n", 
					timing->v_active, timing->v_active + timing->v_sync_off,
					timing->v_active + timing->v_sync_off + timing->v_sync_width,
					timing->v_active + timing->v_blank));
				LOG(4,("size: %.1f cm x %.1f cm\n", 
					timing->h_size / 10.0, timing->v_size / 10.0));
				LOG(4,("border: %.1f cm x %.1f cm\n",
					timing->h_border / 10.0, timing->v_border / 10.0));
				break;
			}
		}
	}
}

/* callback for getting signals from I2C bus */
static status_t
get_signals(void *cookie, int *clk, int *data)
{
	ddc_port_info *info = (ddc_port_info *)cookie;

	*clk = *data = 0x0000;
	if (InSCL(info->port)) *clk = 0x0001;
	if (InSDA(info->port)) *data = 0x0001;

	return B_OK;
}

/* callback for setting signals on I2C bus */
static status_t
set_signals(void *cookie, int clk, int data)
{
	ddc_port_info *info = (ddc_port_info *)cookie;

	if (clk)
		OutSCL(info->port, true);
	else
		OutSCL(info->port, false);

	if (data)
		OutSDA(info->port, true);
	else
		OutSDA(info->port, false);

	return B_OK;
}

/* Read EDID information from monitor via the display data channel (DDC) */
static status_t
i2c_ReadEDID(uint8 BusNR, edid1_info *edid)
{
	i2c_bus bus;
	ddc_port_info info;

	info.port = BusNR;

	bus.cookie = &info;
	bus.set_signals = &set_signals;
	bus.get_signals = &get_signals;
	ddc2_init_timing(&bus);

	/* select GPU I/O pins for I2C buses */
	i2c_select_bus_set(false);

	/* enable access to primary head */
	set_crtc_owner(0);

	if (ddc2_read_edid1(&bus, edid, NULL, NULL) == B_OK) {
		LOG(4,("I2C: EDID succesfully read from monitor at bus %d\n", BusNR));
		LOG(4,("I2C: EDID dump follows (bus %d):\n", BusNR));
		i2c_DumpEDID(edid);
		LOG(4,("I2C: end EDID dump (bus %d).\n", BusNR));
	} else {
		LOG(4,("I2C: reading EDID failed at bus %d!\n", BusNR));
		return B_ERROR;
	}

	return B_OK;
}

void i2c_TestEDID(void)
{
	uint8 bus;
	edid1_info edid;
	bool *i2c_bus = &(si->ps.i2c_bus0);

	/* test wired bus(es) */
	for (bus = 0; bus < 3; bus++) {
		if (i2c_bus[bus])
			i2c_ReadEDID(bus, &edid);
	}
}

static status_t
i2c_ExtractSpecsEDID(edid1_info* edid, edid_specs* specs)
{
	uint32 i;
	edid1_detailed_timing edid_timing;

	specs->have_full_edid = false;
	specs->have_native_edid = false;
	specs->timing.h_display = 0;
	specs->timing.v_display = 0;

	/* find the optimum (native) modeline */
	for (i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i) {
		switch(edid->detailed_monitor[i].monitor_desc_type) {
		case EDID1_IS_DETAILED_TIMING:
			// TODO: handle flags correctly!
			edid_timing = edid->detailed_monitor[i].data.detailed_timing;

			if (edid_timing.pixel_clock <= 0/* || edid_timing.sync != 3*/)
				break;

			/* we want the optimum (native) modeline only, widescreen if possible.
			 * So only check for horizontal display, not for vertical display. */
			if (edid_timing.h_active <= specs->timing.h_display)
				break;

			specs->timing.pixel_clock = edid_timing.pixel_clock * 10;
			specs->timing.h_display = edid_timing.h_active;
			specs->timing.h_sync_start = edid_timing.h_active + edid_timing.h_sync_off;
			specs->timing.h_sync_end = specs->timing.h_sync_start + edid_timing.h_sync_width;
			specs->timing.h_total = specs->timing.h_display + edid_timing.h_blank;
			specs->timing.v_display = edid_timing.v_active;
			specs->timing.v_sync_start = edid_timing.v_active + edid_timing.v_sync_off;
			specs->timing.v_sync_end = specs->timing.v_sync_start + edid_timing.v_sync_width;
			specs->timing.v_total = specs->timing.v_display + edid_timing.v_blank;
			specs->timing.flags = 0;
			if (edid_timing.sync == 3) {
				if (edid_timing.misc & 1)
					specs->timing.flags |= B_POSITIVE_HSYNC;
				if (edid_timing.misc & 2)
					specs->timing.flags |= B_POSITIVE_VSYNC;
			}
			if (edid_timing.interlaced)
				specs->timing.flags |= B_TIMING_INTERLACED;
			break;
		}
	}

	/* check if we actually got a modeline */
	if (!specs->timing.h_display || !specs->timing.v_display) return B_ERROR;

	/* check if the mode is at least VGA. If it's not, ignore specs */
	if ((specs->timing.h_display < 640) || (specs->timing.v_display < 480)) {
		LOG(4,("I2C: specsEDID: screen reports lower than VGA native mode, ignoring specs!\n"));
		return B_ERROR;
	}

	/* determine screen aspect ratio */
	specs->aspect =
		(specs->timing.h_display / ((float)specs->timing.v_display));

	/* determine connection type */
	specs->digital = false;
	if (edid->display.input_type) specs->digital = true;

	/* and also copy full edid1_info for reference */
	memcpy(&(specs->full_edid), edid, sizeof(specs->full_edid));

	/* we succesfully fetched the specs we need */
	specs->have_native_edid = true;
	/* we also got full and valid EDID via DDC */
	specs->have_full_edid = true;

	return B_OK;
}

/* Dump EDID info in driver's logfile */
static void
i2c_DumpSpecsEDID(edid_specs* specs)
{
	LOG(4,("I2C: specsEDID: have_native_edid: %s\n", specs->have_native_edid ? "True" : "False"));
	if (!specs->have_native_edid) return;
	LOG(4,("I2C: specsEDID: timing.pixel_clock %.3f Mhz\n", specs->timing.pixel_clock / 1000.0));
	LOG(4,("I2C: specsEDID: timing.h_display %d\n", specs->timing.h_display));
	LOG(4,("I2C: specsEDID: timing.h_sync_start %d\n", specs->timing.h_sync_start));
	LOG(4,("I2C: specsEDID: timing.h_sync_end %d\n", specs->timing.h_sync_end));
	LOG(4,("I2C: specsEDID: timing.h_total %d\n", specs->timing.h_total));
	LOG(4,("I2C: specsEDID: timing.v_display %d\n", specs->timing.v_display));
	LOG(4,("I2C: specsEDID: timing.v_sync_start %d\n", specs->timing.v_sync_start));
	LOG(4,("I2C: specsEDID: timing.v_sync_end %d\n", specs->timing.v_sync_end));
	LOG(4,("I2C: specsEDID: timing.v_total %d\n", specs->timing.v_total));
	LOG(4,("I2C: specsEDID: timing.flags $%08x\n", specs->timing.flags));
	LOG(4,("I2C: specsEDID: aspect: %1.2f\n", specs->aspect));
	LOG(4,("I2C: specsEDID: digital: %s\n", specs->digital ? "True" : "False"));
}

/* notes:
 * - con1 resides closest to the mainboard on for example NV25 and NV28, while for
 *   example on NV34 con2 sits closest to the mainboard.
 * - i2c bus0 is connected to con1, and i2c bus1 is connected to con2 on all pre-NV40
 *   architecture cards. On later cards it's vice versa. These connections do not depend
 *   on the analog VGA switch setting (see nv_general_output_select()). It also does
 *   not depend on the way screens are connected to the cards (DVI/VGA, 1 or 2 screens).
 * - on some NV40 architecture cards i2c bus2 connects to con2 instead of i2c bus0. This
 *   is confirmed on GeForce FX 6600 (NV43, id 0x0141) and GeForce 7300 (G72, id 0x01d1). 
 * - on pre-NV40 laptops i2c bus2 can connect to con2 as well: confirmed on a Geforce FX
 *   5200 Go (NV34, id 0x0324).
 * - con1 has CRTC1 and DAC1, and con2 has CRTC2 and DAC2 if nv_general_output_select()
 *   is set to 'straight' and there are only VGA type screens connected. */
void i2c_DetectScreens(void)
{
	edid1_info edid;

	si->ps.con1_screen.have_native_edid = false;
	si->ps.con2_screen.have_native_edid = false;
	si->ps.con1_screen.have_full_edid = false;
	si->ps.con2_screen.have_full_edid = false;
	si->ps.con1_screen.aspect = 0;
	si->ps.con2_screen.aspect = 0;

	/* check existance of bus 0 */
	if (si->ps.i2c_bus0) {
		/* check I2C bus 0 for an EDID capable screen */
		if (i2c_ReadEDID(0, &edid) == B_OK) {
			/* fetch optimum (native) modeline */
			switch (si->ps.card_arch) {
			case NV40A:
				i2c_ExtractSpecsEDID(&edid, &si->ps.con2_screen);
				break;
			default:
				i2c_ExtractSpecsEDID(&edid, &si->ps.con1_screen);
				break;
			}
		}
	}

	/* check existance of bus 1 */
	if (si->ps.i2c_bus1) {
		/* check I2C bus 1 for an EDID screen */
		if (i2c_ReadEDID(1, &edid) == B_OK) {
			/* fetch optimum (native) modeline */
			switch (si->ps.card_arch) {
			case NV40A:
				i2c_ExtractSpecsEDID(&edid, &si->ps.con1_screen);
				break;
			default:
				i2c_ExtractSpecsEDID(&edid, &si->ps.con2_screen);
				break;
			}
		}
	}

	/* check existance of bus 2 */
	if (si->ps.i2c_bus2) {
		/* check I2C bus 2 for an EDID screen */
		if (i2c_ReadEDID(2, &edid) == B_OK) {
			/* fetch optimum (native) modeline */
			switch (si->ps.card_arch) {
			case NV40A:
				if (!si->ps.con2_screen.have_native_edid) {
					i2c_ExtractSpecsEDID(&edid, &si->ps.con2_screen);
				} else {
					LOG(4,("I2C: DetectScreens: WARNING, unexpected behaviour detected!\n"));
				}
				break;
			default:
				if (!si->ps.con2_screen.have_native_edid && si->ps.laptop) {
					i2c_ExtractSpecsEDID(&edid, &si->ps.con2_screen);
				} else {
					LOG(4,("I2C: DetectScreens: WARNING, unexpected behaviour detected!\n"));
				}
				break;
			}
		}
	}
}
