/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __INPUT_GLOBALS_H
#define __INPUT_GLOBALS_H


#include <InterfaceDefs.h>
#include <SupportDefs.h>

class BMessage;

status_t _control_input_server_(BMessage *command, BMessage *reply);
status_t _restore_key_map_();
void _get_key_map(key_map **map, char **key_buffer, ssize_t *key_buffer_size);

#endif // __INPUT_GLOBALS_H
