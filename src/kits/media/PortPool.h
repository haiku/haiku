/*
 * Copyright 2019, Ryan Leavengood
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PORT_POOL_H
#define _PORT_POOL_H


#include <ServerInterface.h>

#include <set>


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


extern PortPool* gPortPool;


}	// namespace media
}	// namespace BPrivate


#endif // _PORT_POOL_H