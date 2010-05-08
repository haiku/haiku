/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef GLOBALS_H
#define GLOBALS_H


#include <Messenger.h>
#include <String.h>


BString ActivePrinterName();
status_t GetPrinterServerMessenger(BMessenger& msgr);

#endif // GLOBALS_H

