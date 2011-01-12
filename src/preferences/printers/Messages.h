/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Philippe Houdoin
 */
#ifndef _MESSAGES_H
#define _MESSAGES_H


#include <SupportDefs.h>


const uint32 kMsgAddPrinter         = 'AddP';
const uint32 kMsgAddPrinterClosed   = 'APCl';
const uint32 kMsgRemovePrinter      = 'RemP';
const uint32 kMsgMakeDefaultPrinter = 'MDfP';
const uint32 kMsgPrinterSelected    = 'PSel';
const uint32 kMsgCancelJob          = 'CncJ';
const uint32 kMsgRestartJob         = 'RstJ';
const uint32 kMsgJobSelected        = 'JSel';
const uint32 kMsgPrintTestPage		= 'PtPg';

#endif // _MESSAGES_H
