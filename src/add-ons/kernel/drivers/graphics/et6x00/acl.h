/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/
#ifndef _ET6000ACL_H_
#define _ET6000ACL_H_


/*****************************************************************************/
__inline void et6000aclMasterInterruptEnable(void *base);
__inline void et6000aclMasterInterruptDisable(void *base);
__inline void et6000aclReadInterruptEnable(void *base);
__inline void et6000aclReadInterruptDisable(void *base);
__inline void et6000aclWriteInterruptEnable(void *base);
__inline void et6000aclWriteInterruptDisable(void *base);
__inline void et6000aclReadInterruptClear(void *base);
__inline void et6000aclWriteInterruptClear(void *base);
__inline char et6000aclInterruptCause(void *base);
/*****************************************************************************/
#define ET6000_ACL_INT_CAUSE_NONE 0
#define ET6000_ACL_INT_CAUSE_READ 2
#define ET6000_ACL_INT_CAUSE_WRITE 1
#define ET6000_ACL_INT_CAUSE_BOTH 3
/*****************************************************************************/


#endif /* _ET6000ACL_H_ */
