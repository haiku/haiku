/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/
#ifndef _ET6000SETMODE_H_
#define _ET6000SETMODE_H_

#include "DriverInterface.h"
#include "bits.h"


/*****************************************************************************/
__inline void et6000EnableLinearMemoryMapping(uint16 pciConfigSpace);
status_t et6000SetMode(display_mode *mode, uint16 pciConfigSpace);
status_t et6000ProposeMode(display_mode *mode, uint32 memSize);
/*****************************************************************************/


#endif /* _ET6000SETMODE_H_ */
