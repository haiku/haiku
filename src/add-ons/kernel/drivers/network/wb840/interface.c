/*
 * Copyright (c) 2003-2004 Stefano Ceccherini (burton666@libero.it)
 * Copyright (c) 1997, 1998
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "wb840.h"
#include "device.h"
#include "interface.h"

#include <ByteOrder.h>
#include <KernelExport.h>

#include <string.h>


#define SIO_SET(x)	\
	write32(device->reg_base + WB_SIO,	\
		read32(device->reg_base + WB_SIO) | x)

#define SIO_CLR(x)	\
	write32(device->reg_base + WB_SIO,	\
		read32(device->reg_base + WB_SIO) & ~x)

#define MII_DELAY(x)	read32(x->reg_base + WB_SIO)


static void
mii_sync(struct wb_device *device)
{
	// Set data bit and strobe the clock 32 times
	int bits = 32;

	SIO_SET(WB_SIO_MII_DIR|WB_SIO_MII_DATAIN);

	while (--bits >= 0) {
		SIO_SET(WB_SIO_MII_CLK);
		MII_DELAY(device);
		SIO_CLR(WB_SIO_MII_CLK);
		MII_DELAY(device);
	}
}


static void
mii_send(wb_device *device, uint32 bits, int count)
{
	int	i;

	SIO_CLR(WB_SIO_MII_CLK);

	for (i = (0x1 << (count - 1)); i; i >>= 1) {
    	if (bits & i)
			SIO_SET(WB_SIO_MII_DATAIN);
        else
        	SIO_CLR(WB_SIO_MII_DATAIN);
		MII_DELAY(device);
		SIO_CLR(WB_SIO_MII_CLK);
		MII_DELAY(device);
		SIO_SET(WB_SIO_MII_CLK);
	}
}

/*
 * Read an PHY register through the MII.
 */
static int
wb_mii_readreg(wb_device *device, wb_mii_frame *frame)
{
	int	i, ack;

	/*
	 * Set up frame for RX.
	 */
	frame->mii_stdelim = WB_MII_STARTDELIM;
	frame->mii_opcode = WB_MII_READOP;
	frame->mii_turnaround = 0;
	frame->mii_data = 0;

	write32(device->reg_base + WB_SIO, 0);

	/*
 	 * Turn on data xmit.
	 */
	SIO_SET(WB_SIO_MII_DIR);

	mii_sync(device);

	/*
	 * Send command/address info.
	 */
	mii_send(device, frame->mii_stdelim, 2);
	mii_send(device, frame->mii_opcode, 2);
	mii_send(device, frame->mii_phyaddr, 5);
	mii_send(device, frame->mii_regaddr, 5);

	/* Idle bit */
	SIO_CLR((WB_SIO_MII_CLK|WB_SIO_MII_DATAIN));
	MII_DELAY(device);
	SIO_SET(WB_SIO_MII_CLK);
	MII_DELAY(device);

	/* Turn off xmit. */
	SIO_CLR(WB_SIO_MII_DIR);
	/* Check for ack */
	SIO_CLR(WB_SIO_MII_CLK);
	MII_DELAY(device);
	ack = read32(device->reg_base + WB_SIO) & WB_SIO_MII_DATAOUT;
	SIO_SET(WB_SIO_MII_CLK);
	MII_DELAY(device);
	SIO_CLR(WB_SIO_MII_CLK);
	MII_DELAY(device);
	SIO_SET(WB_SIO_MII_CLK);
	MII_DELAY(device);

	/*
	 * Now try reading data bits. If the ack failed, we still
	 * need to clock through 16 cycles to keep the PHY(s) in sync.
	 */
	if (ack) {
		for(i = 0; i < 16; i++) {
			SIO_CLR(WB_SIO_MII_CLK);
			MII_DELAY(device);
			SIO_SET(WB_SIO_MII_CLK);
			MII_DELAY(device);
		}
		goto fail;
	}

	for (i = 0x8000; i; i >>= 1) {
		SIO_CLR(WB_SIO_MII_CLK);
		MII_DELAY(device);
		if (!ack) {
			if (read32(device->reg_base + WB_SIO) & WB_SIO_MII_DATAOUT)
				frame->mii_data |= i;
			MII_DELAY(device);
		}
		SIO_SET(WB_SIO_MII_CLK);
		MII_DELAY(device);
	}

fail:

	SIO_CLR(WB_SIO_MII_CLK);
	MII_DELAY(device);
	SIO_SET(WB_SIO_MII_CLK);
	MII_DELAY(device);

	if (ack)
		return 1;
	return 0;
}

/*
 * Write to a PHY register through the MII.
 */
static int
wb_mii_writereg(wb_device *device, wb_mii_frame	*frame)
{
	/*
	 * Set up frame for TX.
	 */

	frame->mii_stdelim = WB_MII_STARTDELIM;
	frame->mii_opcode = WB_MII_WRITEOP;
	frame->mii_turnaround = WB_MII_TURNAROUND;

	/*
 	 * Turn on data output.
	 */
	SIO_SET(WB_SIO_MII_DIR);

	mii_sync(device);

	mii_send(device, frame->mii_stdelim, 2);
	mii_send(device, frame->mii_opcode, 2);
	mii_send(device, frame->mii_phyaddr, 5);
	mii_send(device, frame->mii_regaddr, 5);
	mii_send(device, frame->mii_turnaround, 2);
	mii_send(device, frame->mii_data, 16);

	/* Idle bit. */
	SIO_SET(WB_SIO_MII_CLK);
	MII_DELAY(device);
	SIO_CLR(WB_SIO_MII_CLK);
	MII_DELAY(device);

	/*
	 * Turn off xmit.
	 */
	SIO_CLR(WB_SIO_MII_DIR);

	return 0;
}


int
wb_miibus_readreg(wb_device *device, int phy, int reg)
{
	struct wb_mii_frame frame;

	memset(&frame, 0, sizeof(frame));

	frame.mii_phyaddr = phy;
	frame.mii_regaddr = reg;
	wb_mii_readreg(device, &frame);

	return frame.mii_data;
}


void
wb_miibus_writereg(wb_device *device, int phy, int reg, int data)
{
	struct wb_mii_frame frame;

	memset(&frame, 0, sizeof(frame));

	frame.mii_phyaddr = phy;
	frame.mii_regaddr = reg;
	frame.mii_data = data;

	wb_mii_writereg(device, &frame);

	return;
}


#define EEPROM_DELAY(x)	read32(x->reg_base + WB_SIO)

#if 0
static void
wb_eeprom_putbyte(wb_device *device, int addr)
{
	int	d, i;
	int delay;

	d = addr | WB_EECMD_READ;

	/*
	 * Feed in each bit and strobe the clock.
	 */
	for (i = 0x400; i; i >>= 1) {
		if (d & i) {
			SIO_SET(WB_SIO_EE_DATAIN);
		} else {
			SIO_CLR(WB_SIO_EE_DATAIN);
		}
		for (delay = 0; delay < 100; delay++)
			MII_DELAY(device);

		SIO_SET(WB_SIO_EE_CLK);

		for (delay = 0; delay < 150; delay++)
			MII_DELAY(device);

		SIO_CLR(WB_SIO_EE_CLK);

		for (delay = 0; delay < 100; delay++)
			MII_DELAY(device);

	}

	return;
}
#endif


static void
wb_eeprom_askdata(wb_device *device, int addr)
{
	int command, i;
	int delay;

	command = addr | WB_EECMD_READ;

	/* Feed in each bit and strobe the clock. */
	for(i = 0x400; i; i >>= 1) {
		if (command & i)
			SIO_SET(WB_SIO_EE_DATAIN);
		else
			SIO_CLR(WB_SIO_EE_DATAIN);

		SIO_SET(WB_SIO_EE_CLK);

		SIO_CLR(WB_SIO_EE_CLK);
		for (delay = 0; delay < 100; delay++)
			EEPROM_DELAY(device);
	}
}


/* Read a word of data stored in the EEPROM at address "addr". */
static void
wb_eeprom_getword(wb_device *device, int addr, uint16 *dest)
{
	int i;
	uint16 word = 0;

	/* Enter EEPROM access mode */
	write32(device->reg_base + WB_SIO, WB_SIO_EESEL|WB_SIO_EE_CS);

	/* Send address of word we want to read. */
	wb_eeprom_askdata(device, addr);

	write32(device->reg_base + WB_SIO, WB_SIO_EESEL|WB_SIO_EE_CS);

	/* Start reading bits from EEPROM */
	for (i = 0x8000; i > 0; i >>= 1) {
		SIO_SET(WB_SIO_EE_CLK);
		if (read32(device->reg_base + WB_SIO) & WB_SIO_EE_DATAOUT)
			word |= i;
		SIO_CLR(WB_SIO_EE_CLK);
	}

	/* Turn off EEPROM access mode */
	write32(device->reg_base + WB_SIO, 0);

	*dest = word;
}


void
wb_read_eeprom(wb_device *device, void* dest,
	int offset, int count, bool swap)
{
	int i;
	uint16 word = 0, *ptr;

	for (i = 0; i < count; i++) {
		wb_eeprom_getword(device, offset + i, &word);
		ptr = (uint16 *)((uint8 *)dest + (i * 2));
		if (swap)
			*ptr = B_BENDIAN_TO_HOST_INT16(word);
		else
			*ptr = word;
	}
}
