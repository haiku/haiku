/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "acl.h"
#include "bits.h"


/*****************************************************************************/
__inline void et6000aclMasterInterruptEnable(void *base) {
    set8(&((volatile char *)base)[0x34], 0x7f, 0x80);
}
/*****************************************************************************/
__inline void et6000aclMasterInterruptDisable(void *base) {
    set8(&((volatile char *)base)[0x34], 0x7f, 0x00);
}
/*****************************************************************************/
__inline void et6000aclReadInterruptEnable(void *base) {
    set8(&((volatile char *)base)[0x34], 0xfd, 0x02);
}
/*****************************************************************************/
__inline void et6000aclReadInterruptDisable(void *base) {
    set8(&((volatile char *)base)[0x34], 0xfd, 0x00);
}
/*****************************************************************************/
__inline void et6000aclWriteInterruptEnable(void *base) {
    set8(&((volatile char *)base)[0x34], 0xfe, 0x01);
}
/*****************************************************************************/
__inline void et6000aclWriteInterruptDisable(void *base) {
    set8(&((volatile char *)base)[0x34], 0xfe, 0x00);
}
/*****************************************************************************/
__inline void et6000aclReadInterruptClear(void *base) {
    set8(&((volatile char *)base)[0x35], 0xfd, 0x02);
}
/*****************************************************************************/
__inline void et6000aclWriteInterruptClear(void *base) {
    set8(&((volatile char *)base)[0x34], 0xfe, 0x00);
}
/*****************************************************************************/
__inline char et6000aclInterruptCause(void *base) {
    return ((volatile char *)base)[0x35] & 0x03;
}
/*****************************************************************************/
