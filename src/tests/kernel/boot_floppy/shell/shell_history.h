#ifndef _SHELL_HISTORY_H_
#define _SHELL_HISTORY_H_


// you can store up to this many commands in the history table

#define MAX_HISTORY_DEPTH 20


// the only two history operations:

void  store_history_command(char *cmd_string);
char *fetch_history_command(void);


#endif
