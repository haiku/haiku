/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MediaClient.h"

#include <MediaRoster.h>
#include <TimeSource.h>

#include "debug.h"


BMediaClient::BMediaClient(const char* name,
	media_type type, uint64 capability)
	:
	fCapabilities(capability)
{
	CALLED();

	fNode = new BMediaClientNode(name, this, type);
	_Init();
}


BMediaClient::~BMediaClient()
{
	CALLED();

	_Deinit();
}


status_t
BMediaClient::InitCheck() const
{
	CALLED();

	return fInitErr;
}


uint64
BMediaClient::Capabilities() const
{
	CALLED();

	return fCapabilities;
}


media_type
BMediaClient::Type() const
{
	CALLED();

	// Right now ConsumerType() and ProducerType() are the same.
	return fNode->ConsumerType();
}


BMediaConnection*
BMediaClient::BeginConnection(media_connection_kind kind)
{
	CALLED();

	BMediaConnection* conn = new BMediaConnection(this, kind);
	AddConnection(conn);
	return conn;
}


BMediaConnection*
BMediaClient::BeginConnection(const media_input& input)
{
	CALLED();

	BMediaConnection* conn = new BMediaConnection(this, input);
	AddConnection(conn);
	return conn;
}


BMediaConnection*
BMediaClient::BeginConnection(const media_output& output)
{
	CALLED();

	BMediaConnection* conn = new BMediaConnection(this, output);
	AddConnection(conn);
	return conn;
}


BMediaConnection*
BMediaClient::BeginConnection(BMediaConnection* connection)
{
	CALLED();

	if (connection->fOwner == this)
		return NULL;

	BMediaConnection* ret = NULL;
	if (_TranslateConnection(ret, connection) == B_OK) {
		AddConnection(ret);
		return ret;
	}
	return NULL;
}


status_t
BMediaClient::Bind(BMediaConnection* input, BMediaConnection* output)
{
	CALLED();

	if (input->fOwner != this || output->fOwner != this)
		return B_ERROR;
	else if (!input->IsInput() || !output->IsOutput())
		return B_ERROR;

	if (input == NULL
		|| input == NULL)
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
BMediaClient::Unbind(BMediaConnection* input, BMediaConnection* output)
{
	CALLED();

	if (input->fOwner != this || output->fOwner != this)
		return B_ERROR;
	else if (!input->IsInput() || !output->IsOutput())
		return B_ERROR;

	if (input == NULL
		|| input == NULL)
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

	if (ourConnection->IsOutput() && theirConnection->IsInput())
		return _ConnectInput(ourConnection, theirConnection);
	else if (ourConnection->IsInput() && theirConnection->IsOutput())
		return _ConnectOutput(ourConnection, theirConnection);

	return B_ERROR;
}


status_t
BMediaClient::Connect(BMediaConnection* connection,
	const dormant_node_info& dormantInfo)
{
	CALLED();

	media_node node;
	status_t err = BMediaRoster::CurrentRoster()->InstantiateDormantNode(
		dormantInfo, &node, B_FLAVOR_IS_GLOBAL);

	if (err != B_OK)
		return err;

	return Connect(connection, node);
}


status_t
BMediaClient::Connect(BMediaConnection* connection,
	const media_node& node)
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


status_t
BMediaClient::DisconnectConnection(BMediaConnection* conn)
{
	CALLED();

	return B_OK;
}


status_t
BMediaClient::ResetConnection(BMediaConnection* conn)
{
	CALLED();

	return B_OK;
}


status_t
BMediaClient::ReleaseConnection(BMediaConnection* conn)
{
	CALLED();

	return B_OK;
}


int32
BMediaClient::CountConnections() const
{
	CALLED();

	return fOutputs.CountItems()+fInputs.CountItems();
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


BMediaConnection*
BMediaClient::InputAt(int32 index) const
{
	CALLED();

	return fInputs.ItemAt(index);
}


BMediaConnection*
BMediaClient::OutputAt(int32 index) const
{
	CALLED();

	return fOutputs.ItemAt(index);
}


BMediaConnection*
BMediaClient::FindConnection(const media_destination& dest) const
{
	CALLED();

	for (int32 i = 0; i < CountInputs(); i++) {
		if (dest.id == InputAt(i)->Destination().id)
			return InputAt(i);
	}
	return NULL;
}


BMediaConnection*
BMediaClient::FindConnection(const media_source& source) const
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
		media_node remoteNode = OutputAt(i)->fRemoteNode;
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
BMediaClient::SetTimeSource(media_node timesource)
{
	CALLED();

	return BMediaRoster::CurrentRoster()->SetTimeSourceFor(fNode->Node().node,
		timesource.node);
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
BMediaClient::OfflineTime() const
{
	CALLED();

	return fOfflineTime;
}


bigtime_t
BMediaClient::PerformanceTime() const
{
	CALLED();

	return fPerformanceTime;
}


status_t
BMediaClient::SendBuffer(BMediaConnection* connection, BBuffer* buffer)
{
	CALLED();

	return fNode->SendBuffer(buffer, connection);
}


void
BMediaClient::AddConnection(BMediaConnection* connection)
{
	CALLED();

	if (connection->IsInput())
		fInputs.AddItem(connection);
	else
		fOutputs.AddItem(connection);
}


void
BMediaClient::BufferReceived(BMediaConnection* connection,
	BBuffer* buffer)
{
	CALLED();

	if (connection->fProcessHook != NULL)
		connection->fProcessHook(connection, buffer);
}


BMediaAddOn*
BMediaClient::AddOn(int32* id) const
{
	CALLED();

	return NULL;
}


void
BMediaClient::SetNotificationHook(notify_hook notifyHook, void* cookie)
{
	CALLED();

	fNotifyHook = notifyHook;
	fNotifyCookie = cookie;
}


void
BMediaClient::_Init()
{
	CALLED();

	fNotifyHook = NULL;
	fNotifyCookie = NULL;

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
BMediaClient::_TranslateConnection(BMediaConnection* dest,
	BMediaConnection* source)
{
	CALLED();

	return B_ERROR;
}


status_t
BMediaClient::_Connect(BMediaConnection* connection,
	media_node node)
{
	CALLED();

}


status_t
BMediaClient::_ConnectInput(BMediaConnection* output,
	BMediaConnection* input)
{
	CALLED();

	return B_ERROR;
}


status_t
BMediaClient::_ConnectOutput(BMediaConnection* input,
	BMediaConnection* output)
{
	CALLED();

	if (output->Source() == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	media_input ourInput;
	media_output theirOutput;
	media_format format = input->AcceptedFormat();

	input->BuildMediaInput(&ourInput);
	output->BuildMediaOutput(&theirOutput);

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
