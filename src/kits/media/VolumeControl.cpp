/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: VolumeControl.cpp
 *  DESCR: transitional private volume control functions
 ***********************************************************************/

#include <Message.h>
#include <Messenger.h>
#include "debug.h"
#include "VolumeControl.h"
#include "ServerInterface.h"

namespace MediaKitPrivate {

status_t GetMasterVolume(float *left, float *right)
{
	return B_OK;
}

status_t SetMasterVolume(float left, float right)
{
	return B_OK;
}

} //namespace MediaKitPrivate

