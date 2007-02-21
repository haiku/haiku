/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DRAGGER_PRIVATE_H
#define _DRAGGER_PRIVATE_H


#include <Dragger.h>


class BDragger::Private {
	public:
		Private(BDragger* dragger) : fDragger(dragger) {}

		static void UpdateShowAllDraggers(bool visible)
			{ BDragger::_UpdateShowAllDraggers(visible); }

	private:
		BDragger*	fDragger;
};

#endif	// _DRAGGER_PRIVATE_H
