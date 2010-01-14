// NetFSDefs.cpp

#include "NetFSDefs.h"

const uint16 kDefaultInsecureConnectionPort	= 25432;
const uint16 kDefaultBroadcastPort			= 25433;
const uint16 kDefaultServerInfoPort			= 25434;

const bigtime_t kBroadcastingInterval		= 10000000;	// 10 s
const bigtime_t kMinBroadcastingInterval	= 1000000;	// 1 s
