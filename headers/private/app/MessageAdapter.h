/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef _MESSAGE_ADAPTER_H_
#define _MESSAGE_ADAPTER_H_

#include <Message.h>
#include <util/KMessage.h>

// message formats
#define MESSAGE_FORMAT_R5				'FOB1'
#define MESSAGE_FORMAT_R5_SWAPPED		'1BOF'
#define MESSAGE_FORMAT_DANO				'FOB2'
#define MESSAGE_FORMAT_DANO_SWAPPED		'2BOF'
#define MESSAGE_FORMAT_HAIKU			'1FMH'
#define MESSAGE_FORMAT_HAIKU_SWAPPED	'HMF1'

namespace BPrivate {

class MessageAdapter {
public:
static	ssize_t			FlattenedSize(uint32 format, const BMessage *from);

static	status_t		Flatten(uint32 format, const BMessage *from,
							char *buffer, ssize_t *size);
static	status_t		Flatten(uint32 format, const BMessage *from,
							BDataIO *stream, ssize_t *size);

static	status_t		Unflatten(uint32 format, BMessage *into,
							const char *buffer);
static	status_t		Unflatten(uint32 format, BMessage *into,
							BDataIO *stream);

private:
static	status_t		_ConvertKMessage(const KMessage *from, BMessage *to);

static	ssize_t			_R5FlattenedSize(const BMessage *from);

static	status_t		_FlattenR5Message(uint32 format, const BMessage *from,
							char *buffer, ssize_t *size);

static	status_t		_UnflattenR5Message(uint32 format, BMessage *into,
							BDataIO *stream);
static	status_t		_UnflattenDanoMessage(uint32 format, BMessage *into,
							BDataIO *stream);
};

} // namespace BPrivate

#endif // _MESSAGE_ADAPTER_H_
