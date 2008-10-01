/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PARSE_MESSAGE_H
#define _PARSE_MESSAGE_H

class BMessage;
class BDataIO;

status_t ParseMessageFromStream(BMessage **message, BDataIO &stream);

#endif /* _PARSE_MESSAGE_H */

