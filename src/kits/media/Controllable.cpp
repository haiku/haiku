/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: Controllable.cpp
 *  DESCR: 
 ***********************************************************************/
#include <OS.h>
#include <Controllable.h>
#include <ParameterWeb.h>
#include "debug.h"
#include "DataExchange.h"
#include "Notifications.h"

/*************************************************************
 * protected BControllable
 *************************************************************/

BControllable::~BControllable()
{
	CALLED();
	if (fSem > 0)
		delete_sem(fSem);
	if (fWeb)
		delete fWeb;
}

/*************************************************************
 * public BControllable
 *************************************************************/

BParameterWeb *
BControllable::Web()
{
	CALLED();
	BParameterWeb *temp;
	LockParameterWeb();
	temp = fWeb;
	UnlockParameterWeb();
	return temp;
}


bool
BControllable::LockParameterWeb()
{
	CALLED();
	status_t rv;
	if (fSem <= 0)
		return false;
	if (atomic_add(&fBen, 1) > 0) {
		while (B_INTERRUPTED == (rv = acquire_sem(fSem)))
			;
		return rv == B_OK;
	}
	return true;
}

/*************************************************************
 * protected BControllable
 *************************************************************/

void
BControllable::UnlockParameterWeb()
{
	CALLED();
	if (fSem <= 0)
		return;
	if (atomic_add(&fBen, -1) > 1)
		release_sem(fSem);
}


BControllable::BControllable() :
	BMediaNode("this one is never called"),
	fWeb(0),
	fSem(create_sem(0, "BControllable lock")),
	fBen(0)
{
	CALLED();

	AddNodeKind(B_CONTROLLABLE);
}


status_t
BControllable::SetParameterWeb(BParameterWeb *web)
{
	CALLED();
	BParameterWeb *old;
	LockParameterWeb();
	old = fWeb;
	fWeb = web;
	UnlockParameterWeb();
	if (old != web && web != 0)
		BPrivate::media::notifications::WebChanged(Node());
	if (old)
		delete old;
	return B_OK;
}


status_t
BControllable::HandleMessage(int32 message, const void *data, size_t size)
{
	INFO("BControllable::HandleMessage %#lx, node %ld\n", message, ID());

	status_t rv;
	switch (message) {
		case CONTROLLABLE_GET_PARAMETER_WEB:
		{
			const controllable_get_parameter_web_request *request = static_cast<const controllable_get_parameter_web_request *>(data);
			controllable_get_parameter_web_reply reply;
			bool waslocked = LockParameterWeb();
			if (fWeb != NULL && fWeb->FlattenedSize() > request->maxsize) {
				reply.code = 0;
				reply.size = -1; // parameter web too large
				rv = B_OK;
			} else if (fWeb != NULL && fWeb->FlattenedSize() <= request->maxsize) {
				void *buffer;
				area_id area;
				area = clone_area("cloned parameter web", &buffer, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, request->area);
				if (area < B_OK) {
					FATAL("BControllable::HandleMessage CONTROLLABLE_GET_PARAMETER_WEB clone_area failed\n");
					rv = B_ERROR;
				} else {
					reply.code = fWeb->TypeCode();
					reply.size = fWeb->FlattenedSize();
					rv = fWeb->Flatten(buffer, reply.size);
					if (rv != B_OK) {
						FATAL("BControllable::HandleMessage CONTROLLABLE_GET_PARAMETER_WEB Flatten failed\n");
					} else {
						printf("BControllable::HandleMessage CONTROLLABLE_GET_PARAMETER_WEB %ld bytes, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
							reply.size, ((uint32*)buffer)[0], ((uint32*)buffer)[1], ((uint32*)buffer)[2], ((uint32*)buffer)[3]);
					}
					delete_area(area);
				}
			} else {
				reply.code = 0;
				reply.size = 0; // no parameter web
				rv = B_OK;
			}
			if (waslocked)
				UnlockParameterWeb();
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}
				
	}
	return B_ERROR;
}


status_t
BControllable::BroadcastChangedParameter(int32 id)
{
	CALLED();
	return BPrivate::media::notifications::ParameterChanged(Node(), id);
}


status_t
BControllable::BroadcastNewParameterValue(bigtime_t when,
										  int32 id,
										  void *newValue,
										  size_t valueSize)
{
	CALLED();
	return BPrivate::media::notifications::NewParameterValue(Node(), id, when, newValue, valueSize);
}


status_t
BControllable::StartControlPanel(BMessenger *out_messenger)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BControllable::ApplyParameterData(const void *value,
								  size_t size)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BControllable::MakeParameterData(const int32 *controls,
								 int32 count,
								 void *buf,
								 size_t *ioSize)
{
	UNIMPLEMENTED();

	return B_ERROR;
}

/*************************************************************
 * private BControllable
 *************************************************************/

/*
private unimplemented 
BControllable::BControllable(const BControllable &clone)
BControllable & BControllable::operator=(const BControllable &clone)
*/

status_t BControllable::_Reserved_Controllable_0(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_1(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_2(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_3(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_4(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_5(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_6(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_7(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_8(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_9(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_10(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_11(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_12(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_13(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_14(void *) { return B_ERROR; }
status_t BControllable::_Reserved_Controllable_15(void *) { return B_ERROR; }


