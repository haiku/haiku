/*
 * i2c interface.
 * Bus should be run at max. 100kHz: see original Philips I2C specification
 *	
 * Rudolf Cornelissen 12/2002-10/2005
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

	return result;
}
