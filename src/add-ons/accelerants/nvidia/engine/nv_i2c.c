/*
 * i2c interface.
 * Bus should be run at max. 100kHz: see original Philips I2C specification
 *	
 * Rudolf Cornelissen 12/2002-5/2009
 */

#define MODULE_BIT 0x00004000

#include "nv_std.h"

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
	if (set)
	{
		NV_REG32(NV32_FUNCSEL) &= ~0x00000010;
		NV_REG32(NV32_2FUNCSEL) |= 0x00000010;
	}
	else
	{
		NV_REG32(NV32_2FUNCSEL) &= ~0x00000010;
		NV_REG32(NV32_FUNCSEL) |= 0x00000010;
	}
}

static void OutSCL(uint8 BusNR, bool Bit)
{
	uint8 data;

	if (BusNR & 0x01)
	{
		data = (CRTCR(WR_I2CBUS_1) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_1, (data | 0x20));
		else
			CRTCW(WR_I2CBUS_1, (data & ~0x20));
	}
	else
	{
		data = (CRTCR(WR_I2CBUS_0) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_0, (data | 0x20));
		else
			CRTCW(WR_I2CBUS_0, (data & ~0x20));
	}
}

static void OutSDA(uint8 BusNR, bool Bit)
{
	uint8 data;
	
	if (BusNR & 0x01)
	{
		data = (CRTCR(WR_I2CBUS_1) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_1, (data | 0x10));
		else
			CRTCW(WR_I2CBUS_1, (data & ~0x10));
	}
	else
	{
		data = (CRTCR(WR_I2CBUS_0) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_0, (data | 0x10));
		else
			CRTCW(WR_I2CBUS_0, (data & ~0x10));
	}
}

static bool InSCL(uint8 BusNR)
{
	if (BusNR & 0x01)
	{
		if ((CRTCR(RD_I2CBUS_1) & 0x04)) return true;
	}
	else
	{
		if ((CRTCR(RD_I2CBUS_0) & 0x04)) return true;
	}

	return false;
}

static bool InSDA(uint8 BusNR)
{
	if (BusNR & 0x01)
	{
		if ((CRTCR(RD_I2CBUS_1) & 0x08)) return true;
	}
	else
	{
		if ((CRTCR(RD_I2CBUS_0) & 0x08)) return true;
	}

	return false;
}

static void TXBit (uint8 BusNR, bool Bit)
{
	/* send out databit */
	if (Bit)
	{
		OutSDA(BusNR, true);
		snooze(3);
		if (!InSDA(BusNR)) i2c_flag_error (2);
	}
	else
	{
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
	/* select GPU I/O pins set */
	i2c_select_bus_set(BusNR & 0x02);

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
	/* select GPU I/O pins set */
	i2c_select_bus_set(BusNR & 0x02);

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

	/* select GPU I/O pins set */
	i2c_select_bus_set(BusNR & 0x02);

	/* enable access to primary head */
	set_crtc_owner(0);

	/* read data */
	for (cnt = 8; cnt > 0; cnt--)
	{
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

	/* select GPU I/O pins set */
	i2c_select_bus_set(BusNR & 0x02);

	/* enable access to primary head */
	set_crtc_owner(0);

	/* write data */
	for (cnt = 8; cnt > 0; cnt--)
	{
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
	{
		buf[cnt] = i2c_readbyte(BusNR, buf[cnt]);
	}
}

void i2c_writebuffer (uint8 BusNR, uint8* buf, uint8 size)
{
	uint8 cnt;

	for (cnt = 0; cnt < size; cnt++)
	{
		i2c_writebyte(BusNR, buf[cnt]);
	}
}

status_t i2c_init(void)
{
	uint8 bus, buses;
	bool *i2c_bus = &(si->ps.i2c_bus0);
	status_t result = B_ERROR;

	LOG(4,("I2C: searching for wired I2C buses...\n"));

	/* enable access to primary head */
	set_crtc_owner(0);

	/* preset no board wired buses */
	si->ps.i2c_bus0 = false;
	si->ps.i2c_bus1 = false;
	si->ps.i2c_bus2 = false;
	si->ps.i2c_bus3 = false;

	/* set number of buses to test for */
	buses = 2;
	if (si->ps.secondary_head) buses = 4;

	/* find existing buses */
	for (bus = 0; bus < buses; bus++)
	{
		/* reset status */
		i2c_flag_error (-1);
		snooze(6);
		/* init and/or stop I2C bus */
		i2c_bstop(bus);
		/* check for hardware coupling of SCL and SDA -out and -in lines */
		snooze(6);
		OutSCL(bus, false);
		OutSDA(bus, true);
		snooze(3);
		if (InSCL(bus) || !InSDA(bus)) continue;
		snooze(3);
		OutSCL(bus, true);
		OutSDA(bus, false);
		snooze(3);
		if (!InSCL(bus) || InSDA(bus)) continue;
		i2c_bus[bus] = true;
		snooze(3);
		/* re-init bus */
		i2c_bstop(bus);
	}

	for (bus = 0; bus < buses; bus++)
	{
		if (i2c_bus[bus])
		{
			LOG(4,("I2C: bus #%d wiring check: passed\n", bus));
			result = B_OK;
		}
		else
			LOG(4,("I2C: bus #%d wiring check: failed\n", bus));
	}

	i2c_TestEDID();

	return result;
}

/*** DDC/EDID library use ***/
typedef struct {
	uint32 port;
} ddc_port_info;

/* Dump EDID info in driver's logfile */
static void
i2c_edid_dump(edid1_info *edid)
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
				LOG(4,(" %s\n", monitor->data.serial_number));
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
status_t
i2c_ReadEDID(uint8 BusNR, edid1_info *edid)
{
	i2c_bus bus;
	ddc_port_info info;

	info.port = BusNR;

	bus.cookie = &info;
	bus.set_signals = &set_signals;
	bus.get_signals = &get_signals;
	ddc2_init_timing(&bus);

	/* select GPU I/O pins set */
	i2c_select_bus_set(BusNR & 0x02);

	/* enable access to primary head */
	set_crtc_owner(0);

	if (ddc2_read_edid1(&bus, edid, NULL, NULL) == B_OK) {
		LOG(4,("I2C: EDID succesfully read from monitor at bus %d\n", BusNR));
		LOG(4,("I2C: EDID dump follows (bus %d):\n", BusNR));
//		has_edid = true;
		i2c_edid_dump(edid);
		LOG(4,("I2C: end EDID dump (bus %d).\n", BusNR));
	} else {
		LOG(4,("I2C: reading EDID failed at bus %d!\n", BusNR));
		return B_ERROR;
	}

	return B_OK;
}

void i2c_TestEDID(void)
{
	uint8 bus, buses;

	/* set number of buses to test for */
	buses = 2;
	if (si->ps.secondary_head) buses = 4;

	/* test bus */
	for (bus = 0; bus < buses; bus++) {
		edid1_info edid;
		i2c_ReadEDID(bus, &edid);
	}
}
