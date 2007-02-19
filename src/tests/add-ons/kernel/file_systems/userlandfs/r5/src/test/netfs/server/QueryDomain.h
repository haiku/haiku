// QueryDomain.h

#ifndef NET_FS_QUERY_DOMAIN_H
#define NET_FS_QUERY_DOMAIN_H

#include "HashMap.h"
#include "Locker.h"
#include "NodeHandle.h"

class NodeMonitoringEvent;
class Volume;


// QueryDomain
class QueryDomain {
public:
	virtual						~QueryDomain();

	virtual	bool				QueryDomainIntersectsWith(Volume* volume) = 0;

	virtual	void				ProcessQueryEvent(
									NodeMonitoringEvent* event) = 0;
};


#endif	// NET_FS_QUERY_DOMAIN_H
