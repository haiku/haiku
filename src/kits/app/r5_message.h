/*
 * Copyright 2005, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef _R5_MESSAGE_H_
#define _R5_MESSAGE_H_


#include <SupportDefs.h>

class BMessage;
class BDataIO;


namespace BPrivate {

ssize_t r5_message_flattened_size(const BMessage *message);
status_t flatten_r5_message(const BMessage *message, char *buffer, ssize_t size);
status_t flatten_r5_message(const BMessage *message, BDataIO *stream, ssize_t *size);
status_t unflatten_r5_message(uint32 format, BMessage *message, const char *flatBuffer);
status_t unflatten_r5_message(uint32 format, BMessage *message, BDataIO *stream);

}

#endif	// _R5_MESSAGE_H_
