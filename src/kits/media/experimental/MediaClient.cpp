/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MediaClient.h"

#include <MediaConnection.h>

#include <MediaRoster.h>
#include <TimeSource.h>

#include "MediaClientNode.h"

#include "MediaDebug.h"


namespace BPrivate { namespace media {


class ConnReleaser {
public:
	ConnReleaser(BMediaConnection* conn)
		:
		fConn(conn) {}

	virtual ~ConnReleaser()
	{
		fConn->Release();
	}

	bool operator== (const ConnReleaser &c1)
	{
		return c1.fConn == this->fConn;
	}

protected:
	BMediaConnection* Obj() const
	{
		return fConn;
	}

private:
	BMediaConnection* fConn;
};


class InputReleaser : public ConnReleaser {
public:
	InputReleaser(BMediaInput* input)
		:
		ConnReleaser(input) {}

	BMediaInput* Obj() const
	{
		return dynamic_cast<BMediaInput*>(ConnReleaser::Obj());
	}
};


class OutputReleaser : public ConnReleaser {
public:
	OutputReleaser(BMediaOutput* output)
		:
		ConnReleaser(output) {}

	BMediaOutput* Obj() const
	{
		return dynamic_cast<BMediaOutput*>(ConnReleaser::Obj());
	}
};


}
}


BMediaClient::BMediaClient(const char* name,
	media_type type, media_client_kinds kinds)
	:
	fLastID(-1)
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
	input->_ConnectionRegistered(this, ++fLastID);
	_AddInput(input);
	return B_OK;
}


status_t
BMediaClient::RegisterOutput(BMediaOutput* output)
{
	output->_ConnectionRegistered(this, ++fLastID);
	_AddOutput(output);
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

	BMediaOutput* output = dynamic_cast<BMediaOutput*>(ourConnection);
	if (output != NULL && theirConnection.IsInput())
		return _ConnectInput(output, theirConnection);

	BMediaInput* input = dynamic_cast<BMediaInput*>(ourConnection);
	if (input != NULL && theirConnection.IsOutput())
		return _ConnectOutput(input, theirConnection);

	return B_ERROR;
}


status_t
BMediaClient::Connect(BMediaConnection* connection,
	const media_client& client)
{
	UNIMPLEMENTED();

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

	return fInputs.ItemAt(index)->Obj();
}


BMediaOutput*
BMediaClient::OutputAt(int32 index) const
{
	CALLED();

	return fOutputs.ItemAt(index)->Obj();
}


BMediaInput*
BMediaClient::FindInput(const media_connection& input) const
{
	CALLED();

	if (!input.IsInput())
		return NULL;

	return _FindInput(input.destination);
}


BMediaOutput*
BMediaClient::FindOutput(const media_connection& output) const
{
	CALLED();

	if (!output.IsOutput())
		return NULL;

	return _FindOutput(output.source);
}


bool
BMediaClient::IsStarted() const
{
	CALLED();

	return fRunning;
}


void
BMediaClient::ClientRegistered()
{
	CALLED();
}


status_t
BMediaClient::Start()
{
	CALLED();

	status_t err = B_OK;
	for (int32 i = 0; i < CountOutputs(); i++) {
		media_node remoteNode = OutputAt(i)->Connection().remote_node;
		if (remoteNode.kind & B_TIME_SOURCE)
			err = BMediaRoster::CurrentRoster()->StartTimeSource(
				remoteNode, BTimeSource::RealTime());
		else
			err = BMediaRoster::CurrentRoster()->StartNode(
				remoteNode, fNode->TimeSource()->Now());
	}

	if (err != B_OK)
		return err;

	return BMediaRoster::CurrentRoster()->StartNode(
		fNode->Node(), fNode->TimeSource()->Now());
}


status_t
BMediaClient::Stop()
{
	CALLED();

	return BMediaRoster::CurrentRoster()->StopNode(
		fNode->Node(), fNode->TimeSource()->Now());
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


bigtime_t
BMediaClient::CurrentTime() const
{
	CALLED();

	return fCurrentTime;
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
	fRunning = true;
}


void
BMediaClient::HandleStop(bigtime_t performanceTime)
{
	fRunning = false;
}


void
BMediaClient::HandleSeek(bigtime_t mediaTime, bigtime_t performanceTime)
{
}


status_t
BMediaClient::FormatSuggestion(media_type type, int32 quality,
	media_format* format)
{
	return B_ERROR;
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

	if (IsStarted())
		Stop();

	Disconnect();

	// This will release the connections too.
	fInputs.MakeEmpty(true);
	fOutputs.MakeEmpty(true);

	fNode->Release();
}


void
BMediaClient::_AddInput(BMediaInput* input)
{
	CALLED();

	fInputs.AddItem(new InputReleaser(input));
}


void
BMediaClient::_AddOutput(BMediaOutput* output)
{
	CALLED();

	fOutputs.AddItem(new OutputReleaser(output));
}


BMediaInput*
BMediaClient::_FindInput(const media_destination& dest) const
{
	CALLED();

	for (int32 i = 0; i < CountInputs(); i++) {
		if (dest.id == InputAt(i)->_Destination().id)
			return InputAt(i);
	}
	return NULL;
}


BMediaOutput*
BMediaClient::_FindOutput(const media_source& source) const
{
	CALLED();

	for (int32 i = 0; i < CountOutputs(); i++) {
		if (source.id == OutputAt(i)->_Source().id)
			return OutputAt(i);
	}
	return NULL;
}


status_t
BMediaClient::_ConnectInput(BMediaOutput* output,
	const media_connection& input)
{
	CALLED();

	if (input.destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;

	media_output ourOutput = output->Connection()._BuildMediaOutput();
	media_input theirInput = input._BuildMediaInput();
	media_format format;

	// NOTE: We want to set this data in the callbacks if possible.
	// The correct format should have been set in BMediaConnection::Connected.
	// TODO: Perhaps add some check assert?

	status_t ret = BMediaRoster::CurrentRoster()->Connect(ourOutput.source,
		theirInput.destination, &format, &ourOutput, &theirInput,
		BMediaRoster::B_CONNECT_MUTED);

#if 0
	if (ret == B_OK)
		output->fConnection.format = format;
#endif

	return ret;
}


status_t
BMediaClient::_ConnectOutput(BMediaInput* input,
	const media_connection& output)
{
	CALLED();

	if (output.source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	media_input ourInput = input->Connection()._BuildMediaInput();
	media_output theirOutput = output._BuildMediaOutput();
	media_format format;

	// NOTE: We want to set this data in the callbacks if possible.
	// The correct format should have been set in BMediaConnection::Connected.
	// TODO: Perhaps add some check assert?

	status_t ret = BMediaRoster::CurrentRoster()->Connect(theirOutput.source,
		ourInput.destination, &format, &theirOutput, &ourInput,
		BMediaRoster::B_CONNECT_MUTED);

#if 0
	if (ret == B_OK)
		input->fConnection.format = format;
#endif

	return ret;
}


status_t
BMediaClient::_DisconnectConnection(BMediaConnection* conn)
{
	CALLED();

	if (conn->Client() != this)
		return B_ERROR;

	const media_connection& handle = conn->Connection();
	if (handle.IsInput()) {
		return BMediaRoster::CurrentRoster()->Disconnect(
			handle.remote_node.node, handle.source,
			handle._Node().node, handle.destination);
	} else {
		return BMediaRoster::CurrentRoster()->Disconnect(
			handle._Node().node, handle.source,
			handle.remote_node.node, handle.destination);
	}

	return B_ERROR;
}


status_t
BMediaClient::_ReleaseConnection(BMediaConnection* conn)
{
	if (conn->Client() != this)
		return B_ERROR;

	if (conn->Connection().IsInput()) {
		InputReleaser obj = InputReleaser(dynamic_cast<BMediaInput*>(conn));
		fInputs.RemoveItem(&obj);
		return B_OK;
	} else {
		OutputReleaser obj = OutputReleaser(dynamic_cast<BMediaOutput*>(conn));
		fOutputs.RemoveItem(&obj);
		return B_OK;
	}

	return B_ERROR;
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
