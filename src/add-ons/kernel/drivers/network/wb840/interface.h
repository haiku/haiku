#ifndef __WB_INTERFACE_H
#define __WB_INTERFACE_H

#include "wb840.h"

extern void wb_read_eeprom(struct wb_device *, void* dest, int offset, int count, int swap);
extern int wb_miibus_readreg(struct wb_device *, int phy_id, int location);
extern void wb_miibus_writereg(struct wb_device *, int phy, int location, int data);
#endif
