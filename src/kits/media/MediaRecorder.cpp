/*
 * Copyright 2015, Hamish Morrison <hamishm53@gmail.com>
 * Copyright 2014-2016, Dario Casalinuovo
 * Copyright 1999, Be Incorporated
 * All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include <MediaRecorder.h>

#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <TimeSource.h>

#include "MediaDebug.h"
#include "MediaRecorderNode.h"


BMediaRecorder::BMediaRecorder(const char* name, media_type type)
	:
	fInitErr(B_OK),
	fConnected(false),
	fRunning(false),
	fReleaseOutputNode(false),
	fRecordHook(NULL),
	fNotifyHook(NULL),
	fNode(NULL),
	fBufferCookie(NULL)
{
	CALLED();

	BMediaRoster::Roster(&fInitErr);

	if (fInitErr == B_OK) {
		fNode = new(std::nothrow) BMediaRecorderNode(name, this, type);
		if (fNode == NULL)
			fInitErr = B_NO_MEMORY;

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


const media_format&
BMediaRecorder::AcceptedFormat() const
{
	CALLED();

	return fNode->AcceptedFormat();
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
BMediaRecorder::Connect(const media_format& format)
{
	CALLED();

	if (fInitErr != B_OK)
		return fInitErr;

	if (fConnected)
		return B_MEDIA_ALREADY_CONNECTED;

	status_t err = B_OK;
	media_node node;

	switch (format.type) {
		// switch on format for default
		case B_MEDIA_RAW_AUDIO:
			err = BMediaRoster::Roster()->GetAudioMixer(&node);
			break;
		case B_MEDIA_RAW_VIDEO:
		case B_MEDIA_ENCODED_VIDEO:
			err = BMediaRoster::Roster()->GetVideoInput(&node);
			break;
		// give up?
		default:
			return B_MEDIA_BAD_FORMAT;
	}

	if (err != B_OK)
		return err;

	fReleaseOutputNode = true;

	err = _Connect(node, NULL, format);

	if (err != B_OK) {
		BMediaRoster::Roster()->ReleaseNode(node);
		fReleaseOutputNode = false;
	}

	return err;
}


status_t
BMediaRecorder::Connect(const dormant_node_info& dormantNode,
	const media_format& format)
{
	CALLED();

	if (fInitErr != B_OK)
		return fInitErr;

	if (fConnected)
		return B_MEDIA_ALREADY_CONNECTED;

	media_node node;
	status_t err = BMediaRoster::Roster()->InstantiateDormantNode(dormantNode,
		&node, B_FLAVOR_IS_GLOBAL);

	if (err != B_OK)
		return err;

	fReleaseOutputNode = true;

	err = _Connect(node, NULL, format);

	if (err != B_OK) {
		BMediaRoster::Roster()->ReleaseNode(node);
		fReleaseOutputNode = false;
	}

	return err;
}


status_t
BMediaRecorder::Connect(const media_node& node,
	const media_output* output, const media_format* format)
{
	CALLED();

	if (fInitErr != B_OK)
		return fInitErr;

	if (fConnected)
		return B_MEDIA_ALREADY_CONNECTED;

	if (format == NULL && output != NULL)
		format = &output->format;

	return _Connect(node, output, *format);
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

	media_input ourInput;
	fNode->GetInput(&ourInput);

	// do the disconnect
	err = BMediaRoster::CurrentRoster()->Disconnect(
		fOutputNode.node, fOutputSource,
			fNode->Node().node, ourInput.destination);

	if (fReleaseOutputNode) {
		BMediaRoster::Roster()->ReleaseNode(fOutputNode);
		fReleaseOutputNode = false;
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

	if ((fOutputNode.kind & B_TIME_SOURCE) != 0)
		err = BMediaRoster::CurrentRoster()->StartTimeSource(
			fOutputNode, BTimeSource::RealTime());
	else
		err = BMediaRoster::CurrentRoster()->StartNode(
			fOutputNode, fNode->TimeSource()->Now());

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


const media_source&
BMediaRecorder::MediaSource() const
{
	CALLED();

	return fOutputSource;
}


const media_input
BMediaRecorder::MediaInput() const
{
	CALLED();

	media_input input;
	fNode->GetInput(&input);
	return input;
}


const media_format&
BMediaRecorder::Format() const
{
	CALLED();

	return fNode->AcceptedFormat();
}


status_t
BMediaRecorder::SetUpConnection(media_source outputSource)
{
	fOutputSource = outputSource;

	// Perform the connection
	media_node timeSource;
	if ((fOutputNode.kind & B_TIME_SOURCE) != 0)
		timeSource = fOutputNode;
	else
		BMediaRoster::Roster()->GetTimeSource(&timeSource);

	// Set time source
	return BMediaRoster::Roster()->SetTimeSourceFor(fNode->Node().node,
		timeSource.node);
}


status_t
BMediaRecorder::_Connect(const media_node& node,
	const media_output* output, const media_format& format)
{
	CALLED();

	status_t err = B_OK;
	media_format ourFormat = format;
	media_output ourOutput;

	if (fNode == NULL)
		return B_ERROR;

	fNode->SetAcceptedFormat(ourFormat);

	fOutputNode = node;

	// Figure out the output provided
	if (output != NULL) {
		ourOutput = *output;
	} else if (err == B_OK) {
		media_output outputs[10];
		int32 count = 10;

		err = BMediaRoster::Roster()->GetFreeOutputsFor(fOutputNode,
			outputs, count, &count, ourFormat.type);

		if (err != B_OK)
			return err;

		for (int i = 0; i < count; i++) {
			if (format_is_compatible(outputs[i].format, ourFormat)) {
				ourOutput = outputs[i];
				ourFormat = outputs[i].format;
				break;
			}
		}
	}

	if (ourOutput.source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	// Find our Node's free input
	media_input ourInput;
	fNode->GetInput(&ourInput);

	// Acknowledge the node that we already know
	// who is our producer node.
	fNode->ActivateInternalConnect(false);

	return BMediaRoster::CurrentRoster()->Connect(ourOutput.source,
		ourInput.destination, &ourFormat, &ourOutput, &ourInput,
		BMediaRoster::B_CONNECT_MUTED);
}


void BMediaRecorder::_ReservedMediaRecorder0() { }
void BMediaRecorder::_ReservedMediaRecorder1() { }
void BMediaRecorder::_ReservedMediaRecorder2() { }
void BMediaRecorder::_ReservedMediaRecorder3() { }
void BMediaRecorder::_ReservedMediaRecorder4() { }
void BMediaRecorder::_ReservedMediaRecorder5() { }
void BMediaRecorder::_ReservedMediaRecorder6() { }
void BMediaRecorder::_ReservedMediaRecorder7() { }

