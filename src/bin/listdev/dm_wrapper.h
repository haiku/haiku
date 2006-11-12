#ifndef _DM_WRAPPER_H_
#define _DM_WRAPPER_H_

#include "kdevice_manager.h"

status_t init_dm_wrapper(void);
status_t uninit_dm_wrapper(void);

status_t get_root(uint32 *cookie);
status_t get_child(uint32 *cookie);
status_t get_next_child(uint32 *cookie);
status_t dm_get_next_attr(struct dev_attr *attr);

#endif
