#ifndef __WB_INTERFACE_H
#define __WB_INTERFACE_H

#include "wb840.h"

extern void wb_read_eeprom(struct wb_device *, void* dest, int offset, int count, int swap);
extern uint16 mii_read(struct wb_device *, int phy_id, int location);

#endif
