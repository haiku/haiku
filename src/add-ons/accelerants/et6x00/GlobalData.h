/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#ifndef _ET6000GLOBALDATA_H_
#define _ET6000GLOBALDATA_H_

#include "DriverInterface.h"


/*****************************************************************************/
extern int fd;
extern ET6000SharedInfo *si;
extern area_id sharedInfoArea;
extern display_mode *et6000ModesList;
extern area_id et6000ModesListArea;
extern int accelerantIsClone;
extern volatile unsigned char *mmRegs; /* memory mapped registers */
/*****************************************************************************/


#endif /* _ET6000GLOBALDATA_H_ */
