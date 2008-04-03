/*
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <OS.h>
#include <Controllable.h>
#include <ParameterWeb.h>
#include <Roster.h>
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

	if (fWeb)
		fWeb->mNode = Node(); // initialize BParameterWeb member variable
		
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
	PRINT(4, "BControllable::HandleMessage %#lx, node %ld\n", message, ID());

	status_t rv;
	switch (message) {
		case CONTROLLABLE_GET_PARAMETER_DATA:
		{
			const controllable_get_parameter_data_request *request = static_cast<const controllable_get_parameter_data_request *>(data);
			controllable_get_parameter_data_reply reply;
			area_id area;
			void *data;
			
			if (request->area == -1) {
				// small data transfer uses buffer in reply
				area = -1;
				data = reply.rawdata;
			} else {
				// large data transfer, clone area
				area = clone_area("get parameter data clone", &data, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, request->area);
				if (area < B_OK) {
					ERROR("CONTROLLABLE_GET_PARAMETER_DATA cloning area failed\n");
					request->SendReply(B_NO_MEMORY, &reply, sizeof(reply));
					return B_OK;
				}
			}
			reply.size = request->requestsize;
			rv = GetParameterValue(request->parameter_id, &reply.last_change, data, &reply.size);
			if (area != -1)
				delete_area(area);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}

		case CONTROLLABLE_SET_PARAMETER_DATA:
		{
			const controllable_set_parameter_data_request *request = static_cast<const controllable_set_parameter_data_request *>(data);
			controllable_set_parameter_data_reply reply;
			area_id area;
			const void *data;
			
			if (request->area == -1) {
				// small data transfer uses buffer in request
				area = -1;
				data = request->rawdata;
			} else {
				// large data transfer, clone area
				area = clone_area("set parameter data clone", (void **)&data, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, request->area);
				if (area < B_OK) {
					ERROR("CONTROLLABLE_SET_PARAMETER_DATA cloning area failed\n");
					request->SendReply(B_NO_MEMORY, &reply, sizeof(reply));
					return B_OK;
				}
			}
			SetParameterValue(request->parameter_id, request->when, data, request->size);
			if (area != -1)
				delete_area(area);
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}
	
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
					ERROR("BControllable::HandleMessage CONTROLLABLE_GET_PARAMETER_WEB clone_area failed\n");
					rv = B_ERROR;
				} else {
					reply.code = fWeb->TypeCode();
					reply.size = fWeb->FlattenedSize();
					rv = fWeb->Flatten(buffer, reply.size);
					if (rv != B_OK) {
						ERROR("BControllable::HandleMessage CONTROLLABLE_GET_PARAMETER_WEB Flatten failed\n");
					} else {
						printf("BControllable::HandleMessage CONTROLLABLE_GET_PARAMETER_WEB %ld bytes, 0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx\n",
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

		case CONTROLLABLE_START_CONTROL_PANEL:
		{
			const controllable_start_control_panel_request *request = static_cast<const controllable_start_control_panel_request*>(data);
			controllable_start_control_panel_reply reply;
			BMessenger targetMessenger;
			rv = StartControlPanel(&targetMessenger);
			if (rv != B_OK) {
				ERROR("BControllable::HandleMessage CONTROLLABLE_START_CONTROL_PANEL failed\n");
			}
			reply.result = rv;
			reply.team = targetMessenger.Team();
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
	CALLED();

	int32 internalId;
	BMediaAddOn* addon = AddOn(&internalId);
	if (!addon) {
		ERROR("BControllable::StartControlPanel not instantiated per AddOn\n");
		return B_ERROR;
	}

	image_id imageId = addon->ImageID();
	image_info info;
	if ((imageId <= 0) || (get_image_info(imageId, &info) != B_OK)) {
		ERROR("BControllable::StartControlPanel Error accessing image\n");
		return B_BAD_VALUE;
	}

	team_id id;
	entry_ref ref;

	if (BEntry(info.name).GetRef(&ref) != B_OK) {
		ERROR("BControllable::StartControlPanel Error getting ref\n");
		return B_BAD_VALUE;
	}

	// The first argument is "node=id" with id meaning the media_node_id
	char *arg = (char*) malloc(10);
	sprintf(arg, "node=%d" , (int) ID());

	if (be_roster->Launch(&ref, 1, &arg, &id) != B_OK) {
		free(arg);
		ERROR("BControllable::StartControlPanel Error launching application\n");
		return B_BAD_VALUE;
	}
	printf("BControllable::StartControlPanel done with id: %ld\n", id);
	free(arg);

	if (out_messenger)
		*out_messenger = BMessenger(0, id);

	return B_OK;
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


