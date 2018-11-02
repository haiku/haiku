/*
 * Copyright 2001-2018 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler
 *		Jeremy Rand, jrand@magma.ca
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _DESKBAR_PRIVATE_H
#define _DESKBAR_PRIVATE_H


#ifndef kDeskbarSignature
#	define kDeskbarSignature "application/x-vnd.Be-TSKB"
#endif


static const uint32 kMsgIsAlwaysOnTop = 'gtop';
static const uint32 kMsgAlwaysOnTop = 'stop';
static const uint32 kMsgIsAutoRaise = 'grse';
static const uint32 kMsgAutoRaise = 'srse';
static const uint32 kMsgIsAutoHide = 'ghid';
static const uint32 kMsgAutoHide = 'shid';

static const uint32 kMsgAddView = 'icon';
static const uint32 kMsgAddAddOn = 'adon';
static const uint32 kMsgHasItem = 'exst';
static const uint32 kMsgGetItemInfo = 'info';
static const uint32 kMsgCountItems = 'cwnt';
static const uint32 kMsgMaxItemSize = 'mxsz';
static const uint32 kMsgRemoveItem = 'remv';
static const uint32 kMsgLocation = 'gloc';
static const uint32 kMsgIsExpanded = 'gexp';
static const uint32 kMsgSetLocation = 'sloc';
static const uint32 kMsgExpand = 'sexp';


#endif	/* _DESKBAR_PRIVATE_H */
