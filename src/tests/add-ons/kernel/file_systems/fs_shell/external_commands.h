/*
 * Copyright 2005, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_SHELL_EXTERNAL_COMMANDS_H
#define FS_SHELL_EXTERNAL_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

char *get_external_command(const char *prompt, char *input, int len);
void reply_to_external_command(int result);
void external_command_cleanup();

#ifdef __cplusplus
}
#endif

#endif	// FS_SHELL_EXTERNAL_COMMANDS_H
