/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaRoster.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaRoster.h>
#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <StopWatch.h>
#include <OS.h>
#include <String.h>
#include <TimeSource.h>
#include <ParameterWeb.h>
#include "debug.h"
#include "MediaRosterEx.h"
#include "MediaMisc.h"
#include "PortPool.h"
#include "ServerInterface.h"
#include "DataExchange.h"
#include "DormantNodeManager.h"
#include "Notifications.h"
#include "TimeSourceObjectManager.h"

namespace BPrivate { namespace media {

	// the BMediaRoster destructor is private,
	// but _DefaultDeleter is a friend class of
	// the BMediaRoster an thus can delete it
	class DefaultDeleter
	{
	public:
		~DefaultDeleter()
		{
			if (BMediaRoster::_sDefault) {
				BMediaRoster::_sDefault->Lock();
				BMediaRoster::_sDefault->Quit();
			}
		}
	};

} } // BPrivate::media
using namespace BPrivate::media;

// DefaultDeleter will delete the BMediaRoster object in it's destructor.
DefaultDeleter _deleter;

status_t
BMediaRosterEx::SaveNodeConfiguration(BMediaNode *node)
{
	BMediaAddOn *addon;
	media_addon_id addonid;
	int32 flavorid;
	addon = node->AddOn(&flavorid);
	if (!addon) {
		// XXX this check incorrectly triggers on BeOS R5 BT848 node
		FATAL("BMediaRosterEx::SaveNodeConfiguration node %ld not instantiated from BMediaAddOn!\n", node->ID());
		return B_ERROR;
	}
	addonid = addon->AddonID();
	
	// XXX fix this
	printf("### BMediaRosterEx::SaveNodeConfiguration should save addon-id %ld, flavor-id %ld config NOW!\n", addonid, flavorid);
	return B_OK;
}

status_t
BMediaRosterEx::LoadNodeConfiguration(media_addon_id addonid, int32 flavorid, BMessage *out_msg)
{
	// XXX fix this
	out_msg->MakeEmpty(); // to be fully R5 compliant
	printf("### BMediaRosterEx::LoadNodeConfiguration should load addon-id %ld, flavor-id %ld config NOW!\n", addonid, flavorid);
	return B_OK;
}

status_t
BMediaRosterEx::IncrementAddonFlavorInstancesCount(media_addon_id addonid, int32 flavorid)
{
	server_change_addon_flavor_instances_count_request request;
	server_change_addon_flavor_instances_count_reply reply;
	
	request.addonid = addonid;
	request.flavorid = flavorid;
	request.delta = 1;
	request.team = team;
	return QueryServer(SERVER_CHANGE_ADDON_FLAVOR_INSTANCES_COUNT, &request, sizeof(request), &reply, sizeof(reply));
}

status_t
BMediaRosterEx::DecrementAddonFlavorInstancesCount(media_addon_id addonid, int32 flavorid)
{
	server_change_addon_flavor_instances_count_request request;
	server_change_addon_flavor_instances_count_reply reply;
	
	request.addonid = addonid;
	request.flavorid = flavorid;
	request.delta = -1;
	request.team = team;
	return QueryServer(SERVER_CHANGE_ADDON_FLAVOR_INSTANCES_COUNT, &request, sizeof(request), &reply, sizeof(reply));
}
	
status_t
BMediaRosterEx::SetNodeCreator(media_node_id node, team_id creator)
{
	server_set_node_creator_request request;
	server_set_node_creator_reply reply;
	
	request.node = node;
	request.creator = creator;
	return QueryServer(SERVER_SET_NODE_CREATOR, &request, sizeof(request), &reply, sizeof(reply));
}

status_t
BMediaRosterEx::GetNode(node_type type, media_node * out_node, int32 * out_input_id, BString * out_input_name)
{
	if (out_node == NULL)
		return B_BAD_VALUE;

	server_get_node_request request;
	server_get_node_reply reply;
	status_t rv;

	request.type = type;
	request.team = team;
	rv = QueryServer(SERVER_GET_NODE, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*out_node = reply.node;
	if (out_input_id)
		*out_input_id = reply.input_id;
	if (out_input_name)
		*out_input_name = reply.input_name;
	return rv;
}

status_t
BMediaRosterEx::SetNode(node_type type, const media_node *node, const dormant_node_info *info, const media_input *input)
{
	server_set_node_request request;
	server_set_node_reply reply;
		
	request.type = type;
	request.use_node = node ? true : false;
	if (node)
		request.node = *node;
	request.use_dni = info ? true : false;
	if (info)
		request.dni = *info;
	request.use_input = input ? true : false;
	if (input)
		request.input = *input;
		
	return QueryServer(SERVER_SET_NODE, &request, sizeof(request), &reply, sizeof(reply));
}

status_t
BMediaRosterEx::GetAllOutputs(const media_node & node, List<media_output> *list)
{
	int32 cookie;
	status_t rv;
	status_t result;
	
	result = B_OK;
	cookie = 0;
	list->MakeEmpty();
	for (;;) {
		producer_get_next_output_request request;
		producer_get_next_output_reply reply;
		request.cookie = cookie;
		rv = QueryPort(node.port, PRODUCER_GET_NEXT_OUTPUT, &request, sizeof(request), &reply, sizeof(reply));
		if (rv != B_OK)
			break;
		cookie = reply.cookie;
		if (!list->Insert(reply.output)) {
			FATAL("GetAllOutputs: list->Insert failed\n");
			result = B_ERROR;
		}
	}

	producer_dispose_output_cookie_request request;
	producer_dispose_output_cookie_reply reply;
	QueryPort(node.port, PRODUCER_DISPOSE_OUTPUT_COOKIE, &request, sizeof(request), &reply, sizeof(reply));
	
	return result;
}

status_t
BMediaRosterEx::GetAllInputs(const media_node & node, List<media_input> *list)
{
	int32 cookie;
	status_t rv;
	status_t result;
	
	result = B_OK;
	cookie = 0;
	list->MakeEmpty();
	for (;;) {
		consumer_get_next_input_request request;
		consumer_get_next_input_reply reply;
		request.cookie = cookie;
		rv = QueryPort(node.port, CONSUMER_GET_NEXT_INPUT, &request, sizeof(request), &reply, sizeof(reply));
		if (rv != B_OK)
			break;
		cookie = reply.cookie;
		if (!list->Insert(reply.input)) {
			FATAL("GetAllInputs: list->Insert failed\n");
			result = B_ERROR;
		}
	}

	consumer_dispose_input_cookie_request request;
	consumer_dispose_input_cookie_reply reply;
	QueryPort(node.port, CONSUMER_DISPOSE_INPUT_COOKIE, &request, sizeof(request), &reply, sizeof(reply));
	
	return result;
}

status_t
BMediaRosterEx::PublishOutputs(const media_node & node, List<media_output> *list)
{
	server_publish_outputs_request request;
	server_publish_outputs_reply reply;
	media_output *output;
	media_output *outputs;
	int32 count;
	status_t rv;
	
	count = list->CountItems();
	TRACE("PublishOutputs: publishing %ld\n", count);
	
	request.node = node;
	request.count = count;
	if (count > MAX_OUTPUTS) {
		void *start_addr;
		size_t size;
		size = ROUND_UP_TO_PAGE(count * sizeof(media_output));
		request.area = create_area("publish outputs", &start_addr, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (request.area < B_OK) {
			FATAL("PublishOutputs: failed to create area, %#lx\n", request.area);
			return (status_t)request.area;
		}
		outputs = static_cast<media_output *>(start_addr);
	} else {
		request.area = -1;
		outputs = request.outputs;
	}
	TRACE("PublishOutputs: area %ld\n", request.area);
	
	int i;
	for (i = 0, list->Rewind(); list->GetNext(&output); i++) {
		ASSERT(i < count);
		outputs[i] = *output;
	}
	
	rv = QueryServer(SERVER_PUBLISH_OUTPUTS, &request, sizeof(request), &reply, sizeof(reply));
	
	if (request.area != -1)
		delete_area(request.area);
	
	return rv;
}

status_t
BMediaRosterEx::PublishInputs(const media_node & node, List<media_input> *list)
{
	server_publish_inputs_request request;
	server_publish_inputs_reply reply;
	media_input *input;
	media_input *inputs;
	int32 count;
	status_t rv;
	
	count = list->CountItems();
	TRACE("PublishInputs: publishing %ld\n", count);
	
	request.node = node;
	request.count = count;
	if (count > MAX_INPUTS) {
		void *start_addr;
		size_t size;
		size = ROUND_UP_TO_PAGE(count * sizeof(media_input));
		request.area = create_area("publish inputs", &start_addr, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (request.area < B_OK) {
			FATAL("PublishInputs: failed to create area, %#lx\n", request.area);
			return (status_t)request.area;
		}
		inputs = static_cast<media_input *>(start_addr);
	} else {
		request.area = -1;
		inputs = request.inputs;
	}
	TRACE("PublishInputs: area %ld\n", request.area);
	
	int i;
	for (i = 0, list->Rewind(); list->GetNext(&input); i++) {
		ASSERT(i < count);
		inputs[i] = *input;
	}
	
	rv = QueryServer(SERVER_PUBLISH_INPUTS, &request, sizeof(request), &reply, sizeof(reply));

	if (request.area != -1)
		delete_area(request.area);
	
	return rv;
}

/*************************************************************
 * public BMediaRoster
 *************************************************************/

status_t 
BMediaRoster::GetVideoInput(media_node * out_node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(VIDEO_INPUT, out_node);
}


status_t 
BMediaRoster::GetAudioInput(media_node * out_node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(AUDIO_INPUT, out_node);
}


status_t 
BMediaRoster::GetVideoOutput(media_node * out_node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(VIDEO_OUTPUT, out_node);
}


status_t 
BMediaRoster::GetAudioMixer(media_node * out_node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(AUDIO_MIXER, out_node);
}


status_t 
BMediaRoster::GetAudioOutput(media_node * out_node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(AUDIO_OUTPUT, out_node);
}


status_t 
BMediaRoster::GetAudioOutput(media_node * out_node,
							 int32 * out_input_id,
							 BString * out_input_name)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(AUDIO_OUTPUT_EX, out_node, out_input_id, out_input_name);
}

							 
status_t 
BMediaRoster::GetTimeSource(media_node * out_node)
{
	CALLED();
	status_t rv;

	// XXX need to do this in a nicer way.

	rv = MediaRosterEx(this)->GetNode(TIME_SOURCE, out_node); 
	if (rv != B_OK)
		return rv;
		
	// We don't do reference counting for timesources, that's why we
	// release the node immediately.
	ReleaseNode(*out_node);

	// we need to remember to not use this node with server side reference counting
	out_node->kind |= NODE_KIND_NO_REFCOUNTING;
	
	return B_OK;
}


status_t 
BMediaRoster::SetVideoInput(const media_node & producer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(VIDEO_INPUT, &producer);
}


status_t 
BMediaRoster::SetVideoInput(const dormant_node_info & producer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(VIDEO_INPUT, NULL, &producer);
}


status_t 
BMediaRoster::SetAudioInput(const media_node & producer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_INPUT, &producer);
}


status_t 
BMediaRoster::SetAudioInput(const dormant_node_info & producer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_INPUT, NULL, &producer);
}


status_t 
BMediaRoster::SetVideoOutput(const media_node & consumer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(VIDEO_OUTPUT, &consumer);
}


status_t 
BMediaRoster::SetVideoOutput(const dormant_node_info & consumer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(VIDEO_OUTPUT, NULL, &consumer);
}


status_t 
BMediaRoster::SetAudioOutput(const media_node & consumer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_OUTPUT, &consumer);
}


status_t 
BMediaRoster::SetAudioOutput(const media_input & input_to_output)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_OUTPUT, NULL, NULL, &input_to_output);
}


status_t 
BMediaRoster::SetAudioOutput(const dormant_node_info & consumer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_OUTPUT, NULL, &consumer);
}


status_t 
BMediaRoster::GetNodeFor(media_node_id node,
						 media_node * clone)
{
	CALLED();
	if (clone == NULL)
		return B_BAD_VALUE;	
	if (node <= 0)
		return B_MEDIA_BAD_NODE;

	server_get_node_for_request request;
	server_get_node_for_reply reply;
	status_t rv;
	
	request.nodeid = node;
	request.team = team;
	
	rv = QueryServer(SERVER_GET_NODE_FOR, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;
	
	*clone = reply.clone;
	return B_OK;
}


status_t 
BMediaRoster::GetSystemTimeSource(media_node * clone)
{
	CALLED();
	status_t rv;

	// XXX need to do this in a nicer way.

	rv = MediaRosterEx(this)->GetNode(SYSTEM_TIME_SOURCE, clone); 
	if (rv != B_OK)
		return rv;
		
	// We don't do reference counting for timesources, that's why we
	// release the node immediately.
	ReleaseNode(*clone);

	// we need to remember to not use this node with server side reference counting
	clone->kind |= NODE_KIND_NO_REFCOUNTING;
	
	return B_OK;
}


status_t 
BMediaRoster::ReleaseNode(const media_node & node)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;
		
	if (node.kind & NODE_KIND_NO_REFCOUNTING) {
		printf("BMediaRoster::ReleaseNode, trying to release reference counting disabled timesource, node %ld, port %ld, team %ld\n", node.node, node.port, team);
		return B_OK;
	}

	server_release_node_request request;
	server_release_node_reply reply;
	status_t rv;
	
	request.node = node;
	request.team = team;
	
	TRACE("BMediaRoster::ReleaseNode, node %ld, port %ld, team %ld\n", node.node, node.port, team);
	
	rv = QueryServer(SERVER_RELEASE_NODE, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		FATAL("BMediaRoster::ReleaseNode FAILED, node %ld, port %ld, team %ld!\n", node.node, node.port, team);
	}
	return rv;
}

BTimeSource *
BMediaRoster::MakeTimeSourceFor(const media_node & for_node)
{
	CALLED();
	
	BTimeSource *source;

	TRACE("BMediaRoster::MakeTimeSourceFor enter, node %ld, port %ld, kind %#lx\n", for_node.node, for_node.port, for_node.kind);
	
	if (0 == (for_node.kind & B_TIME_SOURCE)) {
		//FATAL("BMediaRoster::MakeTimeSourceFor, node %ld is not a timesource!\n", for_node.node);
		// XXX It appears that Cortex calls this function on every node, and expects
		// XXX to be returned a system time source if the for_node is not a timesource
		media_node clone;
		GetSystemTimeSource(&clone);
		source = _TimeSourceObjectManager->GetTimeSource(clone);
//		ReleaseNode(clone);
	} else {
		source = _TimeSourceObjectManager->GetTimeSource(for_node);
	}

	TRACE("BMediaRoster::MakeTimeSourceFor leave, node %ld, port %ld, kind %#lx\n", source->Node().node, source->Node().port, source->Node().kind);

	return source;
}


status_t 
BMediaRoster::Connect(const media_source & from,
					  const media_destination & to,
					  media_format * io_format,
					  media_output * out_output,
					  media_input * out_input)
{
	return BMediaRoster::Connect(from, to, io_format, out_output, out_input, 0);
}


status_t 
BMediaRoster::Connect(const media_source & from,
					  const media_destination & to,
					  media_format * io_format,
					  media_output * out_output,
					  media_input * out_input,
					  uint32 in_flags,
					  void * _reserved)
{
	CALLED();
	if (io_format == NULL || out_output == NULL || out_input == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_SOURCE(from)) {
		FATAL("BMediaRoster::Connect: media_source invalid\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (IS_INVALID_DESTINATION(to)) {
		FATAL("BMediaRoster::Connect: media_destination invalid\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	status_t rv;
	producer_format_proposal_request request1;
	producer_format_proposal_reply reply1;
	
	// BBufferProducer::FormatProposal
	request1.output = from;
	request1.format = *io_format;
	rv = QueryPort(from.port, PRODUCER_FORMAT_PROPOSAL, &request1, sizeof(request1), &reply1, sizeof(reply1));
	if (rv != B_OK) {
		FATAL("BMediaRoster::Connect: aborted after BBufferProducer::FormatProposal, status = %#lx\n",rv);
		return rv;
	}
	// reply1.format now contains the format proposed by the producer

	consumer_accept_format_request request2;
	consumer_accept_format_reply reply2;
	
	// BBufferConsumer::AcceptFormat
	request2.dest = to;
	request2.format = reply1.format;
	rv = QueryPort(to.port, CONSUMER_ACCEPT_FORMAT, &request2, sizeof(request2), &reply2, sizeof(reply2));
	if (rv != B_OK) {
		FATAL("BMediaRoster::Connect: aborted after BBufferConsumer::AcceptFormat, status = %#lx\n",rv);
		return rv;
	}
	// reply2.format now contains the format accepted by the consumer

	// BBufferProducer::PrepareToConnect
	producer_prepare_to_connect_request request3;
	producer_prepare_to_connect_reply reply3;

	request3.source = from;
	request3.destination = to;
	request3.format = reply2.format;
	strcpy(request3.name, "XXX some default name"); // XXX fix this
	rv = QueryPort(from.port, PRODUCER_PREPARE_TO_CONNECT, &request3, sizeof(request3), &reply3, sizeof(reply3));
	if (rv != B_OK) {
		FATAL("BMediaRoster::Connect: aborted after BBufferProducer::PrepareToConnect, status = %#lx\n",rv);
		return rv;
	}
	// reply3.format is still our pretty media format
	// reply3.out_source the real source to be used for the connection
	// reply3.name the name BBufferConsumer::Connected will see in the outInput->name argument

	// BBufferConsumer::Connected
	consumer_connected_request request4;
	consumer_connected_reply reply4;
	status_t con_status;
	
	request4.producer = reply3.out_source;
	request4.where = to;
	request4.with_format = reply3.format;
	con_status = QueryPort(to.port, CONSUMER_CONNECTED, &request4, sizeof(request4), &reply4, sizeof(reply4));
	if (con_status != B_OK) {
		FATAL("BMediaRoster::Connect: aborting after BBufferConsumer::Connected, status = %#lx\n",con_status);
		// we do NOT return here!
	}
	// con_status contains the status code to be supplied to BBufferProducer::Connect's status argument
	// reply4.input contains the media_input that describes the connection from the consumer point of view

	// BBufferProducer::Connect
	producer_connect_request request5;
	producer_connect_reply reply5;
	
	request5.error = con_status;
	request5.source = reply3.out_source;
	request5.destination = reply4.input.destination;
	request5.format = reply3.format; // XXX reply4.input.format ???
	strcpy(request5.name, reply4.input.name);
	rv = QueryPort(reply4.input.source.port, PRODUCER_CONNECT, &request5, sizeof(request5), &reply5, sizeof(reply5));
	if (con_status != B_OK) {
		FATAL("BMediaRoster::Connect: aborted\n");
		return con_status;
	}
	if (rv != B_OK) {
		FATAL("BMediaRoster::Connect: aborted after BBufferProducer::Connect, status = %#lx\n",rv);
		return rv;
	}
	// reply5.name contains the name assigned to the connection by the producer

	// find the output node
	// XXX isn't there a easier way?
	media_node sourcenode;
	GetNodeFor(NodeIDFor(from.port), &sourcenode);
	ReleaseNode(sourcenode);

	// initilize connection info
	*io_format = reply3.format;
	*out_input = reply4.input;
	out_output->node = sourcenode;
	out_output->source = reply4.input.source;
	out_output->destination = reply4.input.destination;
	out_output->format = reply4.input.format;
	strcpy(out_output->name, reply5.name);

	// the connection is now made
	
	
	// XXX register connection with server
	// XXX we should just send a notification, instead of republishing all endpoints
	List<media_output> outlist;
	List<media_input> inlist;
	if (B_OK == MediaRosterEx(this)->GetAllOutputs(out_output->node , &outlist))
		MediaRosterEx(this)->PublishOutputs(out_output->node , &outlist);
	if (B_OK == MediaRosterEx(this)->GetAllInputs(out_input->node , &inlist))
		MediaRosterEx(this)->PublishInputs(out_input->node, &inlist);


	// XXX if (mute) BBufferProducer::EnableOutput(false)
	if (in_flags & B_CONNECT_MUTED) {
	}


	// send a notification
	BPrivate::media::notifications::ConnectionMade(*out_input, *out_output, *io_format);

	return B_OK;
};


status_t 
BMediaRoster::Disconnect(media_node_id source_nodeid,
						 const media_source & source,
						 media_node_id destination_nodeid,
						 const media_destination & destination)
{
	CALLED();
	if (IS_INVALID_NODEID(source_nodeid)) {
		FATAL("BMediaRoster::Disconnect: source media_node_id invalid\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (IS_INVALID_NODEID(destination_nodeid)) {
		FATAL("BMediaRoster::Disconnect: destination media_node_id invalid\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	if (IS_INVALID_SOURCE(source)) {
		FATAL("BMediaRoster::Disconnect: media_source invalid\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (IS_INVALID_DESTINATION(destination)) {
		FATAL("BMediaRoster::Disconnect: media_destination invalid\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	producer_disconnect_request request2;
	producer_disconnect_reply reply2;
	consumer_disconnected_request request1;
	consumer_disconnected_reply reply1;
	status_t rv1, rv2;

	// XXX we should ask the server if this connection really exists

	request1.source = source;
	request1.destination = destination;
	request2.source = source;
	request2.destination = destination;

	rv1 = QueryPort(source.port, PRODUCER_DISCONNECT, &request1, sizeof(request1), &reply1, sizeof(reply1));
	rv2 = QueryPort(destination.port, CONSUMER_DISCONNECTED, &request2, sizeof(request2), &reply2, sizeof(reply2));

	// XXX unregister connection with server
	// XXX we should just send a notification, instead of republishing all endpoints
	List<media_output> outlist;
	List<media_input> inlist;
	media_node sourcenode; 
	media_node destnode;
	if (B_OK == GetNodeFor(source_nodeid, &sourcenode)) {
		if (B_OK == MediaRosterEx(this)->GetAllOutputs(sourcenode , &outlist))
			MediaRosterEx(this)->PublishOutputs(sourcenode , &outlist);
		ReleaseNode(sourcenode);
	} else FATAL("BMediaRoster::Disconnect: source GetNodeFor failed\n");
	if (B_OK == GetNodeFor(destination_nodeid, &destnode)) {
		if (B_OK == MediaRosterEx(this)->GetAllInputs(destnode , &inlist))
			MediaRosterEx(this)->PublishInputs(destnode, &inlist);
		ReleaseNode(destnode);
	} else FATAL("BMediaRoster::Disconnect: dest GetNodeFor failed\n");
	

	// send a notification
	BPrivate::media::notifications::ConnectionBroken(source, destination);

	return (rv1 != B_OK || rv2 != B_OK) ? B_ERROR : B_OK;
}


status_t 
BMediaRoster::StartNode(const media_node & node,
						bigtime_t at_performance_time)
{
	CALLED();
	if (node.node <= 0)
		return B_MEDIA_BAD_NODE;
		
	TRACE("BMediaRoster::StartNode, node %ld, at perf %Ld\n", node.node, at_performance_time);

	node_start_command command;
	command.performance_time = at_performance_time;
	
	return SendToPort(node.port, NODE_START, &command, sizeof(command));
}


status_t 
BMediaRoster::StopNode(const media_node & node,
					   bigtime_t at_performance_time,
					   bool immediate)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	TRACE("BMediaRoster::StopNode, node %ld, at perf %Ld %s\n", node.node, at_performance_time, immediate ? "NOW" : "");

	node_stop_command command;
	command.performance_time = at_performance_time;
	command.immediate = immediate;
	
	return SendToPort(node.port, NODE_STOP, &command, sizeof(command));
}

					   
status_t 
BMediaRoster::SeekNode(const media_node & node,
					   bigtime_t to_media_time,
					   bigtime_t at_performance_time)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	TRACE("BMediaRoster::SeekNode, node %ld, at perf %Ld, to perf %Ld\n", node.node, at_performance_time, to_media_time);

	node_seek_command command;
	command.media_time = to_media_time;
	command.performance_time = at_performance_time;
	
	return SendToPort(node.port, NODE_SEEK, &command, sizeof(command));
}


status_t 
BMediaRoster::StartTimeSource(const media_node & node,
							  bigtime_t at_real_time)
{
	CALLED();
	if (IS_SYSTEM_TIMESOURCE(node)) {
		// XXX debug this
		//FATAL("BMediaRoster::StartTimeSource node %ld is system timesource\n", node.node);
		return B_OK;
	}
	if (IS_SHADOW_TIMESOURCE(node)) {
		// XXX debug this
		FATAL("BMediaRoster::StartTimeSource node %ld is shadow timesource\n", node.node);
		return B_OK;
	}
	if (IS_INVALID_NODE(node)) {
		FATAL("BMediaRoster::StartTimeSource node %ld invalid\n", node.node);
		return B_MEDIA_BAD_NODE;
	}
	if ((node.kind & B_TIME_SOURCE) == 0) {
		FATAL("BMediaRoster::StartTimeSource node %ld is no timesource\n", node.node);
		return B_MEDIA_BAD_NODE;
	}

	TRACE("BMediaRoster::StartTimeSource, node %ld, at real %Ld\n", node.node, at_real_time);
		
	BTimeSource::time_source_op_info msg;
	msg.op = BTimeSource::B_TIMESOURCE_START;
	msg.real_time = at_real_time;

	return write_port(node.port, TIMESOURCE_OP, &msg, sizeof(msg));
}

							  
status_t 
BMediaRoster::StopTimeSource(const media_node & node,
							 bigtime_t at_real_time,
							 bool immediate)
{
	CALLED();
	if (IS_SYSTEM_TIMESOURCE(node)) {
		// XXX debug this
		//FATAL("BMediaRoster::StopTimeSource node %ld is system timesource\n", node.node);
		return B_OK;
	}
	if (IS_SHADOW_TIMESOURCE(node)) {
		// XXX debug this
		FATAL("BMediaRoster::StopTimeSource node %ld is shadow timesource\n", node.node);
		return B_OK;
	}
	if (IS_INVALID_NODE(node)) {
		FATAL("BMediaRoster::StopTimeSource node %ld invalid\n", node.node);
		return B_MEDIA_BAD_NODE;
	}
	if ((node.kind & B_TIME_SOURCE) == 0) {
		FATAL("BMediaRoster::StopTimeSource node %ld is no timesource\n", node.node);
		return B_MEDIA_BAD_NODE;
	}

	TRACE("BMediaRoster::StopTimeSource, node %ld, at real %Ld %s\n", node.node, at_real_time, immediate ? "NOW" : "");
		
	BTimeSource::time_source_op_info msg;
	msg.op = immediate ? BTimeSource::B_TIMESOURCE_STOP_IMMEDIATELY : BTimeSource::B_TIMESOURCE_STOP;
	msg.real_time = at_real_time;

	return write_port(node.port, TIMESOURCE_OP, &msg, sizeof(msg));
}

							 
status_t 
BMediaRoster::SeekTimeSource(const media_node & node,
							 bigtime_t to_performance_time,
							 bigtime_t at_real_time)
{
	CALLED();
	if (IS_SYSTEM_TIMESOURCE(node)) {
		// XXX debug this
		// FATAL("BMediaRoster::SeekTimeSource node %ld is system timesource\n", node.node);
		// you can't seek the system time source, but
		// returning B_ERROR would break StampTV
		return B_OK;
	}
	if (IS_SHADOW_TIMESOURCE(node)) {
		// XXX debug this
		FATAL("BMediaRoster::SeekTimeSource node %ld is shadow timesource\n", node.node);
		return B_OK;
	}
	if (IS_INVALID_NODE(node)) {
		FATAL("BMediaRoster::SeekTimeSource node %ld invalid\n", node.node);
		return B_MEDIA_BAD_NODE;
	}
	if ((node.kind & B_TIME_SOURCE) == 0) {
		FATAL("BMediaRoster::SeekTimeSource node %ld is no timesource\n", node.node);
		return B_MEDIA_BAD_NODE;
	}

	TRACE("BMediaRoster::SeekTimeSource, node %ld, at real %Ld, to perf %Ld\n", node.node, at_real_time, to_performance_time);

	BTimeSource::time_source_op_info msg;
	msg.op = BTimeSource::B_TIMESOURCE_SEEK;
	msg.real_time = at_real_time;
	msg.performance_time = to_performance_time; 

	return write_port(node.port, TIMESOURCE_OP, &msg, sizeof(msg));
}


status_t 
BMediaRoster::SyncToNode(const media_node & node,
						 bigtime_t at_time,
						 bigtime_t timeout)
{
	UNIMPLEMENTED();
	return B_OK;
}

						 
status_t 
BMediaRoster::SetRunModeNode(const media_node & node,
							 BMediaNode::run_mode mode)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	node_set_run_mode_command msg;
	msg.mode = mode;
	
	return write_port(node.port, NODE_SET_RUN_MODE, &msg, sizeof(msg));
}

							 
status_t 
BMediaRoster::PrerollNode(const media_node & node)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	char dummy;
	return write_port(node.port, NODE_PREROLL, &dummy, sizeof(dummy));
}


status_t 
BMediaRoster::RollNode(const media_node & node, 
					   bigtime_t startPerformance,
					   bigtime_t stopPerformance,
					   bigtime_t atMediaTime)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::SetProducerRunModeDelay(const media_node & node,
									  bigtime_t delay,
									  BMediaNode::run_mode mode)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;
	if ((node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
		
	producer_set_run_mode_delay_command command;
	command.mode = mode;
	command.delay = delay;

	return SendToPort(node.port, PRODUCER_SET_RUN_MODE_DELAY, &command, sizeof(command));
}


status_t 
BMediaRoster::SetProducerRate(const media_node & producer,
							  int32 numer,
							  int32 denom)
{
	CALLED();
	if (IS_INVALID_NODE(producer))
		return B_MEDIA_BAD_NODE;
	if ((producer.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;

	producer_set_play_rate_request msg;
	producer_set_play_rate_reply reply;
	status_t rv;
	int32 code;

	msg.numer = numer;
	msg.denom = denom;
	msg.reply_port = _PortPool->GetPort();
	rv = write_port(producer.node, PRODUCER_SET_PLAY_RATE, &msg, sizeof(msg));
	if (rv != B_OK) {
		_PortPool->PutPort(msg.reply_port);
		return rv;
	}
	rv = read_port(msg.reply_port, &code, &reply, sizeof(reply));
	_PortPool->PutPort(msg.reply_port);
	return (rv < B_OK) ? rv : reply.result;
}


/* Nodes will have available inputs/outputs as long as they are capable */
/* of accepting more connections. The node may create an additional */
/* output or input as the currently available is taken into usage. */
status_t 
BMediaRoster::GetLiveNodeInfo(const media_node & node,
							  live_node_info * out_live_info)
{
	CALLED();
	if (out_live_info == NULL)
		return B_BAD_VALUE;	
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	server_get_live_node_info_request request;
	server_get_live_node_info_reply reply;
	status_t rv;
	
	request.node = node;
	
	rv = QueryServer(SERVER_GET_LIVE_NODE_INFO, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;
	
	*out_live_info = reply.live_info;
	return B_OK;
}


status_t 
BMediaRoster::GetLiveNodes(live_node_info * out_live_nodes,
						   int32 * io_total_count,
						   const media_format * has_input,
						   const media_format * has_output,
						   const char * name,
						   uint64 node_kinds)
{
	CALLED();
	if (out_live_nodes == NULL || io_total_count == NULL)
		return B_BAD_VALUE;
	if (*io_total_count <= 0)
		return B_BAD_VALUE;

	// XXX we also support the wildcard search as GetDormantNodes does. This needs to be documented

	server_get_live_nodes_request request;
	server_get_live_nodes_reply reply;
	status_t rv;
	
	request.maxcount = *io_total_count;
	request.has_input = (bool) has_input;
	if (has_input)
		request.inputformat = *has_input; // XXX we should not make a flat copy of media_format
	request.has_output = (bool) has_output;
	if (has_output)
		request.outputformat = *has_output; // XXX we should not make a flat copy of media_format
	request.has_name = (bool) name;
	if (name) {
		int len = strlen(name);
		len = min_c(len, (int)sizeof(request.name) - 1);
		memcpy(request.name, name, len);
		request.name[len] = 0;
	}
	request.require_kinds = node_kinds;

	rv = QueryServer(SERVER_GET_LIVE_NODES, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		FATAL("BMediaRoster::GetLiveNodes failed\n");
		*io_total_count = 0;
		return rv;
	}

	if (reply.count > MAX_LIVE_INFO) {
		live_node_info *live_info;
		area_id clone;

		clone = clone_area("live_node_info clone", reinterpret_cast<void **>(&live_info), B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, reply.area);
		if (clone < B_OK) {
			FATAL("BMediaRoster::GetLiveNodes failed to clone area, %#lx\n", clone);
			delete_area(reply.area);
			*io_total_count = 0;
			return B_ERROR;
		}

		for (int32 i = 0; i < reply.count; i++) {
			out_live_nodes[i] = live_info[i];
		}

		delete_area(clone);
		delete_area(reply.area);
	} else {
		for (int32 i = 0; i < reply.count; i++) {
			out_live_nodes[i] = reply.live_info[i];
		}
	}
	*io_total_count = reply.count;

	return B_OK;
}


status_t 
BMediaRoster::GetFreeInputsFor(const media_node & node,
							   media_input * out_free_inputs,
							   int32 buf_num_inputs,
							   int32 * out_total_count,
							   media_type filter_type)
{
	CALLED();
	if (IS_INVALID_NODE(node) || (node.kind & B_BUFFER_CONSUMER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_free_inputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	List<media_input> list;
	media_input *input;
	status_t rv;

	*out_total_count = 0;

	rv = MediaRosterEx(this)->GetAllInputs(node, &list);
	if (B_OK != rv)
		return rv;

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&input); i++) {
		if (filter_type != B_MEDIA_UNKNOWN_TYPE && filter_type != input->format.type)
			continue; // media_type used, but doesn't match
		if (input->source != media_source::null)
			continue; // consumer source already connected
		out_free_inputs[i] = *input;
		*out_total_count += 1;
		buf_num_inputs -= 1;
		if (buf_num_inputs == 0)
			break;
	}
	
	MediaRosterEx(this)->PublishInputs(node, &list);
	return B_OK;
}


status_t 
BMediaRoster::GetConnectedInputsFor(const media_node & node,
									media_input * out_active_inputs,
									int32 buf_num_inputs,
									int32 * out_total_count)
{
	CALLED();
	if (IS_INVALID_NODE(node) || (node.kind & B_BUFFER_CONSUMER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_active_inputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	List<media_input> list;
	media_input *input;
	status_t rv;

	*out_total_count = 0;

	rv = MediaRosterEx(this)->GetAllInputs(node, &list);
	if (B_OK != rv)
		return rv;

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&input); i++) {
		if (input->source == media_source::null)
			continue; // consumer source not connected
		out_active_inputs[i] = *input;
		*out_total_count += 1;
		buf_num_inputs -= 1;
		if (buf_num_inputs == 0)
			break;
	}
	
	MediaRosterEx(this)->PublishInputs(node, &list);
	return B_OK;
}


status_t 
BMediaRoster::GetAllInputsFor(const media_node & node,
							  media_input * out_inputs,
							  int32 buf_num_inputs,
							  int32 * out_total_count)
{
	CALLED();
	if (IS_INVALID_NODE(node) || (node.kind & B_BUFFER_CONSUMER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_inputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	List<media_input> list;
	media_input *input;
	status_t rv;

	*out_total_count = 0;

	rv = MediaRosterEx(this)->GetAllInputs(node, &list);
	if (B_OK != rv)
		return rv;

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&input); i++) {
		out_inputs[i] = *input;
		*out_total_count += 1;
		buf_num_inputs -= 1;
		if (buf_num_inputs == 0)
			break;
	}
	
	MediaRosterEx(this)->PublishInputs(node, &list);
	return B_OK;
}


status_t 
BMediaRoster::GetFreeOutputsFor(const media_node & node,
								media_output * out_free_outputs,
								int32 buf_num_outputs,
								int32 * out_total_count,
								media_type filter_type)
{
	CALLED();
	if (IS_INVALID_NODE(node) || (node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_free_outputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	List<media_output> list;
	media_output *output;
	status_t rv;

	*out_total_count = 0;

	rv = MediaRosterEx(this)->GetAllOutputs(node, &list);
	if (B_OK != rv)
		return rv;

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&output); i++) {
		if (filter_type != B_MEDIA_UNKNOWN_TYPE && filter_type != output->format.type)
			continue; // media_type used, but doesn't match
		if (output->destination != media_destination::null)
			continue; // producer destination already connected
		out_free_outputs[i] = *output;
		*out_total_count += 1;
		buf_num_outputs -= 1;
		if (buf_num_outputs == 0)
			break;
	}
	
	MediaRosterEx(this)->PublishOutputs(node, &list);
	return B_OK;
}

								
status_t 
BMediaRoster::GetConnectedOutputsFor(const media_node & node,
									 media_output * out_active_outputs,
									 int32 buf_num_outputs,
									 int32 * out_total_count)
{
	CALLED();
	if (IS_INVALID_NODE(node) || (node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_active_outputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	List<media_output> list;
	media_output *output;
	status_t rv;

	*out_total_count = 0;

	rv = MediaRosterEx(this)->GetAllOutputs(node, &list);
	if (B_OK != rv)
		return rv;

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&output); i++) {
		if (output->destination == media_destination::null)
			continue; // producer destination not connected
		out_active_outputs[i] = *output;
		*out_total_count += 1;
		buf_num_outputs -= 1;
		if (buf_num_outputs == 0)
			break;
	}
	
	MediaRosterEx(this)->PublishOutputs(node, &list);
	return B_OK;
}


status_t 
BMediaRoster::GetAllOutputsFor(const media_node & node,
							   media_output * out_outputs,
							   int32 buf_num_outputs,
							   int32 * out_total_count)
{
	CALLED();
	if (IS_INVALID_NODE(node) || (node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_outputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	List<media_output> list;
	media_output *output;
	status_t rv;

	*out_total_count = 0;

	rv = MediaRosterEx(this)->GetAllOutputs(node, &list);
	if (B_OK != rv)
		return rv;

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&output); i++) {
		out_outputs[i] = *output;
		*out_total_count += 1;
		buf_num_outputs -= 1;
		if (buf_num_outputs == 0)
			break;
	}
	
	MediaRosterEx(this)->PublishOutputs(node, &list);
	return B_OK;
}


status_t 
BMediaRoster::StartWatching(const BMessenger & where)
{
	CALLED();
	if (!where.IsValid()) {
		FATAL("BMediaRoster::StartWatching: messenger invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Register(where, media_node::null, B_MEDIA_WILDCARD);
}


status_t 
BMediaRoster::StartWatching(const BMessenger & where,
							int32 notificationType)
{
	CALLED();
	if (!where.IsValid()) {
		FATAL("BMediaRoster::StartWatching: messenger invalid!\n");
		return B_BAD_VALUE;
	}
	if (false == BPrivate::media::notifications::IsValidNotificationRequest(false, notificationType)) {
		FATAL("BMediaRoster::StartWatching: notificationType invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Register(where, media_node::null, notificationType);
}


status_t 
BMediaRoster::StartWatching(const BMessenger & where,
							const media_node & node,
							int32 notificationType)
{
	CALLED();
	if (!where.IsValid()) {
		FATAL("BMediaRoster::StartWatching: messenger invalid!\n");
		return B_BAD_VALUE;
	}
	if (IS_INVALID_NODE(node)) {
		FATAL("BMediaRoster::StartWatching: node invalid!\n");
		return B_MEDIA_BAD_NODE;
	}
	if (false == BPrivate::media::notifications::IsValidNotificationRequest(true, notificationType)) {
		FATAL("BMediaRoster::StartWatching: notificationType invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Register(where, node, notificationType);
}

							
status_t 
BMediaRoster::StopWatching(const BMessenger & where)
{
	CALLED();
	// messenger may already be invalid, so we don't check this
	return BPrivate::media::notifications::Unregister(where, media_node::null, B_MEDIA_WILDCARD);
}


status_t 
BMediaRoster::StopWatching(const BMessenger & where,
						   int32 notificationType)
{
	CALLED();
	// messenger may already be invalid, so we don't check this
	if (false == BPrivate::media::notifications::IsValidNotificationRequest(false, notificationType)) {
		FATAL("BMediaRoster::StopWatching: notificationType invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Unregister(where, media_node::null, notificationType);
}

						   
status_t 
BMediaRoster::StopWatching(const BMessenger & where,
						   const media_node & node,
						   int32 notificationType)
{
	CALLED();
	// messenger may already be invalid, so we don't check this
	if (IS_INVALID_NODE(node)) {
		FATAL("BMediaRoster::StopWatching: node invalid!\n");
		return B_MEDIA_BAD_NODE;
	}
	if (false == BPrivate::media::notifications::IsValidNotificationRequest(true, notificationType)) {
		FATAL("BMediaRoster::StopWatching: notificationType invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Unregister(where, node, notificationType);
}


status_t 
BMediaRoster::RegisterNode(BMediaNode * node)
{
	CALLED();
	// addon-id = -1 (unused), addon-flavor-id = 0 (unused, too)
	return MediaRosterEx(this)->RegisterNode(node, -1, 0);
}


status_t
BMediaRosterEx::RegisterNode(BMediaNode * node, media_addon_id addonid, int32 flavorid)
{
	CALLED();
	if (node == NULL)
		return B_BAD_VALUE;
		
	// some sanity check
	// I'm not sure if the media kit warrants to call BMediaNode::AddOn() here.
	// Perhaps we don't need it.
	{
		BMediaAddOn *addon;
		int32 addon_flavor_id;
		media_addon_id addon_id;
		addon_flavor_id = 0;
		addon = node->AddOn(&addon_flavor_id);
		addon_id = addon ? addon->AddonID() : -1;
		ASSERT(addonid == addon_id);
		ASSERT(flavorid == addon_flavor_id);
	}
	
	status_t rv;
	server_register_node_request request;
	server_register_node_reply reply;
	
	request.addon_id = addonid;
	request.addon_flavor_id = flavorid;
	strcpy(request.name, node->Name());
	request.kinds = node->Kinds();
	request.port = node->ControlPort();
	request.team = team;

	TRACE("BMediaRoster::RegisterNode: sending SERVER_REGISTER_NODE: port %ld, kinds 0x%Lx, team %ld, name '%s'\n", request.port, request.kinds, request.team, request.name);
	
	rv = QueryServer(SERVER_REGISTER_NODE, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		FATAL("BMediaRoster::RegisterNode: failed to register node %s (error %#lx)\n", node->Name(), rv);
		return rv;
	}
	
	// we are a friend class of BMediaNode and initialize this member variable
	node->fNodeID = reply.nodeid;
	ASSERT(reply.nodeid == node->Node().node);
	ASSERT(reply.nodeid == node->ID());

	// call the callback
	node->NodeRegistered();
	
	// if the BMediaNode also inherits from BTimeSource, we need to call BTimeSource::FinishCreate()
	if (node->Kinds() & B_TIME_SOURCE) {
		BTimeSource *ts;
		ts = dynamic_cast<BTimeSource *>(node);
		if (ts)
			ts->FinishCreate();
	}

/*
	// register existing inputs and outputs with the
	// media_server, this allows GetLiveNodes() to work
	// with created, but unconnected nodes.
	if (node->Kinds() & B_BUFFER_PRODUCER) {
		Stack<media_output> stack;
		if (B_OK == GetAllOutputs(node->Node(), &stack))
			PublishOutputs(node->Node(), &stack);
	} else if (node->Kinds() & B_BUFFER_CONSUMER) {
		Stack<media_input> stack;
		if (B_OK == GetAllInputs(node->Node(), &stack))
			PublishInputs(node->Node(), &stack);
	}
*/

	BPrivate::media::notifications::NodesCreated(&reply.nodeid, 1);
/*
	TRACE("BMediaRoster::RegisterNode: registered node name '%s', id %ld, addon %ld, flavor %ld\n", node->Name(), node->ID(), addon_id, addon_flavor_id);
	TRACE("BMediaRoster::RegisterNode: node this               %p\n", node);
	TRACE("BMediaRoster::RegisterNode: node fConsumerThis      %p\n", node->fConsumerThis);
	TRACE("BMediaRoster::RegisterNode: node fProducerThis      %p\n", node->fProducerThis);
	TRACE("BMediaRoster::RegisterNode: node fFileInterfaceThis %p\n", node->fFileInterfaceThis);
	TRACE("BMediaRoster::RegisterNode: node fControllableThis  %p\n", node->fControllableThis);
	TRACE("BMediaRoster::RegisterNode: node fTimeSourceThis    %p\n", node->fTimeSourceThis);
*/	

	return B_OK;
}


status_t 
BMediaRoster::UnregisterNode(BMediaNode * node)
{
	CALLED();
	if (node == NULL)
		return B_BAD_VALUE;

	TRACE("BMediaRoster::UnregisterNode %ld (%p)\n", node->ID(), node);

	if (node->fKinds & NODE_KIND_NO_REFCOUNTING) {
		printf("BMediaRoster::UnregisterNode, trying to unregister reference counting disabled timesource, node %ld, port %ld, team %ld\n", node->ID(), node->ControlPort(), team);
		return B_OK;
	}
	if (node->ID() == NODE_UNREGISTERED_ID) {
		FATAL("BMediaRoster::UnregisterNode: Warning node id %ld, name '%s' already unregistered\n", node->ID(), node->Name());
		return B_OK;
	}
	if (node->fRefCount != 0) {
		FATAL("BMediaRoster::UnregisterNode: Warning node id %ld, name '%s' has local reference count of %ld\n", node->ID(), node->Name(), node->fRefCount);
		// no return here, we continue and unregister!
	}
	
	// Calling BMediaAddOn::GetConfigurationFor(BMediaNode *node, BMessage *config)
	// if this node was instanciated by an add-on needs to be done *somewhere*
	// We can't do it here because it is already to late (destructor of the node
	// might have been called).
			
	server_unregister_node_request request;
	server_unregister_node_reply reply;
	status_t rv;

	request.nodeid = node->ID();
	request.team = team;

	// send a notification
	BPrivate::media::notifications::NodesDeleted(&request.nodeid, 1);

	rv = QueryServer(SERVER_UNREGISTER_NODE, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		FATAL("BMediaRoster::UnregisterNode: failed to unregister node id %ld, name '%s' (error %#lx)\n", node->ID(), node->Name(), rv);
		return rv;
	}
	
	if (reply.addonid != -1) {
		// Small problem here, we can't use DormantNodeManager::PutAddon(), as
		// UnregisterNode() is called by a dormant node itself (by the destructor).
		// The add-on that contains the node needs to remain in memory until the
		// destructor execution is finished.
		// DormantNodeManager::PutAddonDelayed() will delay unloading.
		_DormantNodeManager->PutAddonDelayed(reply.addonid);

		rv = MediaRosterEx(this)->DecrementAddonFlavorInstancesCount(reply.addonid, reply.flavorid);
		if (rv != B_OK) {
			FATAL("BMediaRoster::UnregisterNode: DecrementAddonFlavorInstancesCount failed\n");
			// this is really a problem, but we can't fail now
		}
	}

	// we are a friend class of BMediaNode and invalidate this member variable
	node->fNodeID = NODE_UNREGISTERED_ID;
	
	return B_OK;
}


//	thread safe for multiple calls to Roster()
/* static */ BMediaRoster * 
BMediaRoster::Roster(status_t* out_error)
{
	static BLocker locker("BMediaRoster::Roster locker");
	locker.Lock();
	if (_sDefault == NULL) {
		_sDefault = new BMediaRosterEx();
		if (out_error != NULL)
			*out_error = B_OK;
	} else {
		if (out_error != NULL)
			*out_error = B_OK;
	}
	locker.Unlock();
	return _sDefault;
}


//	won't create it if there isn't one	
//	not thread safe if you call Roster() at the same time
/* static */ BMediaRoster * 
BMediaRoster::CurrentRoster()			
{
	return _sDefault;
}

												
status_t 
BMediaRoster::SetTimeSourceFor(media_node_id node,
							   media_node_id time_source)
{
	CALLED();
	if (IS_INVALID_NODEID(node) || IS_INVALID_NODEID(time_source))
		return B_BAD_VALUE;
	
	media_node clone;
	status_t rv, result;
	
	// we need to get a clone of the node to have a port id
	rv = GetNodeFor(node, &clone);
	if (rv != B_OK) {
		FATAL("BMediaRoster::SetTimeSourceFor, GetNodeFor failed, node id %ld\n", node);
		return B_ERROR;
	}

	TRACE("BMediaRoster::SetTimeSourceFor: node %ld will be assigned time source %ld\n", node, time_source);
	
	// we just send the request to set time_source-id as timesource to the node,
	// the NODE_SET_TIMESOURCE handler code will do the real assignment
	result = B_OK;
	node_set_timesource_command cmd;
	cmd.timesource_id = time_source;
	rv = SendToPort(clone.port, NODE_SET_TIMESOURCE, &cmd, sizeof(cmd));	
	if (rv != B_OK) {
		FATAL("BMediaRoster::SetTimeSourceFor, sending NODE_SET_TIMESOURCE failed, node id %ld\n", node);
		result = B_ERROR;
	}

	// we release the clone
	rv = ReleaseNode(clone);
	if (rv != B_OK) {
		FATAL("BMediaRoster::SetTimeSourceFor, ReleaseNode failed, node id %ld\n", node);
		result = B_ERROR;
	}

	return result;
}


status_t 
BMediaRoster::GetParameterWebFor(const media_node & node, 
								 BParameterWeb ** out_web)
{
	CALLED();
	if (out_web == NULL)
		return B_BAD_VALUE;	
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;
	if ((node.kind & B_CONTROLLABLE) == 0)
		return B_MEDIA_BAD_NODE;
		
	controllable_get_parameter_web_request request;
	controllable_get_parameter_web_reply reply;
	int32 requestsize[] = {B_PAGE_SIZE, 4*B_PAGE_SIZE, 16*B_PAGE_SIZE, 64*B_PAGE_SIZE, 128*B_PAGE_SIZE, 256*B_PAGE_SIZE, 0};
	int32 size;
	
	// XXX it might be better to query the node for the (current) parameter size first
	for (int i = 0; (size = requestsize[i]) != 0; i++) {
		status_t rv;
		area_id area;
		void *data;
		area = create_area("parameter web data", &data, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < B_OK) {
			FATAL("BMediaRoster::GetParameterWebFor couldn't create area of size %ld\n", size);
			return B_ERROR;
		}
		request.maxsize = size;
		request.area = area;
		rv = QueryPort(node.port, CONTROLLABLE_GET_PARAMETER_WEB, &request, sizeof(request), &reply, sizeof(reply));
		if (rv != B_OK) {
			FATAL("BMediaRoster::GetParameterWebFor CONTROLLABLE_GET_PARAMETER_WEB failed\n");
			delete_area(area);
			return B_ERROR;
		}
		if (reply.size == 0) {
			// no parameter web available
			// XXX should we return an error?
			FATAL("BMediaRoster::GetParameterWebFor node %ld has no parameter web\n", node.node);
			*out_web = new BParameterWeb();
			delete_area(area);
			return B_OK;
		}
		if (reply.size > 0) {
			// we got a flattened parameter web!
			*out_web = new BParameterWeb();
			
			printf("BMediaRoster::GetParameterWebFor Unflattening %ld bytes, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
				reply.size, ((uint32*)data)[0], ((uint32*)data)[1], ((uint32*)data)[2], ((uint32*)data)[3]);
			
			rv = (*out_web)->Unflatten(reply.code, data, reply.size);
			if (rv != B_OK) {
				FATAL("BMediaRoster::GetParameterWebFor Unflatten failed, %s\n", strerror(rv));
				delete_area(area);
				delete *out_web;
				return B_ERROR;
			}
			delete_area(area);
			return B_OK;
		}
		delete_area(area);
		ASSERT(reply.size == -1);
		// parameter web data was too large
		// loop and try a larger size
	}
	FATAL("BMediaRoster::GetParameterWebFor node %ld has no parameter web larger than %ld\n", node.node, size);
	return B_ERROR;
}

								 
status_t 
BMediaRoster::StartControlPanel(const media_node & node,
								BMessenger * out_messenger)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetDormantNodes(dormant_node_info * out_info,
							  int32 * io_count,
							  const media_format * has_input /* = NULL */,
							  const media_format * has_output /* = NULL */,
							  const char * name /* = NULL */,
							  uint64 require_kinds /* = NULL */,
							  uint64 deny_kinds /* = NULL */)
{
	CALLED();
	if (out_info == NULL)
		return B_BAD_VALUE;
	if (io_count == NULL)
		return B_BAD_VALUE;
	if (*io_count <= 0)
		return B_BAD_VALUE;

	xfer_server_get_dormant_nodes msg;
	port_id port;
	status_t rv;

	port = find_port("media_server port");
	if (port <= B_OK)
		return B_ERROR;
	
	msg.maxcount = *io_count;
	msg.has_input = (bool) has_input;
	if (has_input)
		msg.inputformat = *has_input; // XXX we should not make a flat copy of media_format
	msg.has_output = (bool) has_output;
	if (has_output)
		msg.outputformat = *has_output;; // XXX we should not make a flat copy of media_format
	msg.has_name = (bool) name;
	if (name) {
		int len = strlen(name);
		len = min_c(len, (int)sizeof(msg.name) - 1);
		memcpy(msg.name, name, len);
		msg.name[len] = 0;
	}
	msg.require_kinds = require_kinds;
	msg.deny_kinds = deny_kinds;
	msg.reply_port = _PortPool->GetPort();

	rv = write_port(port, SERVER_GET_DORMANT_NODES, &msg, sizeof(msg));
	if (rv != B_OK) {
		_PortPool->PutPort(msg.reply_port);
		return rv;
	}

	xfer_server_get_dormant_nodes_reply reply;
	int32 code;

	rv = read_port(msg.reply_port, &code, &reply, sizeof(reply));
	if (rv < B_OK) {
		_PortPool->PutPort(msg.reply_port);
		return rv;
	}

	*io_count = reply.count;
	
	if (*io_count > 0) {
		rv = read_port(msg.reply_port, &code, out_info, *io_count * sizeof(dormant_node_info));
		if (rv < B_OK)
			reply.result = rv;
	}
	_PortPool->PutPort(msg.reply_port);
	
	return reply.result;
}

/* This function is used to do the real work of instantiating a dormant node. It is either
 * called by the media_addon_server to instantiate a global node, or it gets called from
 * BMediaRoster::InstantiateDormantNode() to create a local one.
 *
 * Checks concerning global/local are not done here.
 */
status_t
BMediaRosterEx::InstantiateDormantNode(media_addon_id addonid, int32 flavorid, team_id creator, media_node *out_node)
{
	// This function is always called from the correct context, if the node
	// is supposed to be global, it is called from the media_addon_server.

	// if B_FLAVOR_IS_GLOBAL, we need to use the BMediaAddOn object that
	// resides in the media_addon_server

	// RegisterNode() must be called for nodes instantiated from add-ons,
	// since the media kit warrants that it's done automatically.

	// addonid		Indicates the ID number of the media add-on in which the node resides.
	// flavorid		Indicates the internal ID number that the add-on uses to identify the flavor,
	//				this is the number that was published by BMediaAddOn::GetFlavorAt() in the
	//				flavor_info::internal_id field.
	// creator		The creator team is -1 if nodes are created locally. If created globally,
	//				it will contain (while called in media_addon_server context) the team-id of
	// 				the team that requested the instantiation.
	
	TRACE("BMediaRosterEx::InstantiateDormantNode: addon-id %ld, flavor_id %ld\n", addonid, flavorid);

	// Get flavor_info from the server
	dormant_flavor_info node_info;
	status_t rv;
	rv = GetDormantFlavorInfo(addonid, flavorid, &node_info);	
	if (rv != B_OK) {
		FATAL("BMediaRosterEx::InstantiateDormantNode error: failed to get dormant_flavor_info for addon-id %ld, flavor-id %ld\n", addonid, flavorid);
		return B_ERROR;
	}
	
	ASSERT(node_info.internal_id == flavorid);

	// load the BMediaAddOn object
	BMediaAddOn *addon;
	addon = _DormantNodeManager->GetAddon(addonid);
	if (!addon) {
		FATAL("BMediaRosterEx::InstantiateDormantNode: GetAddon failed\n");
		return B_ERROR;
	}
	
	// Now we need to try to increment the use count of this addon flavor
	// in the server. This can fail if the total number instances of this
	// flavor is limited.
	rv = IncrementAddonFlavorInstancesCount(addonid, flavorid);
	if (rv != B_OK) {
		FATAL("BMediaRosterEx::InstantiateDormantNode error: can't create more nodes for addon-id %ld, flavor-id %ld\n", addonid, flavorid);
		// Put the addon back into the pool
		_DormantNodeManager->PutAddon(addonid);
		return B_ERROR;
	}

	BMessage config;
	rv = LoadNodeConfiguration(addonid, flavorid, &config);
	if (rv != B_OK) {
		FATAL("BMediaRosterEx::InstantiateDormantNode: couldn't load configuration for addon-id %ld, flavor-id %ld\n", addonid, flavorid);
		// do not return, this is a minor problem, not a reason to fail
	}

	BMediaNode *node;
	status_t out_error;
	
	out_error = B_OK;
	node = addon->InstantiateNodeFor(&node_info, &config, &out_error);
	if (!node) {
		FATAL("BMediaRosterEx::InstantiateDormantNode: InstantiateNodeFor failed\n");
		// Put the addon back into the pool
		_DormantNodeManager->PutAddon(addonid);
		// We must decrement the use count of this addon flavor in the
		// server to compensate the increment done in the beginning.
		rv = DecrementAddonFlavorInstancesCount(addonid, flavorid);
		if (rv != B_OK) {
			FATAL("BMediaRosterEx::InstantiateDormantNode: DecrementAddonFlavorInstancesCount failed\n");
		}
		return (out_error != B_OK) ? out_error : B_ERROR;
	}

	rv = RegisterNode(node, addonid, flavorid);
	if (rv != B_OK) {
		FATAL("BMediaRosterEx::InstantiateDormantNode: RegisterNode failed\n");
		delete node;
		// Put the addon back into the pool
		_DormantNodeManager->PutAddon(addonid);
		// We must decrement the use count of this addon flavor in the
		// server to compensate the increment done in the beginning.
		rv = DecrementAddonFlavorInstancesCount(addonid, flavorid);
		if (rv != B_OK) {
			FATAL("BMediaRosterEx::InstantiateDormantNode: DecrementAddonFlavorInstancesCount failed\n");
		}
		return B_ERROR;
	}
	
	if (creator != -1) {
		// send a message to the server to assign team "creator" as creator of node "node->ID()"
		printf("!!! BMediaRosterEx::InstantiateDormantNode assigning team %ld as creator of node %ld\n", creator, node->ID());
		rv = MediaRosterEx(this)->SetNodeCreator(node->ID(), creator);
		if (rv != B_OK) {
			FATAL("BMediaRosterEx::InstantiateDormantNode failed to assign team %ld as creator of node %ld\n", creator, node->ID());
			// do not return, this is a minor problem, not a reason to fail
		}
	}
	
	// RegisterNode() does remember the add-on id in the server
	// and UnregisterNode() will call DormantNodeManager::PutAddon()
	// when the node is unregistered.

	*out_node = node->Node();

	TRACE("BMediaRosterEx::InstantiateDormantNode: addon-id %ld, flavor_id %ld instanciated as node %ld, port %ld in team %ld\n", addonid, flavorid, out_node->node, out_node->port, team);

	return B_OK;
}


status_t 
BMediaRoster::InstantiateDormantNode(const dormant_node_info & in_info,
									 media_node * out_node,
									 uint32 flags /* currently 0 or B_FLAVOR_IS_GLOBAL or B_FLAVOR_IS_LOCAL */ )
{
	CALLED();
	if (out_node == 0)
		return B_BAD_VALUE;
	if (in_info.addon <= 0) {
		FATAL("BMediaRoster::InstantiateDormantNode error: addon-id %ld invalid.\n", in_info.addon);
		return B_BAD_VALUE;
	}

	printf("BMediaRoster::InstantiateDormantNode: addon-id %ld, flavor_id %ld, flags 0x%X\n", in_info.addon, in_info.flavor_id, flags);

	// Get flavor_info from the server
	// XXX this is a little overhead, as we get the full blown dormant_flavor_info,
	// XXX but only need the flags.
	dormant_flavor_info node_info;
	status_t rv;
	rv = MediaRosterEx(this)->GetDormantFlavorInfo(in_info.addon, in_info.flavor_id, &node_info);	
	if (rv != B_OK) {
		FATAL("BMediaRoster::InstantiateDormantNode error: failed to get dormant_flavor_info for addon-id %ld, flavor-id %ld\n", in_info.addon, in_info.flavor_id);
		return B_NAME_NOT_FOUND;
	}

	ASSERT(node_info.internal_id == in_info.flavor_id);

	printf("BMediaRoster::InstantiateDormantNode: name \"%s\", info \"%s\", flavor_flags 0x%X, internal_id %ld, possible_count %ld\n",
		node_info.name, node_info.info, node_info.flavor_flags, node_info.internal_id, node_info.possible_count);

	#if DEBUG
		if (flags & B_FLAVOR_IS_LOCAL)
			printf("BMediaRoster::InstantiateDormantNode: caller requested B_FLAVOR_IS_LOCAL\n");
		if (flags & B_FLAVOR_IS_GLOBAL)
			printf("BMediaRoster::InstantiateDormantNode: caller requested B_FLAVOR_IS_GLOBAL\n");
		if (node_info.flavor_flags & B_FLAVOR_IS_LOCAL)
			printf("BMediaRoster::InstantiateDormantNode: node requires B_FLAVOR_IS_LOCAL\n");
		if (node_info.flavor_flags & B_FLAVOR_IS_GLOBAL)
			printf("BMediaRoster::InstantiateDormantNode: node requires B_FLAVOR_IS_GLOBAL\n");
	#endif

	// Make sure that flags demanded by the dormant node and those requested
	// by the caller are not incompatible.
	if ((node_info.flavor_flags & B_FLAVOR_IS_GLOBAL) && (flags & B_FLAVOR_IS_LOCAL)) {
		FATAL("BMediaRoster::InstantiateDormantNode Error: requested B_FLAVOR_IS_LOCAL, but dormant node has B_FLAVOR_IS_GLOBAL\n");
		return B_NAME_NOT_FOUND;
	}
	if ((node_info.flavor_flags & B_FLAVOR_IS_LOCAL) && (flags & B_FLAVOR_IS_GLOBAL)) {
		FATAL("BMediaRoster::InstantiateDormantNode Error: requested B_FLAVOR_IS_GLOBAL, but dormant node has B_FLAVOR_IS_LOCAL\n");
		return B_NAME_NOT_FOUND;
	}

	// If either the node, or the caller requested to make the instance global
	// we will do it by forwarding this request into the media_addon_server, which
	// in turn will call BMediaRosterEx::InstantiateDormantNode to create the node
	// there and make it globally available.
	if ((node_info.flavor_flags & B_FLAVOR_IS_GLOBAL) || (flags & B_FLAVOR_IS_GLOBAL)) {

		printf("BMediaRoster::InstantiateDormantNode: creating global object in media_addon_server\n");

		addonserver_instantiate_dormant_node_request request;
		addonserver_instantiate_dormant_node_reply reply;
		request.addonid = in_info.addon;
		request.flavorid = in_info.flavor_id;
		request.creator_team = team; // creator team is allowed to also release global nodes
		rv = QueryAddonServer(ADDONSERVER_INSTANTIATE_DORMANT_NODE, &request, sizeof(request), &reply, sizeof(reply));
		if (rv == B_OK) {
			*out_node = reply.node;
		}

	} else {

		// creator team = -1, as this is a local node
		rv = MediaRosterEx(this)->InstantiateDormantNode(in_info.addon, in_info.flavor_id, -1, out_node);

	}
	if (rv != B_OK) {
		*out_node = media_node::null;
		return B_NAME_NOT_FOUND;
	}
	return B_OK;
}

									 
status_t 
BMediaRoster::InstantiateDormantNode(const dormant_node_info & in_info,
									 media_node * out_node)
{
	return InstantiateDormantNode(in_info, out_node, 0);
}


status_t 
BMediaRoster::GetDormantNodeFor(const media_node & node,
								dormant_node_info * out_info)
{
	CALLED();
	if (out_info == NULL)
		return B_BAD_VALUE;	
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	server_get_dormant_node_for_request request;
	server_get_dormant_node_for_reply reply;
	status_t rv;
	
	request.node = node;
	
	rv = QueryServer(SERVER_GET_DORMANT_NODE_FOR, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;
	
	*out_info = reply.node_info;
	return B_OK;
}

status_t 
BMediaRosterEx::GetDormantFlavorInfo(media_addon_id addonid,
									  int32 flavorid,
									  dormant_flavor_info * out_flavor)
{
	CALLED();
	
	xfer_server_get_dormant_flavor_info msg;
	xfer_server_get_dormant_flavor_info_reply *reply;
	port_id port;
	status_t rv;
	int32 code;

	port = find_port("media_server port");
	if (port <= B_OK)
		return B_ERROR;

	reply = (xfer_server_get_dormant_flavor_info_reply *) malloc(16000);
	if (reply == 0)
		return B_ERROR;
	
	msg.addon 		= addonid;
	msg.flavor_id 	= flavorid;
	msg.reply_port 	= _PortPool->GetPort();
	rv = write_port(port, SERVER_GET_DORMANT_FLAVOR_INFO, &msg, sizeof(msg));
	if (rv != B_OK) {
		free(reply);
		_PortPool->PutPort(msg.reply_port);
		return rv;
	}
	rv = read_port(msg.reply_port, &code, reply, 16000);
	_PortPool->PutPort(msg.reply_port);

	if (rv < B_OK) {
		free(reply);
		return rv;
	}
	
	if (reply->result == B_OK)
		rv = out_flavor->Unflatten(reply->dfi_type, &reply->dfi, reply->dfi_size);
	else
		rv = reply->result;
	
	free(reply);
	return rv;
}

status_t 
BMediaRoster::GetDormantFlavorInfoFor(const dormant_node_info & in_dormant,
									  dormant_flavor_info * out_flavor)
{
	return MediaRosterEx(this)->GetDormantFlavorInfo(in_dormant.addon, in_dormant.flavor_id, out_flavor);
}

// Reports in outLatency the maximum latency found downstream from
// the specified BBufferProducer, producer, given the current connections.
status_t 
BMediaRoster::GetLatencyFor(const media_node & producer,
							bigtime_t * out_latency)
{
	CALLED();
	if (out_latency == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_NODE(producer))
		return B_MEDIA_BAD_NODE;
	if ((producer.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	
	producer_get_latency_request request;
	producer_get_latency_reply reply;
	status_t rv;

	rv = QueryPort(producer.port, PRODUCER_GET_LATENCY, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;
	
	*out_latency = reply.latency;
	
	printf("BMediaRoster::GetLatencyFor producer %ld has maximum latency %Ld\n", producer.node, *out_latency);
	return B_OK;
}


status_t 
BMediaRoster::GetInitialLatencyFor(const media_node & producer,
								   bigtime_t * out_latency,
								   uint32 * out_flags /* = NULL */)
{
	UNIMPLEMENTED();
	*out_latency = 5000;
	if (out_flags)
		*out_flags = 0;
	return B_OK;
}


status_t 
BMediaRoster::GetStartLatencyFor(const media_node & time_source,
								 bigtime_t * out_latency)
{
	UNIMPLEMENTED();
	*out_latency = 5000;
	return B_OK;
}


status_t 
BMediaRoster::GetFileFormatsFor(const media_node & file_interface, 
								media_file_format * out_formats,
								int32 * io_num_infos)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::SetRefFor(const media_node & file_interface,
						const entry_ref & file,
						bool create_and_truncate,
						bigtime_t * out_length)	/* if create is false */
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetRefFor(const media_node & node,
						entry_ref * out_file,
						BMimeType * mime_type)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::SniffRefFor(const media_node & file_interface,
						  const entry_ref & file,
						  BMimeType * mime_type,
						  float * out_capability)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


/* This is the generic "here's a file, now can someone please play it" interface */
status_t 
BMediaRoster::SniffRef(const entry_ref & file,
					   uint64 require_node_kinds,		/* if you need an EntityInterface or BufferConsumer or something */
					   dormant_node_info * out_node,
					   BMimeType * mime_type)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetDormantNodeForType(const BMimeType & type,
									uint64 require_node_kinds,
									dormant_node_info * out_node)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

									
status_t 
BMediaRoster::GetReadFileFormatsFor(const dormant_node_info & in_node,
									media_file_format * out_read_formats,
									int32 in_read_count,
									int32 * out_read_count)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

									
status_t 
BMediaRoster::GetWriteFileFormatsFor(const dormant_node_info & in_node,
									 media_file_format * out_write_formats,
									 int32 in_write_count,
									 int32 * out_write_count)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetFormatFor(const media_output & output,
						   media_format * io_format,
						   uint32 flags)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

						   
status_t 
BMediaRoster::GetFormatFor(const media_input & input,
						   media_format * io_format,
						   uint32 flags)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetFormatFor(const media_node & node,
						   media_format * io_format,
						   float quality)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

						   
ssize_t 
BMediaRoster::GetNodeAttributesFor(const media_node & node,
								   media_node_attribute * outArray,
								   size_t inMaxCount)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

								   
media_node_id 
BMediaRoster::NodeIDFor(port_id source_or_destination_port)
{
	CALLED();
	
	server_node_id_for_request request;
	server_node_id_for_reply reply;
	status_t rv;

	request.port = source_or_destination_port;
	
	rv = QueryServer(SERVER_NODE_ID_FOR, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		FATAL("BMediaRoster::NodeIDFor: failed (error %#lx)\n", rv);
		return -1;
	}
	
	return reply.nodeid;
}


status_t 
BMediaRoster::GetInstancesFor(media_addon_id addon,
							  int32 flavor,
							  media_node_id * out_id,
							  int32 * io_count)
{
	CALLED();
	if (out_id == NULL || io_count == NULL)
		return B_BAD_VALUE;
	if (*io_count <= 0)
		return B_BAD_VALUE;

	server_get_instances_for_request request;
	server_get_instances_for_reply reply;
	status_t rv;

	request.maxcount = *io_count;
	request.addon_id = addon;
	request.addon_flavor_id = flavor;

	rv = QueryServer(SERVER_GET_INSTANCES_FOR, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		FATAL("BMediaRoster::GetLiveNodes failed\n");
		return rv;
	}

	*io_count = reply.count;
	if (reply.count > 0)
		memcpy(out_id, reply.node_id, sizeof(media_node_id) * reply.count);

	return B_OK;
}


status_t 
BMediaRoster::SetRealtimeFlags(uint32 in_enabled)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetRealtimeFlags(uint32 * out_enabled)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


ssize_t 
BMediaRoster::AudioBufferSizeFor(int32 channel_count,
								 uint32 sample_format,
								 float frame_rate,
								 bus_type bus_kind)
{
	UNIMPLEMENTED();
	return 4096;
}

								 
/* Use MediaFlags to inquire about specific features of the Media Kit. */
/* Returns < 0 for "not present", positive size for output data size. */
/* 0 means that the capability is present, but no data about it. */
/* static */ ssize_t 
BMediaRoster::MediaFlags(media_flags cap,
						 void * buf,
						 size_t maxSize)
{
	UNIMPLEMENTED();
	return 0;
}


/* BLooper overrides */
/* virtual */ void 
BMediaRoster::MessageReceived(BMessage * message)
{
	switch (message->what) {
		case 'PING':
		{
			// media_server plays ping-pong with the BMediaRosters
			// to detect dead teams. Normal communication uses ports.
			static BMessage pong('PONG');
			message->SendReply(&pong, static_cast<BHandler *>(NULL), 2000000);
			return;
		}

		case NODE_FINAL_RELEASE:
		{
			// this function is called by a BMediaNode to delete
			// itself, as this needs to be done from another thread
			// context, it is done here.
			// XXX If a node is released using BMediaRoster::ReleaseNode()
			// XXX instead of using BMediaNode::Release() / BMediaNode::Acquire()
			// XXX fRefCount of the BMediaNode will not be correct.

			BMediaNode *node;
			message->FindPointer("node", reinterpret_cast<void **>(&node));

			TRACE("BMediaRoster::MessageReceived NODE_FINAL_RELEASE saving node %ld configuration\n", node->ID());
			MediaRosterEx(BMediaRoster::Roster())->SaveNodeConfiguration(node);

			TRACE("BMediaRoster::MessageReceived NODE_FINAL_RELEASE releasing node %ld\n", node->ID());
			node->DeleteHook(node); // we don't call Release(), see above!
			return;
		}
	}
	printf("BMediaRoster::MessageReceived: unknown message!\n");
	message->PrintToStream();
}

/* virtual */ bool 
BMediaRoster::QuitRequested()
{
	UNIMPLEMENTED();
	return true;
}

/* virtual */ BHandler * 
BMediaRoster::ResolveSpecifier(BMessage *msg,
				 int32 index,
				 BMessage *specifier,
				 int32 form,
				 const char *property)
{
	UNIMPLEMENTED();
	return 0;
}				 


/* virtual */ status_t 
BMediaRoster::GetSupportedSuites(BMessage *data)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


BMediaRoster::~BMediaRoster()
{
	CALLED();
	
	// unregister this application with the media server
	server_unregister_app_request request;
	server_unregister_app_reply reply;
	request.team = team;
	QueryServer(SERVER_UNREGISTER_APP, &request, sizeof(request), &reply, sizeof(reply));
}


/*************************************************************
 * private BMediaRoster
 *************************************************************/

// deprecated call
status_t 
BMediaRoster::SetOutputBuffersFor(const media_source & output,
								  BBufferGroup * group,
								  bool will_reclaim )
{
	UNIMPLEMENTED();
	debugger("BMediaRoster::SetOutputBuffersFor missing\n");
	return B_ERROR;
}
	

/* FBC stuffing (Mmmh, Stuffing!) */
status_t BMediaRoster::_Reserved_MediaRoster_0(void *) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_1(void *) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_2(void *) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_3(void *) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_4(void *) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_5(void *) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_6(void *) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_7(void *) { return B_ERROR; }


BMediaRoster::BMediaRoster() : 
	BLooper("_BMediaRoster_", B_URGENT_DISPLAY_PRIORITY, B_LOOPER_PORT_DEFAULT_CAPACITY)
{
	CALLED();
	
	// start the looper
	Run();
	
	// register this application with the media server
	server_register_app_request request;
	server_register_app_reply reply;
	request.team = team;
	request.messenger = BMessenger(NULL, this);
	QueryServer(SERVER_REGISTER_APP, &request, sizeof(request), &reply, sizeof(reply));
}


/* static */ status_t
BMediaRoster::ParseCommand(BMessage & reply)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetDefaultInfo(media_node_id for_default,
							 BMessage & out_config)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

		
							 
status_t 
BMediaRoster::SetRunningDefault(media_node_id for_default,
								const media_node & node)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


/*************************************************************
 * static BMediaRoster variables
 *************************************************************/

bool BMediaRoster::_isMediaServer;
port_id BMediaRoster::_mReplyPort;
int32 BMediaRoster::_mReplyPortRes;
int32 BMediaRoster::_mReplyPortUnavailCount;
BMediaRoster * BMediaRoster::_sDefault = NULL;

