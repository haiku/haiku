/*
 * Copyright 2005, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_SHELL_COMMAND_H
#define FS_SHELL_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

int send_external_command(const char *command, int *result);

#ifdef __cplusplus
}
#endif

#endif	// FS_SHELL_COMMAND_H
