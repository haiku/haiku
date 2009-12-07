/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PORT_POOL_H
#define PORT_POOL_H


#include <set>

#include <Locker.h>


namespace BPrivate {


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


extern PortPool* gPortPool;


}	// namespace BPrivate


using BPrivate::gPortPool;


#endif	// PORT_POOL_H
