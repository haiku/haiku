/* Copyright (c) 2003-2004 
 * Stefano Ceccherini <burton666@libero.it>. All rights reserved.
 * This file is released under the MIT license
 */
#ifndef __WB_INTERFACE_H
#define __WB_INTERFACE_H

#include "wb840.h"

extern void wb_read_eeprom(wb_device *, void* dest, int offset, int count, bool swap);
extern int wb_miibus_readreg(wb_device *, int phy_id, int location);
extern void wb_miibus_writereg(wb_device *, int phy, int location, int data);

#endif
