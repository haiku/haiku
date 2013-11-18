/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ROUTESUPPORT_H_
#define ROUTESUPPORT_H_


#include <ObjectList.h>


namespace BPrivate {


status_t get_routes(const char* interfaceName,
		int family, BObjectList<route_entry>& routes);


}


#endif /* __ROUTESUPPORT_H_ */
