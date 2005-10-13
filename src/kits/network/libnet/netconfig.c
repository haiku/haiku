/*
** Copyright 2005, Jérôme DUVAL. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <netconfig.h>

_EXPORT char *
_netconfig_find(const char *heading, const char *name, char *value,
                                         int nbytes)
{
	printf("_netconfig_find heading:%s, name:%s, value:%s, nbytes:%d\n", heading, name, value, nbytes);
	return find_net_setting(NULL, heading, name, value, nbytes);
}

