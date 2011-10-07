/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
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
#define kMsgAddPersistentNetwork	'APnw'
#define kMsgJoinNetwork				'JNnw'
#define kMsgLeaveNetwork			'LVnw'


#endif	// _NET_SERVER_H
