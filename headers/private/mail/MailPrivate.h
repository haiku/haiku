/*
 * Copyright 2011, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIL_PRIVATE_H
#define MAIL_PRIVATE_H


#include <Message.h>
#include <Path.h>


namespace BPrivate {

BPath default_mail_directory();
BPath default_mail_in_directory();
BPath default_mail_out_directory();

status_t WriteMessageFile(const BMessage& archive, const BPath& path,
	const char* name);

}	// namespace BPrivate


#endif	// MAIL_PRIVATE_H
