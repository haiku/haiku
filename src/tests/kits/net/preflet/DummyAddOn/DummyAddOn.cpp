#include <stdlib.h>

#include "NetworkSetupAddOn.h"

NetworkSetupAddOn*
get_nth_addon(image_id image, int index)
{
	if (index != 0)
		return NULL;
	return new NetworkSetupAddOn(image);
}
