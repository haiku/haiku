#ifndef STACK_AND_TILE_PRIVATE_H
#define STACK_AND_TILE_PRIVATE_H

namespace BPrivate {


const int32 kMagicSATIdentifier = 'SATI';


enum sat_messages {
	kAddWindowToStack,
	kRemoveWindowFromStack,
	kRemoveWindowFromStackAt,
	kCountWindowsOnStack,
	kWindowOnStackAt,
	kStackHasWindow
};

};


#endif
