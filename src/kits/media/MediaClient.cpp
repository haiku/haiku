/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MediaClient.h"

#include <MediaConnection.h>

#include <MediaRoster.h>
#include <TimeSource.h>

#include "debug.h"


BMediaClient::BMediaClient(const char* name,
	media_type type, media_client_kinds kinds)
	:
	fLastID(0)
{
	CALLED();

	fNode = new BMediaClientNode(name, this, type);
	_Init();

	fClient.node = fNode->Node();
	fClient.kinds = kinds;
}


BMediaClient::~BMediaClient()
{
	CALLED();

	_Deinit();
}


const media_client&
BMediaClient::Client() const
{
	return fClient;
}


status_t
BMediaClient::InitCheck() const
{
	CALLED();

	return fInitErr;
}


media_client_kinds
BMediaClient::Kinds() const
{
	CALLED();

	return fClient.Kinds();
}


media_type
BMediaClient::MediaType() const
{
	CALLED();

	// Right now ConsumerType() and ProducerType() are the same.
	return fNode->ConsumerType();
}


status_t
BMediaClient::RegisterInput(BMediaInput* input)
{
	input->ConnectionRegistered(this, fLastID++);
	AddInput(input);
	return B_OK;
}


status_t
BMediaClient::RegisterOutput(BMediaOutput* output)
{
	output->ConnectionRegistered(this, fLastID++);
	AddOutput(output);
	return B_OK;
}


status_t
BMediaClient::Bind(BMediaInput* input, BMediaOutput* output)
{
	CALLED();

	if (input == NULL
		|| output == NULL)
		return B_ERROR;

	if (input->fOwner != this || output->fOwner != this)
		return B_ERROR;

	// TODO: Implement binding one input to more outputs.
	if (input->fBind != NULL
		|| output->fBind != NULL)
		return B_ERROR;

	input->fBind = output;
	output->fBind = input;
	return B_OK;
}


status_t
BMediaClient::Unbind(BMediaInput* input, BMediaOutput* output)
{
	CALLED();

	if (input == NULL
		|| input == NULL)
		return B_ERROR;

	if (input->fOwner != this || output->fOwner != this)
		return B_ERROR;

	input->fBind = NULL;
	output->fBind = NULL;
	return B_OK;
}


status_t
BMediaClient::Connect(BMediaConnection* ourConnection,
	BMediaConnection* theirConnection)
{
	CALLED();

	return Connect(ourConnection, theirConnection->Connection());
}


status_t
BMediaClient::Connect(BMediaConnection* ourConnection,
	const media_connection& theirConnection)
{
	CALLED();

	if (ourConnection->IsOutput() && theirConnection.IsInput())
		return _ConnectInput((BMediaOutput*)ourConnection, theirConnection);
	else if (ourConnection->IsInput() && theirConnection.IsOutput())
		return _ConnectOutput((BMediaInput*)ourConnection, theirConnection);

	return B_ERROR;
}


status_t
BMediaClient::Connect(BMediaConnection* connection,
	const media_client& client)
{
	CALLED();

	// TODO: implement this

	return B_ERROR;
}


status_t
BMediaClient::Disconnect()
{
	CALLED();

	for (int32 i = 0; i < CountInputs(); i++)
		InputAt(i)->Disconnect();

	for (int32 i = 0; i < CountOutputs(); i++)
		OutputAt(i)->Disconnect();

	return B_OK;
}


int32
BMediaClient::CountInputs() const
{
	CALLED();

	return fInputs.CountItems();
}


int32
BMediaClient::CountOutputs() const
{
	CALLED();

	return fOutputs.CountItems();
}


BMediaInput*
BMediaClient::InputAt(int32 index) const
{
	CALLED();

	return fInputs.ItemAt(index);
}


BMediaOutput*
BMediaClient::OutputAt(int32 index) const
{
	CALLED();

	return fOutputs.ItemAt(index);
}


BMediaInput*
BMediaClient::FindInput(const media_connection& input) const
{
	CALLED();

	if (!input.IsInput())
		return NULL;

	return FindInput(input.Destination());
}


BMediaOutput*
BMediaClient::FindOutput(const media_connection& output) const
{
	CALLED();

	if (!output.IsOutput())
		return NULL;

	return FindOutput(output.Source());
}


BMediaInput*
BMediaClient::FindInput(const media_destination& dest) const
{
	CALLED();

	for (int32 i = 0; i < CountInputs(); i++) {
		if (dest.id == InputAt(i)->Destination().id)
			return InputAt(i);
	}
	return NULL;
}


BMediaOutput*
BMediaClient::FindOutput(const media_source& source) const
{
	CALLED();

	for (int32 i = 0; i < CountOutputs(); i++) {
		if (source.id == OutputAt(i)->Source().id)
			return OutputAt(i);
	}
	return NULL;
}


bool
BMediaClient::IsRunning() const
{
	CALLED();

	return fRunning;
}


status_t
BMediaClient::Start(bool force)
{
	CALLED();

	status_t err = B_OK;
	for (int32 i = 0; i < CountOutputs(); i++) {
		media_node remoteNode = OutputAt(i)->Connection().RemoteNode();
		if (remoteNode.kind & B_TIME_SOURCE)
			err = BMediaRoster::CurrentRoster()->StartTimeSource(
				remoteNode, BTimeSource::RealTime());
		else
			err = BMediaRoster::CurrentRoster()->StartNode(
				remoteNode, fNode->TimeSource()->Now());
	}

	err = BMediaRoster::CurrentRoster()->StartNode(
		fNode->Node(), fNode->TimeSource()->Now());

	if (err == B_OK)
		fRunning = true;
	else
		fRunning = false;

	return err;
}


status_t
BMediaClient::Stop(bool force)
{
	CALLED();

	status_t err = BMediaRoster::CurrentRoster()->StopNode(
		fNode->Node(), fNode->TimeSource()->Now());

	if (err == B_OK)
		fRunning = false;
	else
		fRunning = true;

	return err;
}


status_t
BMediaClient::Seek(bigtime_t mediaTime,
	bigtime_t performanceTime)
{
	CALLED();

	return BMediaRoster::CurrentRoster()->SeekNode(fNode->Node(),
		mediaTime, performanceTime);
}


status_t
BMediaClient::Roll(bigtime_t start, bigtime_t stop, bigtime_t seek)
{
	CALLED();

	return BMediaRoster::CurrentRoster()->RollNode(fNode->Node(),
		start, stop, seek);
}


status_t
BMediaClient::Preroll()
{
	CALLED();

	return BMediaRoster::CurrentRoster()->PrerollNode(fNode->Node());
}


status_t
BMediaClient::SyncTo(bigtime_t performanceTime, bigtime_t timeout)
{
	CALLED();

	return BMediaRoster::CurrentRoster()->SyncToNode(fNode->Node(),
		performanceTime, timeout);
}


BMediaNode::run_mode
BMediaClient::RunMode() const
{
	CALLED();

	return fNode->RunMode();
}


status_t
BMediaClient::SetRunMode(BMediaNode::run_mode mode)
{
	CALLED();

	return BMediaRoster::CurrentRoster()->SetRunModeNode(fNode->Node(), mode);
}


status_t
BMediaClient::SetTimeSource(const media_client& timesource)
{
	CALLED();

	return BMediaRoster::CurrentRoster()->SetTimeSourceFor(fNode->Node().node,
		timesource.node.node);
}


void
BMediaClient::GetLatencyRange(bigtime_t* min, bigtime_t* max) const
{
	CALLED();

	*min = fMinLatency;
	*max = fMaxLatency;
}


void
BMediaClient::SetLatencyRange(bigtime_t min, bigtime_t max)
{
	CALLED();

	fMinLatency = min;
	fMaxLatency = max;
}


bigtime_t
BMediaClient::CurrentTime() const
{
	CALLED();

	return fCurrentTime;
}


void
BMediaClient::AddInput(BMediaInput* input)
{
	CALLED();

	fInputs.AddItem(input);
}


void
BMediaClient::AddOutput(BMediaOutput* output)
{
	CALLED();

	fOutputs.AddItem(output);
}


BMediaAddOn*
BMediaClient::AddOn(int32* id) const
{
	CALLED();

	return NULL;
}


void
BMediaClient::HandleStart(bigtime_t performanceTime)
{
}


void
BMediaClient::HandleStop(bigtime_t performanceTime)
{
}


void
BMediaClient::HandleSeek(bigtime_t mediaTime, bigtime_t performanceTime)
{
}


void
BMediaClient::HandleTimeWarp(bigtime_t realTime, bigtime_t performanceTime)
{
}


status_t
BMediaClient::HandleFormatSuggestion(media_type type, int32 quality,
	media_format* format)
{
	return B_ERROR;
}


status_t
BMediaClient::ConnectionReleased(BMediaConnection* connection)
{
	return B_OK;
}


status_t
BMediaClient::ConnectionDisconnected(BMediaConnection* connection)
{
	return B_OK;
}


void
BMediaClient::_Init()
{
	CALLED();

	BMediaRoster* roster = BMediaRoster::Roster(&fInitErr);
	if (fInitErr == B_OK && roster != NULL)
		fInitErr = roster->RegisterNode(fNode);
}


void
BMediaClient::_Deinit()
{
	CALLED();

	if (fRunning)
		Stop();

	Disconnect();

	fNode->Release();
}


status_t
BMediaClient::_ConnectInput(BMediaOutput* output,
	const media_connection& input)
{
	CALLED();

	if (input.Destination() == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;

	media_output ourOutput = output->Connection().MediaOutput();
	media_input theirInput = input.MediaInput();
	media_format format = output->AcceptedFormat();

	return BMediaRoster::CurrentRoster()->Connect(ourOutput.source,
		theirInput.destination, &format, &ourOutput, &theirInput,
		BMediaRoster::B_CONNECT_MUTED);
}


status_t
BMediaClient::_ConnectOutput(BMediaInput* input,
	const media_connection& output)
{
	CALLED();

	if (output.Source() == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	media_input ourInput = input->Connection().MediaInput();
	media_output theirOutput = output.MediaOutput();
	media_format format = input->AcceptedFormat();

	// TODO manage the node problems
	//fNode->ActivateInternalConnect(false);

	return BMediaRoster::CurrentRoster()->Connect(theirOutput.source,
		ourInput.destination, &format, &theirOutput, &ourInput,
		BMediaRoster::B_CONNECT_MUTED);
}


void BMediaClient::_ReservedMediaClient0() {}
void BMediaClient::_ReservedMediaClient1() {}
void BMediaClient::_ReservedMediaClient2() {}
void BMediaClient::_ReservedMediaClient3() {}
void BMediaClient::_ReservedMediaClient4() {}
void BMediaClient::_ReservedMediaClient5() {}
void BMediaClient::_ReservedMediaClient6() {}
void BMediaClient::_ReservedMediaClient7() {}
void BMediaClient::_ReservedMediaClient8() {}
void BMediaClient::_ReservedMediaClient9() {}
void BMediaClient::_ReservedMediaClient10() {}
