/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _NET_SERVER_H
#define _NET_SERVER_H


#define kNetServerSignature		"application/x-vnd.haiku-net_server"

#define kMsgConfigureInterface		'COif'
#define kMsgConfigureResolver		'COrs'
#define kMsgCountPersistentNetworks	'CPnw'
#define kMsgGetPersistentNetwork	'GPnw'
#define kMsgAddPersistentNetwork	'APnw'
#define kMsgRemovePersistentNetwork	'RPnw'
#define kMsgJoinNetwork				'JNnw'
#define kMsgLeaveNetwork			'LVnw'
#define kMsgAutoJoinNetwork			'AJnw'
#define kMsgIsServiceRunning		'IRsv'


#endif	// _NET_SERVER_H
