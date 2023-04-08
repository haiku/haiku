/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SETTINGS_H
#define _SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/* settings */
extern uint32 max_vnodes;
extern uint32 max_results;
extern bool sync_unlink_queries;

extern status_t load_settings(void);

#ifdef __cplusplus
}
#endif

#endif /* _SETTINGS_H */
