/*
 * Copyright 2005, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_SHELL_ADDITIONAL_COMMANDS_H
#define FS_SHELL_ADDITIONAL_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct cmd_entry {
    char *name;
    int  (*func)(int argc, char **argv);
    char *help;
} cmd_entry;

extern cmd_entry additional_commands[];


#ifdef __cplusplus
}
#endif

#endif	// FS_SHELL_ADDITIONAL_COMMANDS_H
