/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SETTINGS_H
#define _SETTINGS_H

/* settings */
extern char google_server[20];
extern int google_server_port;
extern uint32 max_vnodes;
extern uint32 max_results;
extern bool sync_unlink_queries;

extern status_t load_settings(void);

#endif /* _SETTINGS_H */
