/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "GlobalData.h"


/*****************************************************************************/
int fd;
ET6000SharedInfo *si;
area_id sharedInfoArea;
display_mode *et6000ModesList;
area_id et6000ModesListArea;
int accelerantIsClone;
volatile unsigned char *mmRegs; /* memory mapped registers */
/*****************************************************************************/
