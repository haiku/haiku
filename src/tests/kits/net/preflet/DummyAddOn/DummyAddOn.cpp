/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include <stdlib.h>

#include "NetworkSetupAddOn.h"

NetworkSetupAddOn*
get_nth_addon(image_id image, int index)
{
	if (index != 0)
		return NULL;
	return new NetworkSetupAddOn(image);
}
