/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

#include <messaging.h>


extern "C" status_t
send_message(const void *message, int32 messageSize,
	const messaging_target *targets, int32 targetCount)
{
	return B_NOT_SUPPORTED;
}
