#ifndef _SERVER_REGION_H_
#define _SERVER_REGION_H_

#include "ServerRect.h"

class ServerRegion
{
public:
	ServerRegion(void);
	ServerRegion(const ServerRect rect);
	ServerRegion(const ServerRegion &reg);
	virtual ~ServerRegion(void);
	bool Intersects(ServerRect rect);
	void Set(ServerRect rect);
	void Include(ServerRect rect);
	void Exclude(ServerRect rect);
	void IntersectWith(ServerRect rect);
	void MakeEmpty(void);
	void Optimize(void);
};
#endif