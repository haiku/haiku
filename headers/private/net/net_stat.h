/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_STAT_H
#define NET_STAT_H


#include <OS.h>

#include <sys/socket.h>


#define NET_STAT_SOCKET		1
#define NET_STAT_PROTOCOL	2


typedef struct net_stat {
	int		family;
	int		type;
	int		protocol;
	char	state[B_OS_NAME_LENGTH];
	team_id	owner;
	struct	sockaddr_storage address;
	struct	sockaddr_storage peer;
	size_t	receive_queue_size;
	size_t	send_queue_size;
} net_stat;

#endif	// NET_STAT_H
