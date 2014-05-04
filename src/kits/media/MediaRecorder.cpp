/*
 * Copyright 1999, Be Incorporated
 * Copyright 2014, Dario Casalinuovo
 * All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include "MediaRecorder.h"

#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <TimeSource.h>

#include "MediaDebug.h"
#include "MediaRecorderNode.h"


class MediaNodeReleaser {
public:
				MediaNodeReleaser(media_node& mediaNode,
					bool release = true)
					:
					fNode(mediaNode),
					fRelease(release)
				{
				}

				~MediaNodeReleaser()
				{
					if (fRelease)
						BMediaRoster::Roster()->ReleaseNode(fNode);
				}

	void		SetTo(media_node& node)
				{
					fNode = node;
				}

	void		SetRelease(bool release)
				{
					fRelease = release;
				}

private:
	media_node& fNode;
	bool		fRelease;
};


BMediaRecorder::BMediaRecorder(const char* name, media_type type)
	:
	fInitErr(B_OK),
	fConnected(false),
	fRunning(false),
	fTimeSource(NULL),
	fRecordHook(NULL),
	fNotifyHook(NULL),
	fNode(NULL),
	fBufferCookie(NULL)
{
	CALLED();

	BMediaRoster::Roster(&fInitErr);

	if (fInitErr == B_OK) {
		fNode = new BMediaRecorderNode(name, this, type);
		fInitErr = BMediaRoster::CurrentRoster()->RegisterNode(fNode);
	}
}


BMediaRecorder::~BMediaRecorder()
{
	CALLED();

	if (fNode != NULL) {
		Stop();
		Disconnect();
		fNode->Release();
	}

	if (fTimeSource != NULL)
		fTimeSource->Release();

}


status_t
BMediaRecorder::InitCheck() const
{
	CALLED();

	return fInitErr;
}


void
BMediaRecorder::SetAcceptedFormat(const media_format& format)
{
	CALLED();

	fNode->SetAcceptedFormat(format);
}


status_t
BMediaRecorder::SetHooks(ProcessFunc recordFunc, NotifyFunc notifyFunc,
	void* cookie)
{
	CALLED();

	fRecordHook = recordFunc;
	fNotifyHook = notifyFunc;
	fBufferCookie = cookie;

	return B_OK;
}


void
BMediaRecorder::BufferReceived(void* buffer, size_t size,
	const media_header& header)
{
	CALLED();

	if (fRecordHook) {
		(*fRecordHook)(fBufferCookie, header.start_time,
			buffer, size, Format());
	}
}


status_t
BMediaRecorder::Connect(const media_format& format, uint32 flags)
{
	CALLED();

	if (fInitErr != B_OK)
		return fInitErr;

	if (fConnected)
		return B_MEDIA_ALREADY_CONNECTED;

	return _Connect(&format, flags, NULL, NULL, NULL);
}


status_t
BMediaRecorder::Connect(const dormant_node_info& dormantInfo,
	const media_format* format, uint32 flags)
{
	CALLED();

	if (fInitErr != B_OK)
		return fInitErr;

	if (fConnected)
		return B_MEDIA_ALREADY_CONNECTED;

	return _Connect(format, flags, &dormantInfo, NULL, NULL);
}


status_t
BMediaRecorder::Connect(const media_node& node,
	const media_output* useOutput, const media_format* format,
	uint32 flags)
{
	CALLED();

	if (fInitErr != B_OK)
		return fInitErr;

	if (fConnected)
		return B_MEDIA_ALREADY_CONNECTED;

	return _Connect(format, flags, NULL, &node, useOutput);
}


status_t
BMediaRecorder::Disconnect()
{
	CALLED();

	status_t err = B_OK;

	if (fInitErr != B_OK)
		return fInitErr;

	if (!fConnected)
		return B_MEDIA_NOT_CONNECTED;

	if (!fNode)
		return B_ERROR;

	if (fRunning)
		err = Stop();

	if (err != B_OK)
		return err;

	// do the disconnect
	err = BMediaRoster::CurrentRoster()->Disconnect(
		fOutputNode.node, fOutput.source,
			fNode->Node().node, fInput.destination);

	BMediaRoster::Roster()->ReleaseNode(fOutputNode);

	if (fTimeSource != NULL) {
		fTimeSource->Release();
		fTimeSource = NULL;
	}

	fConnected = false;
	fRunning = false;

	return err;
}


status_t
BMediaRecorder::Start(bool force)
{
	CALLED();

	if (fInitErr != B_OK)
		return fInitErr;

	if (!fConnected)
		return B_MEDIA_NOT_CONNECTED;

	if (fRunning && !force)
		return EALREADY;

	if (!fNode)
		return B_ERROR;

	// start node here
	status_t err = B_OK;

	if (fNode->Node().kind & B_TIME_SOURCE)
		err = BMediaRoster::CurrentRoster()->StartTimeSource(
			fOutputNode, BTimeSource::RealTime());
	else
		err = BMediaRoster::CurrentRoster()->StartNode(
			fOutputNode, fTimeSource->Now());

	// then un-mute it
	if (err == B_OK) {
		fNode->SetDataEnabled(true);
		fRunning = true;
	} else
		fRunning = false;

	return err;
}


status_t
BMediaRecorder::Stop(bool force)
{
	CALLED();

	if (fInitErr != B_OK)
		return fInitErr;

	if (!fRunning && !force)
		return EALREADY;

	if (!fNode)
		return B_ERROR;

	// should have the Node mute the output here
	fNode->SetDataEnabled(false);

	fRunning = false;

	return BMediaRoster::CurrentRoster()->StopNode(fNode->Node(), 0);
}


bool
BMediaRecorder::IsRunning() const
{
	CALLED();

	return fRunning;
}


bool
BMediaRecorder::IsConnected() const
{
	CALLED();

	return fConnected;
}


const media_output&
BMediaRecorder::MediaOutput() const
{
	CALLED();

	return fOutput;
}


const media_input&
BMediaRecorder::MediaInput() const
{
	CALLED();

	return fInput;
}


const media_format&
BMediaRecorder::Format() const
{
	CALLED();

	return fOutput.format;
}


status_t
BMediaRecorder::_Connect(const media_format* format,
	uint32 flags, const dormant_node_info* dormantNode,
	const media_node* mediaNode, const media_output* output)
{
	CALLED();

	status_t err = B_OK;
	media_format ourFormat;
	media_node node;
	MediaNodeReleaser away(node, false);
	media_output ourOutput;

	// argument checking and set-up
	if (format != NULL)
		ourFormat = *format;

	if (fNode == NULL)
		return B_ERROR;

	if (mediaNode == NULL && output != NULL)
		return B_MISMATCHED_VALUES;

	if (dormantNode != NULL && (mediaNode != NULL || output != NULL))
		return B_MISMATCHED_VALUES;

	if (format == NULL && output != NULL)
		ourFormat = output->format;

	fNode->SetAcceptedFormat(ourFormat);

	// figure out the node to instantiate
	if (dormantNode != NULL) {

		err = BMediaRoster::Roster()->InstantiateDormantNode(
			*dormantNode, &node, B_FLAVOR_IS_GLOBAL);

		away.SetRelease(true);

	} else if (mediaNode != NULL) {
		node = *mediaNode;
	} else {
		switch (ourFormat.type) {

			// switch on format for default
			case B_MEDIA_RAW_AUDIO:
				err = BMediaRoster::Roster()->GetAudioInput(&node);
				away.SetRelease(true);
				break;

			case B_MEDIA_RAW_VIDEO:
			case B_MEDIA_ENCODED_VIDEO:
				err = BMediaRoster::Roster()->GetVideoInput(&node);
				away.SetRelease(true);
				break;

			// give up?
			default:
				return B_MEDIA_BAD_FORMAT;
				break;
		}
	}

	fOutputNode = node;

	// figure out the output provided

	if (output != NULL) {
		ourOutput = *output;
	} else if (err == B_OK) {

		media_output outputs[10];
		int32 count = 10;

		err = BMediaRoster::Roster()->GetFreeOutputsFor(node,
			outputs, count, &count, ourFormat.type);

		if (err != B_OK)
			return err;

		err = B_MEDIA_BAD_SOURCE;

		for (int i = 0; i < count; i++) {
			if (format_is_compatible(outputs[i].format, ourFormat)) {
				ourOutput = outputs[i];
				err = B_OK;
				ourFormat = outputs[i].format;
				break;
			}

		}

	} else {
		return err;
	}

	if (ourOutput.source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	// find our Node's free input
	media_input ourInput;

	err = fNode->GetInput(&ourInput);
	if (err != B_OK)
		return err;

	media_node time_source;
	if (node.kind & B_TIME_SOURCE)
		time_source = node;
	else
		BMediaRoster::Roster()->GetSystemTimeSource(&time_source);

	// set time source
	BMediaRoster::Roster()->SetTimeSourceFor(
		fNode->Node().node, time_source.node);

	fTimeSource = BMediaRoster::CurrentRoster()->MakeTimeSourceFor(
		fNode->Node());

	if (err != B_OK)
		return err;

	// start the recorder node (it's always running)
	err = BMediaRoster::CurrentRoster()->StartNode(
		fOutputNode, fTimeSource->Now());

	if (err != B_OK)
		return err;

	// perform the connection
	fOutput = ourOutput;
	fInput = ourInput;

	err = BMediaRoster::CurrentRoster()->Connect(fOutput.source,
		fInput.destination, &ourFormat, &fOutput, &fInput,
		BMediaRoster::B_CONNECT_MUTED);

	if (err != B_OK)
		return err;

	fConnected = true;
	away.SetRelease(false);

	return err;
}
