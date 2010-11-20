/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef STACK_AND_TILE_PRIVATE_H
#define STACK_AND_TILE_PRIVATE_H


namespace BPrivate {


const int32 kMagicSATIdentifier = 'SATI';


enum sat_target {
	kStacking,
	kTiling
};


enum sat_messages {
	kAddWindowToStack,
	kRemoveWindowFromStack,
	kRemoveWindowFromStackAt,
	kCountWindowsOnStack,
	kWindowOnStackAt,
	kStackHasWindow,

	kSaveAllGroups,
	kRestoreGroup
};


}


#endif // STACK_AND_TILE_PRIVATE_H
