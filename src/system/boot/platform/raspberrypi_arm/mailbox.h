/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PLATFORM_MAILBOX_H_
#define _PLATFORM_MAILBOX_H_

status_t write_mailbox(uint8 channel, uint32 value);
status_t read_mailbox(uint8 channel, uint32& value);

#endif // _PLATFORM_MAILBOX_H_

