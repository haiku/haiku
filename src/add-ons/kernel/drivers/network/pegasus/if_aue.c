/*
 * Pegasus BeOS Driver
 *
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */

/*-
 * Copyright (c) 1997, 1998, 1999, 2000
 *	Bill Paul <wpaul@ee.columbia.edu>.  All rights reserved.
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
 */

#include <string.h>
#include "driver.h"
#include "usbdevs.h"

extern usb_module_info *usb;

#define AUE_SETBIT(sc, reg, x)				\
	aue_csr_write_1(sc, reg, aue_csr_read_1(sc, reg) | (x))

#define AUE_CLRBIT(sc, reg, x)				\
	aue_csr_write_1(sc, reg, aue_csr_read_1(sc, reg) & ~(x))

static int
aue_csr_read_1(pegasus_dev *sc, int reg)
{
	status_t		err;
	uint8		val = 0;
	size_t		length;

	if (sc->aue_dying)
		return (0);

	AUE_LOCK(sc);

	err = usb->send_request(sc->dev, USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
		AUE_UR_READREG, 0, reg, 1, &val, &length);
	AUE_UNLOCK(sc);

	if (err) {
		return (0);
	}

	return (val);
}

static int
aue_csr_read_2(pegasus_dev *sc, int reg)
{
	status_t		err;
	uint16		val = 0;
	size_t		length;

	if (sc->aue_dying)
		return (0);

	AUE_LOCK(sc);

	err = usb->send_request(sc->dev, USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
		AUE_UR_READREG, 0, reg, 2, &val, &length);

	AUE_UNLOCK(sc);

	if (err) {
		return (0);
	}

	return (val);
}

static int
aue_csr_write_1(pegasus_dev *sc, int reg, int val)
{
	status_t		err;
	size_t		length = 1;

	if (sc->aue_dying)
		return (0);

	AUE_LOCK(sc);

	err = usb->send_request(sc->dev, USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		AUE_UR_WRITEREG, val, reg, 1, &val, &length);

	AUE_UNLOCK(sc);

	if (err) {
		return (-1);
	}

	return (0);
}

static int
aue_csr_write_2(pegasus_dev *sc, int reg, int val)
{
	status_t		err;
	size_t			length = 2;

	if (sc->aue_dying)
		return (0);

	AUE_LOCK(sc);

	err = usb->send_request(sc->dev, USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		AUE_UR_WRITEREG, val, reg, 2, &val, &length);

	AUE_UNLOCK(sc);

	if (err) {
		return (-1);
	}

	return (0);
}

/*
 * Read a word of data stored in the EEPROM at address 'addr.'
 */
static void
aue_eeprom_getword(pegasus_dev *sc, int addr, u_int16_t *dest)
{
	int		i;
	u_int16_t	word = 0;

	aue_csr_write_1(sc, AUE_EE_REG, addr);
	aue_csr_write_1(sc, AUE_EE_CTL, AUE_EECTL_READ);

	for (i = 0; i < AUE_TIMEOUT; i++) {
		if (aue_csr_read_1(sc, AUE_EE_CTL) & AUE_EECTL_DONE)
			break;
	}

	if (i == AUE_TIMEOUT) {
		dprintf("aue%d: EEPROM read timed out\n",
		    sc->aue_unit);
	}

	word = aue_csr_read_2(sc, AUE_EE_DATA);
	*dest = word;

	return;
}




/*
 * Read a sequence of words from the EEPROM.
 */
static void
aue_read_eeprom(pegasus_dev *sc, caddr_t dest, int off, int cnt, int swap)
{
	int			i;
	u_int16_t		word = 0, *ptr;

	for (i = 0; i < cnt; i++) {
		aue_eeprom_getword(sc, off + i, &word);
		ptr = (u_int16_t *)(dest + (i * 2));
		if (swap)
			*ptr = B_BENDIAN_TO_HOST_INT16(word);
		else
			*ptr = word;
	}

	return;
}


static int
aue_miibus_readreg(pegasus_dev *sc, int phy, int reg)
{
	int			i;
	u_int16_t		val = 0;

	/*
	 * The Am79C901 HomePNA PHY actually contains
	 * two transceivers: a 1Mbps HomePNA PHY and a
	 * 10Mbps full/half duplex ethernet PHY with
	 * NWAY autoneg. However in the ADMtek adapter,
	 * only the 1Mbps PHY is actually connected to
	 * anything, so we ignore the 10Mbps one. It
	 * happens to be configured for MII address 3,
	 * so we filter that out.
	 */
	if (sc->aue_vendor == USB_VENDOR_ADMTEK &&
	    sc->aue_product == USB_PRODUCT_ADMTEK_PEGASUS) {
		if (phy == 3)
			return (0);
#ifdef notdef
		if (phy != 1)
			return (0);
#endif
	}

	aue_csr_write_1(sc, AUE_PHY_ADDR, phy);
	aue_csr_write_1(sc, AUE_PHY_CTL, reg | AUE_PHYCTL_READ);

	for (i = 0; i < AUE_TIMEOUT; i++) {
		if (aue_csr_read_1(sc, AUE_PHY_CTL) & AUE_PHYCTL_DONE)
			break;
	}

	if (i == AUE_TIMEOUT) {
		dprintf("aue%d: MII read timed out\n", sc->aue_unit);
	}

	val = aue_csr_read_2(sc, AUE_PHY_DATA);

	return (val);
}


static int
aue_miibus_writereg(pegasus_dev *sc, int phy, int reg, int data)
{
	int			i;

	if (phy == 3)
		return (0);

	aue_csr_write_2(sc, AUE_PHY_DATA, data);
	aue_csr_write_1(sc, AUE_PHY_ADDR, phy);
	aue_csr_write_1(sc, AUE_PHY_CTL, reg | AUE_PHYCTL_WRITE);

	for (i = 0; i < AUE_TIMEOUT; i++) {
		if (aue_csr_read_1(sc, AUE_PHY_CTL) & AUE_PHYCTL_DONE)
			break;
	}

	if (i == AUE_TIMEOUT) {
		dprintf("aue%d: MII read timed out\n",
		    sc->aue_unit);
	}

	return (0);
}


static uint16
aue_miibus_read(pegasus_dev *dev, uint16 reg)
{
	return aue_miibus_readreg(dev, dev->phy, reg);
}


static void
aue_miibus_write(pegasus_dev *dev, uint16 reg, uint16 value)
{
	aue_miibus_writereg(dev, dev->phy, reg, value);
}


static uint16
aue_miibus_status_from_phy(pegasus_dev *dev, uint16 phy)
{
	uint16 status;
	int i = 0;

	// the status must be retrieved two times, because the first
	// one may not work on some PHYs (notably ICS 1893)
	while (i++ < 2)
		status = aue_miibus_readreg(dev, phy, MII_STATUS);

	return status;
}


static uint16
aue_miibus_status(pegasus_dev *dev)
{
	return aue_miibus_status_from_phy(dev, dev->phy);
}


static status_t
aue_init_phy(pegasus_dev *dev)
{
	uint16 phy, status;
	int i;

	dev->phy = 255;

	// search for total of 32 possible MII PHY addresses
	for (phy = 0; phy < 32; phy++) {
		uint16 status;

		status = aue_miibus_status_from_phy(dev, phy);
		if (status == 0xffff || status == 0x0000)
			// this MII is not accessable
			continue;

		dev->phy = phy;
	}

	if (dev->phy == 255) {
		DPRINTF_ERR("No MII PHY transceiver found!\n");
		return B_ENTRY_NOT_FOUND;
	}
	DPRINTF_INFO("aue_init_phy MII PHY found at %d\n", dev->phy);

	status = aue_miibus_read(dev, MII_CONTROL);
	status &= ~MII_CONTROL_ISOLATE;

	aue_miibus_write(dev, MII_CONTROL, status);

	aue_miibus_write(dev, MII_CONTROL, MII_CONTROL_RESET);
	for (i = 0; i < 100; i++) {
		if ((aue_miibus_read(dev, MII_STATUS) & MII_CONTROL_RESET) == 0)
			break;
		DELAY(1000);
	}

	dev->link = aue_miibus_status(dev) & MII_STATUS_LINK;

	return B_OK;
}


static void
aue_reset_pegasus_II(pegasus_dev *sc)
{
	/* Magic constants taken from Linux driver. */
	aue_csr_write_1(sc, AUE_REG_1D, 0);
	aue_csr_write_1(sc, AUE_REG_7B, 2);
#if 0
	if ((sc->aue_flags & HAS_HOME_PNA) && mii_mode)
		aue_csr_write_1(sc, AUE_REG_81, 6);
	else
#endif
		aue_csr_write_1(sc, AUE_REG_81, 2);
}

static void
aue_reset(pegasus_dev *sc)
{
	int		i;

	AUE_SETBIT(sc, AUE_CTL1, AUE_CTL1_RESETMAC);

	for (i = 0; i < AUE_TIMEOUT; i++) {
		if (!(aue_csr_read_1(sc, AUE_CTL1) & AUE_CTL1_RESETMAC))
			break;
	}

	if (i == AUE_TIMEOUT)
		dprintf("aue%d: reset failed\n", sc->aue_unit);

	/*
	 * The PHY(s) attached to the Pegasus chip may be held
	 * in reset until we flip on the GPIO outputs. Make sure
	 * to set the GPIO pins high so that the PHY(s) will
	 * be enabled.
	 *
	 * Note: We force all of the GPIO pins low first, *then*
	 * enable the ones we want.
	 */
	aue_csr_write_1(sc, AUE_GPIO0, AUE_GPIO_OUT0|AUE_GPIO_SEL0);
	aue_csr_write_1(sc, AUE_GPIO0, AUE_GPIO_OUT0|AUE_GPIO_SEL0|AUE_GPIO_SEL1);

	if (sc->aue_flags & LSYS) {
		/* Grrr. LinkSys has to be different from everyone else. */
		aue_csr_write_1(sc, AUE_GPIO0,
			AUE_GPIO_SEL0 | AUE_GPIO_SEL1);
		aue_csr_write_1(sc, AUE_GPIO0,
			AUE_GPIO_SEL0 | AUE_GPIO_SEL1 | AUE_GPIO_OUT0);
	}

	if (sc->aue_flags & PII)
		aue_reset_pegasus_II(sc);

	/* Wait a little while for the chip to get its brains in order. */
	DELAY(10000);

	return;
}


/*
 * Attach
 */
void
aue_attach(pegasus_dev *sc)
{
	u_char		eaddr[ETHER_ADDRESS_LENGTH];

	AUE_LOCK(sc);

	/* Reset the adapter. */
	aue_reset(sc);

	/*
	 * Get station address from the EEPROM.
	 */
	aue_read_eeprom(sc, (caddr_t)&eaddr, 0, 3, 0);

	memcpy(sc->macaddr, eaddr, ETHER_ADDRESS_LENGTH);

	if (aue_init_phy(sc) != B_OK)
		goto done;

	sc->aue_dying = 0;

done:
	AUE_UNLOCK(sc);
}


void
aue_init(pegasus_dev *sc)
{

	/* Enable RX and TX */
	aue_csr_write_1(sc, AUE_CTL0, AUE_CTL0_RXSTAT_APPEND | AUE_CTL0_RX_ENB);
	AUE_SETBIT(sc, AUE_CTL0, AUE_CTL0_TX_ENB);
	AUE_SETBIT(sc, AUE_CTL2, AUE_CTL2_EP3_CLR);

}


void
aue_uninit(pegasus_dev *sc)
{
	aue_csr_write_1(sc, AUE_CTL0, 0);
	aue_csr_write_1(sc, AUE_CTL1, 0);
	aue_reset(sc);
}
