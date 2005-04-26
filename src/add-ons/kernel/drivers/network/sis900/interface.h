/* MII, MDIO and EEPROM interface of SiS 900
 *
 * Copyright 2001-2005 pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef INTERFACE_H
#define INTERFACE_H


#include "sis900.h"


extern uint16 eeprom_read(struct sis_info *info, int address);

extern uint16 mdio_read(struct sis_info *info, uint16 reg);
extern uint16 mdio_readFromPHY(struct sis_info *info, uint16 phy, uint16 reg);
extern void mdio_write(struct sis_info *info, uint16 reg, uint16 value);
extern void mdio_writeToPHY(struct sis_info *info, uint16 phy, uint16 reg, uint16 value);

extern uint16 mdio_statusFromPHY(struct sis_info *info, uint16 phy);
extern uint16 mdio_status(struct sis_info *info);

#endif  /* INTERFACE_H */
