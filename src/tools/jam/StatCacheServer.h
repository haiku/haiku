// StatCacheServer.h

#ifndef STAT_CACHE_SERVER_H
#define STAT_CACHE_SERVER_H

#include <StorageDefs.h>

// common definitions used by server and client 

#define STAT_CACHE_SERVER_PORT_NAME	"stat_cache_server_port"

enum {
	STAT_CACHE_COMMAND_STAT		= 0,
	STAT_CACHE_COMMAND_READDIR	= 1,
};

typedef struct stat_cache_request {
	port_id		replyPort;
	int32		command;
	char		path[B_PATH_NAME_LENGTH];
} stat_cache_request;

typedef struct stat_cache_stat_reply {
	status_t	error;
	struct stat	st;
} stat_cache_stat_reply;

typedef struct stat_cache_readdir_reply {
	status_t	error;
	int32		entryCount;
	void		*clientData;		// used by the client only
	uint8		buffer[1];
} stat_cache_readdir_reply;

#endif	// STAT_CACHE_SERVER_H
