/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef COMMON_PROPERTY_IDS_H
#define COMMON_PROPERTY_IDS_H

#include <SupportDefs.h>

enum {
	PROPERTY_NAME						= 'name',

	PROPERTY_OPACITY					= 'alpa',
	PROPERTY_COLOR						= 'colr',

	PROPERTY_WIDTH						= 'wdth',
	PROPERTY_HEIGHT						= 'hght',

	PROPERTY_CAP_MODE					= 'cpmd',
	PROPERTY_JOIN_MODE					= 'jnmd',
	PROPERTY_MITER_LIMIT				= 'mtlm',
};


const char*		name_for_id(int32 id);

#endif // COMMON_PROPERTY_IDS_H


