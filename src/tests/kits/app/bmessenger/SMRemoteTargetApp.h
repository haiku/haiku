// SMRemoteTargetApp.h

#ifndef SM_REMOTE_TARGET_APP_H
#define SM_REMOTE_TARGET_APP_H

#include <Messenger.h>

enum {
	SMRT_INIT						= 'init',
	SMRT_QUIT						= 'quit',
	SMRT_GET_READY					= 'gtrd',
	SMRT_DELIVERY_SUCCESS_REQUEST	= 'dsrq',
	SMRT_DELIVERY_SUCCESS_REPLY		= 'dsre',
};

struct smrt_init {
	port_id		port;
	BMessenger	messenger;
};

struct smrt_delivery_success {
	bool success;
};

struct smrt_get_ready {
	bigtime_t unblock_time;
	bigtime_t reply_delay;
};

#endif	// SM_REMOTE_TARGET_APP_H
