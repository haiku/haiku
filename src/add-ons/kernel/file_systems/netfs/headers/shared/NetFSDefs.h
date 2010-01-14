// NetFSDefs.h

#ifndef NET_FS_DEFS_H
#define NET_FS_DEFS_H

#include <SupportDefs.h>

extern const uint16 kDefaultInsecureConnectionPort;
extern const uint16 kDefaultBroadcastPort;
extern const uint16 kDefaultServerInfoPort;

struct BroadcastMessage {
	uint32	magic;
	int32	protocolVersion;
	uint32	message;
};

// version of the general NetFS protocol (the request stuff)
enum {
	NETFS_PROTOCOL_VERSION			= 1,
};

// BroadcastMessage::magic value
enum {
	BROADCAST_MESSAGE_MAGIC			= 'BcMc',
};

// BroadcastMessage::message values
enum {
	BROADCAST_MESSAGE_SERVER_TICK	= 0,	// periodical tick
	BROADCAST_MESSAGE_SERVER_UPDATE	= 1,	// shares have changed
	BROADCAST_MESSAGE_CLIENT_HELLO	= 2,	// client requests server tick
};

extern const bigtime_t kBroadcastingInterval;
extern const bigtime_t kMinBroadcastingInterval;

#endif	// NET_FS_DEFS_H
