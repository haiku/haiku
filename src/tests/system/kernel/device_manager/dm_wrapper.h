#ifndef _DM_WRAPPER_H_
#define _DM_WRAPPER_H_

status_t init_dm_wrapper(void);
status_t uninit_dm_wrapper(void);

status_t get_child(void);
status_t get_next_child(void);
status_t get_parent(void);
status_t dm_get_next_attr(void);
status_t dm_retrieve_attr(struct dev_attr *attr);

#endif
