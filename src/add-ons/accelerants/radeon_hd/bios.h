/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include <stdint.h>

// AtomBios includes
extern "C" {
#include "CD_Common_Types.h"
#include "CD_Definitions.h"
#include "atombios.h"
}


status_t AtomParser(void *parameterSpace, uint8_t index,
	void *handle, void *biosBase);

