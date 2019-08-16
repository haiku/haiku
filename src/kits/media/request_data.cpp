/*
 * Copyright 2019, Ryan Leavengood
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <ServerInterface.h>

#include <set>

#include <Autolock.h>
#include <Locker.h>

#include <DataExchange.h>
#include <MediaDebug.h>

#include "PortPool.h"


namespace BPrivate {
namespace media {


request_data::request_data()
{
	reply_port = gPortPool->GetPort();
}


request_data::~request_data()
{
	gPortPool->PutPort(reply_port);
}


status_t
request_data::SendReply(status_t result, reply_data *reply,
	size_t replySize) const
{
	reply->result = result;
	// we cheat and use the (command_data *) version of SendToPort
	return SendToPort(reply_port, 0, reinterpret_cast<command_data *>(reply),
		replySize);
}


}	// namespace media
}	// namespace BPrivate
