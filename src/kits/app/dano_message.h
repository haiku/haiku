/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DANO_MESSAGE_H
#define DANO_MESSAGE_H


#include <SupportDefs.h>

class BMessage;
class BDataIO;


namespace BPrivate {

status_t unflatten_dano_message(uint32 format, BDataIO& stream, BMessage& message);
ssize_t dano_message_flattened_size(const char* buffer);

}

#endif	// DANO_MESSAGE_H
