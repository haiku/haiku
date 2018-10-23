/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <ServerInterface.h>

#include <set>

#include <Autolock.h>
#include <Locker.h>

#include <DataExchange.h>
#include <MediaDebug.h>


namespace BPrivate {
namespace media {


class PortPool : BLocker {
public:
								PortPool();
								~PortPool();

			port_id				GetPort();
			void				PutPort(port_id port);

private:
			typedef std::set<port_id> PortSet;

			PortSet				fPool;
};


static PortPool sPortPool;


PortPool::PortPool()
	:
	BLocker("port pool")
{
}


PortPool::~PortPool()
{
	PortSet::iterator iterator = fPool.begin();

	for (; iterator != fPool.end(); iterator++)
		delete_port(*iterator);
}


port_id
PortPool::GetPort()
{
	BAutolock _(this);

	if (fPool.empty())
		return create_port(1, "media reply port");

	port_id port = *fPool.begin();
	fPool.erase(port);

	ASSERT(port >= 0);
	return port;
}


void
PortPool::PutPort(port_id port)
{
	ASSERT(port >= 0);

	BAutolock _(this);

	try {
		fPool.insert(port);
	} catch (std::bad_alloc& exception) {
		delete_port(port);
	}
}


// #pragma mark -


request_data::request_data()
{
	reply_port = sPortPool.GetPort();
}


request_data::~request_data()
{
	sPortPool.PutPort(reply_port);
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
