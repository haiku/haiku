/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <PortPool.h>

#include <Autolock.h>

#include <debug.h>


namespace BPrivate {


static PortPool sPortPool;
PortPool* gPortPool = &sPortPool;


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


}	// namespace BPrivate
