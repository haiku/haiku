/*
 * Copyright 2009-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */

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

#include <Controllable.h>

#include <string.h>

#include <OS.h>
#include <ParameterWeb.h>
#include <Roster.h>
#include <TimeSource.h>

#include <MediaDebug.h>
#include <DataExchange.h>
#include <Notifications.h>


namespace BPrivate { namespace media {

/*!	A helper class for the communication with the media server that
	takes care of large buffers that need an area.
*/
class ReceiveTransfer {
public:
	ReceiveTransfer(const area_request_data& request, const void* smallBuffer)
	{
		if (request.area == -1 && smallBuffer != NULL) {
			// small data transfer uses buffer in reply
			fArea = -1;
			fData = const_cast<void*>(smallBuffer);
				// The caller is actually responsible to enforce the const;
				// we don't touch the data.
		} else {
			// large data transfer, clone area
			fArea = clone_area("get parameter data clone", &fData,
				B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, request.area);
			if (fArea < B_OK) {
				ERROR("BControllabe: cloning area failed: %s\n",
					strerror(fArea));
				fData = NULL;
			}
		}
	}

	~ReceiveTransfer()
	{
		if (fArea >= B_OK)
			delete_area(fArea);
	}

	status_t InitCheck() const
	{
		return fData != NULL ? B_OK : fArea;
	}

	void* Data() const
	{
		return fData;
	}

private:
	area_id				fArea;
	void*				fData;
};

} // namespace media
} // namespace BPrivate

using BPrivate::media::ReceiveTransfer;


//	#pragma mark - protected


BControllable::~BControllable()
{
	CALLED();
	if (fSem > 0)
		delete_sem(fSem);

	delete fWeb;
}


//	#pragma mark - public


BParameterWeb*
BControllable::Web()
{
	CALLED();
	return fWeb;
}


bool
BControllable::LockParameterWeb()
{
	CALLED();
	if (fSem <= 0)
		return false;

	if (atomic_add(&fBen, 1) > 0) {
		status_t status;
		do {
			status = acquire_sem(fSem);
		} while (status == B_INTERRUPTED);

		return status == B_OK;
	}

	return true;
}


void
BControllable::UnlockParameterWeb()
{
	CALLED();
	if (fSem <= 0)
		return;

	if (atomic_add(&fBen, -1) > 1)
		release_sem(fSem);
}


//	#pragma mark - protected


BControllable::BControllable()
	: BMediaNode("this one is never called"),
	fWeb(NULL),
	fSem(create_sem(0, "BControllable lock")),
	fBen(0)
{
	CALLED();

	AddNodeKind(B_CONTROLLABLE);
}


status_t
BControllable::SetParameterWeb(BParameterWeb* web)
{
	CALLED();

	LockParameterWeb();
	BParameterWeb* old = fWeb;
	fWeb = web;

	if (fWeb != NULL) {
		// initialize BParameterWeb member variable
		fWeb->fNode = Node();
	}

	UnlockParameterWeb();

	if (old != web && web != NULL)
		BPrivate::media::notifications::WebChanged(Node());
	delete old;
	return B_OK;
}


status_t
BControllable::HandleMessage(int32 message, const void* data, size_t size)
{
	PRINT(4, "BControllable::HandleMessage %#lx, node %ld\n", message, ID());

	switch (message) {
		case CONTROLLABLE_GET_PARAMETER_DATA:
		{
			const controllable_get_parameter_data_request& request
				= *static_cast<const controllable_get_parameter_data_request*>(
					data);
			controllable_get_parameter_data_reply reply;

			ReceiveTransfer transfer(request, reply.raw_data);
			if (transfer.InitCheck() != B_OK) {
				request.SendReply(transfer.InitCheck(), &reply, sizeof(reply));
				return B_OK;
			}

			reply.size = request.request_size;
			status_t status = GetParameterValue(request.parameter_id,
				&reply.last_change, transfer.Data(), &reply.size);

			request.SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		case CONTROLLABLE_SET_PARAMETER_DATA:
		{
			const controllable_set_parameter_data_request& request
				= *static_cast<const controllable_set_parameter_data_request*>(
					data);
			controllable_set_parameter_data_reply reply;

			ReceiveTransfer transfer(request, request.raw_data);
			if (transfer.InitCheck() != B_OK) {
				request.SendReply(transfer.InitCheck(), &reply, sizeof(reply));
				return B_OK;
			}

			// NOTE: This is not very fair, but the alternative
			// would have been to mess with friends classes and
			// member variables.
			bigtime_t perfTime = 0;
			if (request.when == -1)
				perfTime = TimeSource()->Now();
			else
				perfTime = request.when;

			SetParameterValue(request.parameter_id, perfTime,
				transfer.Data(), request.size);
			request.SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case CONTROLLABLE_GET_PARAMETER_WEB:
		{
			const controllable_get_parameter_web_request& request
				= *static_cast<const controllable_get_parameter_web_request*>(
					data);
			controllable_get_parameter_web_reply reply;

			status_t status = B_OK;
			bool wasLocked = true;
			if (!LockParameterWeb()) {
				status = B_ERROR;
				wasLocked = false;
			}

			if (status == B_OK && fWeb != NULL) {
				if (fWeb->FlattenedSize() > request.max_size) {
					// parameter web too large
					reply.code = 0;
					reply.size = -1;
					status = B_OK;
				} else {
					ReceiveTransfer transfer(request, NULL);
					status = transfer.InitCheck();
					if (status == B_OK) {
						reply.code = fWeb->TypeCode();
						reply.size = fWeb->FlattenedSize();
						status = fWeb->Flatten(transfer.Data(), reply.size);
						if (status != B_OK) {
							ERROR("BControllable::HandleMessage "
								"CONTROLLABLE_GET_PARAMETER_WEB Flatten failed\n");
#if 0
						} else {
							printf("BControllable::HandleMessage CONTROLLABLE_GET_PARAMETER_WEB %ld bytes, 0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx\n",
								reply.size, ((uint32*)buffer)[0], ((uint32*)buffer)[1], ((uint32*)buffer)[2], ((uint32*)buffer)[3]);
#endif
						}
					}
				}
			} else {
				// no parameter web
				reply.code = 0;
				reply.size = 0;
			}
			if (wasLocked)
				UnlockParameterWeb();

			request.SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		case CONTROLLABLE_START_CONTROL_PANEL:
		{
			const controllable_start_control_panel_request* request
				= static_cast<const controllable_start_control_panel_request*>(
					data);
			controllable_start_control_panel_reply reply;
			BMessenger targetMessenger;
			status_t status = StartControlPanel(&targetMessenger);
			if (status != B_OK) {
				ERROR("BControllable::HandleMessage "
					"CONTROLLABLE_START_CONTROL_PANEL failed\n");
			}
			reply.result = status;
			reply.team = targetMessenger.Team();
			request->SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		default:
			return B_ERROR;
	}

	return B_OK;
}


status_t
BControllable::BroadcastChangedParameter(int32 id)
{
	CALLED();
	return BPrivate::media::notifications::ParameterChanged(Node(), id);
}


status_t
BControllable::BroadcastNewParameterValue(bigtime_t when, int32 id,
	void* newValue, size_t valueSize)
{
	CALLED();
	return BPrivate::media::notifications::NewParameterValue(Node(), id, when,
		newValue, valueSize);
}


status_t
BControllable::StartControlPanel(BMessenger* _messenger)
{
	CALLED();

	int32 internalId;
	BMediaAddOn* addon = AddOn(&internalId);
	if (!addon) {
		ERROR("BControllable::StartControlPanel not instantiated per AddOn\n");
		return B_ERROR;
	}

	image_id imageID = addon->ImageID();
	image_info info;
	if (imageID <= 0 || get_image_info(imageID, &info) != B_OK) {
		ERROR("BControllable::StartControlPanel Error accessing image\n");
		return B_BAD_VALUE;
	}

	entry_ref ref;
	if (get_ref_for_path(info.name, &ref) != B_OK) {
		ERROR("BControllable::StartControlPanel Error getting ref\n");
		return B_BAD_VALUE;
	}

	// The first argument is "node=id" with id meaning the media_node_id
	char arg[32];
	snprintf(arg, sizeof(arg), "node=%d", (int)ID());

	team_id team;
	if (be_roster->Launch(&ref, 1, (const char* const*)&arg, &team) != B_OK) {
		ERROR("BControllable::StartControlPanel Error launching application\n");
		return B_BAD_VALUE;
	}
	printf("BControllable::StartControlPanel done with id: %" B_PRId32 "\n",
		team);

	if (_messenger)
		*_messenger = BMessenger(NULL, team);

	return B_OK;
}


status_t
BControllable::ApplyParameterData(const void* value, size_t size)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BControllable::MakeParameterData(const int32* controls, int32 count,
	void* buffer, size_t* ioSize)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


//	#pragma mark - private


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
