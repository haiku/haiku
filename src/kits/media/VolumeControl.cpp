/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: VolumeControl.cpp
 *  DESCR: transitional private volume control functions
 ***********************************************************************/

#include <Message.h>
#include <Messenger.h>
#include "debug.h"
#include "VolumeControl.h"
#include "../server/headers/ServerInterface.h"

namespace MediaKitPrivate {

status_t GetMasterVolume(float *left, float *right)
{
	CALLED();
	BMessenger m(NEW_MEDIA_SERVER_SIGNATURE);
	BMessage msg(MEDIA_SERVER_GET_VOLUME);
	BMessage reply;
	status_t s;
	
	if (!m.IsValid())
		return B_ERROR;
	
	s = m.SendMessage(&msg,&reply);
	if (s != B_OK)
		return s;
		
	reply.FindFloat("left",left);
	reply.FindFloat("right",right);

	return (status_t)reply.what;
}

status_t SetMasterVolume(float left, float right)
{
	CALLED();
	BMessenger m(NEW_MEDIA_SERVER_SIGNATURE);
	BMessage msg(MEDIA_SERVER_SET_VOLUME);
	BMessage reply;
	status_t s;
	
	if (!m.IsValid())
		return B_ERROR;
	
	msg.AddFloat("left",left);
	msg.AddFloat("right",right);
	
	s = m.SendMessage(&msg,&reply);
	if (s != B_OK)
		return s;

	return (status_t)reply.what;
}

} //namespace MediaKitPrivate

