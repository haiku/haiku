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
#undef 	DEBUG
#define	DEBUG 3
#include "debug.h"
#include "TStack.h"
#include "PortPool.h"
#include "ServerInterface.h"
#include "DataExchange.h"
#include "DormantNodeManager.h"
#include "Notifications.h"

namespace BPrivate { namespace media {
	extern team_id team;
} } // BPrivate::media

using namespace BPrivate::media;

// the BMediaRoster destructor is private,
// but _DefaultDeleter is a friend class of
// the BMediaRoster an thus can delete it
class _DefaultDeleter
{
public:
	~_DefaultDeleter() { delete BMediaRoster::_sDefault; }
};

_DefaultDeleter _deleter;

namespace BPrivate { namespace media { namespace mediaroster {

status_t GetNode(node_type type, media_node * out_node, int32 * out_input_id = NULL, BString * out_input_name = NULL);
status_t SetNode(node_type type, const media_node *node, const dormant_node_info *info = NULL, const media_input *input = NULL);
status_t GetAllOutputs(const media_node & node, Stack<media_output> *stack);
status_t GetAllInputs(const media_node & node, Stack<media_input> *stack);
status_t PublishOutputs(const media_node & node, Stack<media_output> *stack);
status_t PublishInputs(const media_node & node, Stack<media_input> *stack);

status_t
GetNode(node_type type, media_node * out_node, int32 * out_input_id, BString * out_input_name)
{
	if (out_node == NULL)
		return B_BAD_VALUE;

	server_get_node_request request;
	server_get_node_reply reply;
	status_t rv;

	request.type = type;
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
SetNode(node_type type, const media_node *node, const dormant_node_info *info, const media_input *input)
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
GetAllOutputs(const media_node & node, Stack<media_output> *stack)
{
	int32 cookie;
	status_t rv;
	status_t result;
	
	result = B_OK;
	cookie = 0;
	for (;;) {
		producer_get_next_output_request request;
		producer_get_next_output_reply reply;
		request.cookie = cookie;
		rv = QueryServer(PRODUCER_GET_NEXT_OUTPUT, &request, sizeof(request), &reply, sizeof(reply));
		if (rv != B_OK)
			break;
		cookie = reply.cookie;
		if (!stack->Push(reply.output)) {
			TRACE("GetAllOutputs: stack->Push failed\n");
			result = B_ERROR;
		}
	}

	producer_dispose_output_cookie_request request;
	producer_dispose_output_cookie_reply reply;
	QueryServer(PRODUCER_DISPOSE_OUTPUT_COOKIE, &request, sizeof(request), &reply, sizeof(reply));
	
	return result;
}

status_t
GetAllInputs(const media_node & node, Stack<media_input> *stack)
{
	int32 cookie;
	status_t rv;
	status_t result;
	
	result = B_OK;
	cookie = 0;
	for (;;) {
		consumer_get_next_input_request request;
		consumer_get_next_input_reply reply;
		request.cookie = cookie;
		rv = QueryServer(CONSUMER_GET_NEXT_INPUT, &request, sizeof(request), &reply, sizeof(reply));
		if (rv != B_OK)
			break;
		cookie = reply.cookie;
		if (!stack->Push(reply.input)) {
			TRACE("GetAllInputs: stack->Push failed\n");
			result = B_ERROR;
		}
	}

	consumer_dispose_input_cookie_request request;
	consumer_dispose_input_cookie_reply reply;
	QueryServer(CONSUMER_DISPOSE_INPUT_COOKIE, &request, sizeof(request), &reply, sizeof(reply));
	
	return result;
}

status_t
PublishOutputs(const media_node & node, Stack<media_output> *stack)
{
	server_publish_outputs_request request;
	server_publish_outputs_reply reply;
	media_output *output;
	media_output *outputs;
	int32 count;
	
	count = stack->CountItems();
	TRACE("PublishOutputs: publishing %ld\n", count);
	
	request.node = node;
	request.count = count;
	if (count > MAX_OUTPUTS) {
		void *start_addr;
		size_t size;
		size = ((count * sizeof(media_output)) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
		request.area = create_area("publish outputs", &start_addr, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (request.area < B_OK) {
			TRACE("PublishOutputs: failed to create area, %#lx\n", request.area);
			return (status_t)request.area;
		}
		outputs = static_cast<media_output *>(start_addr);
	} else {
		request.area = -1;
		outputs = request.outputs;
	}
	TRACE("PublishOutputs: area %#lx\n", request.area);
	
	for (int32 i = 0; i != count; i++) {
		stack->GetPointerAt(i, &output);
		outputs[i] = *output;
	}
	
	return QueryServer(SERVER_PUBLISH_OUTPUTS, &request, sizeof(request), &reply, sizeof(reply));
}

status_t
PublishInputs(const media_node & node, Stack<media_input> *stack)
{
	server_publish_inputs_request request;
	server_publish_inputs_reply reply;
	media_input *input;
	media_input *inputs;
	int32 count;
	
	count = stack->CountItems();
	TRACE("PublishInputs: publishing %ld\n", count);
	
	request.node = node;
	request.count = count;
	if (count > MAX_INPUTS) {
		void *start_addr;
		size_t size;
		size = ((count * sizeof(media_input)) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
		request.area = create_area("publish inputs", &start_addr, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (request.area < B_OK) {
			TRACE("PublishInputs: failed to create area, %#lx\n", request.area);
			return (status_t)request.area;
		}
		inputs = static_cast<media_input *>(start_addr);
	} else {
		request.area = -1;
		inputs = request.inputs;
	}
	TRACE("PublishInputs: area %#lx\n", request.area);
	
	for (int32 i = 0; i != count; i++) {
		stack->GetPointerAt(i, &input);
		inputs[i] = *input;
	}
	
	return QueryServer(SERVER_PUBLISH_INPUTS, &request, sizeof(request), &reply, sizeof(reply));
}

} } } // namespace BPrivate::media::mediaroster

using namespace BPrivate::media::mediaroster;

/*************************************************************
 * public BMediaRoster
 *************************************************************/

status_t 
BMediaRoster::GetVideoInput(media_node * out_node)
{
	CALLED();
	return GetNode(VIDEO_INPUT, out_node);
}


status_t 
BMediaRoster::GetAudioInput(media_node * out_node)
{
	CALLED();
	return GetNode(AUDIO_INPUT, out_node);
}


status_t 
BMediaRoster::GetVideoOutput(media_node * out_node)
{
	CALLED();
	return GetNode(VIDEO_OUTPUT, out_node);
}


status_t 
BMediaRoster::GetAudioMixer(media_node * out_node)
{
	CALLED();
	return GetNode(AUDIO_MIXER, out_node);
}


status_t 
BMediaRoster::GetAudioOutput(media_node * out_node)
{
	CALLED();
	return GetNode(AUDIO_OUTPUT, out_node);
}


status_t 
BMediaRoster::GetAudioOutput(media_node * out_node,
							 int32 * out_input_id,
							 BString * out_input_name)
{
	CALLED();
	return GetNode(AUDIO_OUTPUT_EX, out_node, out_input_id, out_input_name);
}

							 
status_t 
BMediaRoster::GetTimeSource(media_node * out_node)
{
	CALLED();
	return GetNode(TIME_SOURCE, out_node);
}


status_t 
BMediaRoster::SetVideoInput(const media_node & producer)
{
	CALLED();
	return SetNode(VIDEO_INPUT, &producer);
}


status_t 
BMediaRoster::SetVideoInput(const dormant_node_info & producer)
{
	CALLED();
	return SetNode(VIDEO_INPUT, NULL, &producer);
}


status_t 
BMediaRoster::SetAudioInput(const media_node & producer)
{
	CALLED();
	return SetNode(AUDIO_INPUT, &producer);
}


status_t 
BMediaRoster::SetAudioInput(const dormant_node_info & producer)
{
	CALLED();
	return SetNode(AUDIO_INPUT, NULL, &producer);
}


status_t 
BMediaRoster::SetVideoOutput(const media_node & consumer)
{
	CALLED();
	return SetNode(VIDEO_OUTPUT, &consumer);
}


status_t 
BMediaRoster::SetVideoOutput(const dormant_node_info & consumer)
{
	CALLED();
	return SetNode(VIDEO_OUTPUT, NULL, &consumer);
}


status_t 
BMediaRoster::SetAudioOutput(const media_node & consumer)
{
	CALLED();
	return SetNode(AUDIO_OUTPUT, &consumer);
}


status_t 
BMediaRoster::SetAudioOutput(const media_input & input_to_output)
{
	CALLED();
	return SetNode(AUDIO_OUTPUT, NULL, NULL, &input_to_output);
}


status_t 
BMediaRoster::SetAudioOutput(const dormant_node_info & consumer)
{
	CALLED();
	return SetNode(AUDIO_OUTPUT, NULL, &consumer);
}


status_t 
BMediaRoster::GetNodeFor(media_node_id node,
						 media_node * clone)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetSystemTimeSource(media_node * clone)
{
	CALLED();
	return GetNode(SYSTEM_TIME_SOURCE, clone);
}


status_t 
BMediaRoster::ReleaseNode(const media_node & node)
{
	UNIMPLEMENTED();
	return B_ERROR;
}



BTimeSource *
BMediaRoster::MakeTimeSourceFor(const media_node & for_node)
{
	UNIMPLEMENTED();
	return 0;
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
	if (from == media_source::null)
		return B_MEDIA_BAD_SOURCE;
	if (to == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;

	status_t rv;
	producer_format_proposal_request request1;
	producer_format_proposal_reply reply1;
	
	// BBufferProducer::FormatProposal
	request1.output = from;
	request1.format = *io_format;
	rv = QueryPort(from.port, PRODUCER_FORMAT_PROPOSAL, &request1, sizeof(request1), &reply1, sizeof(reply1));
	if (rv != B_OK) {
		TRACE("BMediaRoster::Connect: aborted after BBufferProducer::FormatProposal, status = %#lx\n",rv);
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
		TRACE("BMediaRoster::Connect: aborted after BBufferConsumer::AcceptFormat, status = %#lx\n",rv);
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
		TRACE("BMediaRoster::Connect: aborted after BBufferProducer::PrepareToConnect, status = %#lx\n",rv);
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
		TRACE("BMediaRoster::Connect: aborting after BBufferConsumer::Connected, status = %#lx\n",con_status);
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
	rv = QueryPort(reply4.input.node.port, PRODUCER_CONNECT, &request5, sizeof(request5), &reply5, sizeof(reply5));
	if (con_status != B_OK) {
		TRACE("BMediaRoster::Connect: aborted\n");
		return con_status;
	}
	if (rv != B_OK) {
		TRACE("BMediaRoster::Connect: aborted after BBufferProducer::Connect, status = %#lx\n",rv);
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


	// XXX if (mute) BBufferProducer::EnableOutput(false)


	return B_OK;
};


status_t 
BMediaRoster::Disconnect(media_node_id source_node,
						 const media_source & source,
						 media_node_id destination_node,
						 const media_destination & destination)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::StartNode(const media_node & node,
						bigtime_t at_performance_time)
{
	CALLED();
	if (node.node == 0)
		return B_MEDIA_BAD_NODE;

	xfer_node_start msg;
	msg.performance_time = at_performance_time;
	
	return write_port(node.port, NODE_START, &msg, sizeof(msg));
}


status_t 
BMediaRoster::StopNode(const media_node & node,
					   bigtime_t at_performance_time,
					   bool immediate)
{
	CALLED();
	if (node.node == 0)
		return B_MEDIA_BAD_NODE;

	xfer_node_stop msg;
	msg.performance_time = at_performance_time;
	msg.immediate = immediate;
	
	return write_port(node.port, NODE_STOP, &msg, sizeof(msg));
}

					   
status_t 
BMediaRoster::SeekNode(const media_node & node,
					   bigtime_t to_media_time,
					   bigtime_t at_performance_time)
{
	CALLED();
	if (node.node == 0)
		return B_MEDIA_BAD_NODE;

	xfer_node_seek msg;
	msg.media_time = to_media_time;
	msg.performance_time = at_performance_time;
	
	return write_port(node.port, NODE_SEEK, &msg, sizeof(msg));
}


status_t 
BMediaRoster::StartTimeSource(const media_node & node,
							  bigtime_t at_real_time)
{
	CALLED();
	if (node.node == 0)
		return B_MEDIA_BAD_NODE;
	if ((node.kind & B_TIME_SOURCE) == 0)
		return B_MEDIA_BAD_NODE;
		
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
	if (node.node == 0)
		return B_MEDIA_BAD_NODE;
	if ((node.kind & B_TIME_SOURCE) == 0)
		return B_MEDIA_BAD_NODE;
		
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
	if (node.node == 0)
		return B_MEDIA_BAD_NODE;
	if ((node.kind & B_TIME_SOURCE) == 0)
		return B_MEDIA_BAD_NODE;
		
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
	return B_ERROR;
}

						 
status_t 
BMediaRoster::SetRunModeNode(const media_node & node,
							 BMediaNode::run_mode mode)
{
	CALLED();
	if (node.node == 0)
		return B_MEDIA_BAD_NODE;

	xfer_node_set_run_mode msg;
	msg.mode = mode;
	
	return write_port(node.port, NODE_SET_RUN_MODE, &msg, sizeof(msg));
}

							 
status_t 
BMediaRoster::PrerollNode(const media_node & node)
{
	CALLED();
	if (node.node == 0)
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
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::SetProducerRate(const media_node & producer,
							  int32 numer,
							  int32 denom)
{
	CALLED();
	if (producer.node == 0)
		return B_MEDIA_BAD_NODE;
	if ((producer.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;

	xfer_producer_set_play_rate msg;
	xfer_producer_set_play_rate_reply reply;
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
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetLiveNodes(live_node_info * out_live_nodes,
						   int32 * io_total_count,
						   const media_format * has_input,
						   const media_format * has_output,
						   const char * name,
						   uint64 node_kinds)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetFreeInputsFor(const media_node & node,
							   media_input * out_free_inputs,
							   int32 buf_num_inputs,
							   int32 * out_total_count,
							   media_type filter_type)
{
	CALLED();
	if (node.node == 0 || (node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_free_inputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	Stack<media_input> stack;
	media_input *input;
	status_t rv;

	rv = GetAllInputs(node, &stack);
	if (B_OK != rv)
		return rv;

	*out_total_count = 0;
	for (int32 i = 0; stack.GetPointerAt(i, &input); i++) {
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
	
	PublishInputs(node, &stack);
	return B_OK;
}


status_t 
BMediaRoster::GetConnectedInputsFor(const media_node & node,
									media_input * out_active_inputs,
									int32 buf_num_inputs,
									int32 * out_total_count)
{
	CALLED();
	if (node.node == 0 || (node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_active_inputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	Stack<media_input> stack;
	media_input *input;
	status_t rv;

	rv = GetAllInputs(node, &stack);
	if (B_OK != rv)
		return rv;

	*out_total_count = 0;
	for (int32 i = 0; stack.GetPointerAt(i, &input); i++) {
		if (input->source == media_source::null)
			continue; // consumer source not connected
		out_active_inputs[i] = *input;
		*out_total_count += 1;
		buf_num_inputs -= 1;
		if (buf_num_inputs == 0)
			break;
	}
	
	PublishInputs(node, &stack);
	return B_OK;
}


status_t 
BMediaRoster::GetAllInputsFor(const media_node & node,
							  media_input * out_inputs,
							  int32 buf_num_inputs,
							  int32 * out_total_count)
{
	CALLED();
	if (node.node == 0 || (node.kind & B_BUFFER_CONSUMER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_inputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	Stack<media_input> stack;
	media_input *input;
	status_t rv;

	rv = GetAllInputs(node, &stack);
	if (B_OK != rv)
		return rv;

	*out_total_count = 0;
	for (int32 i = 0; stack.GetPointerAt(i, &input); i++) {
		out_inputs[i] = *input;
		*out_total_count += 1;
		buf_num_inputs -= 1;
		if (buf_num_inputs == 0)
			break;
	}
	
	PublishInputs(node, &stack);
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
	if (node.node == 0 || (node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_free_outputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	Stack<media_output> stack;
	media_output *output;
	status_t rv;

	rv = GetAllOutputs(node, &stack);
	if (B_OK != rv)
		return rv;

	*out_total_count = 0;
	for (int32 i = 0; stack.GetPointerAt(i, &output); i++) {
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
	
	PublishOutputs(node, &stack);
	return B_OK;
}

								
status_t 
BMediaRoster::GetConnectedOutputsFor(const media_node & node,
									 media_output * out_active_outputs,
									 int32 buf_num_outputs,
									 int32 * out_total_count)
{
	CALLED();
	if (node.node == 0 || (node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_active_outputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	Stack<media_output> stack;
	media_output *output;
	status_t rv;

	rv = GetAllOutputs(node, &stack);
	if (B_OK != rv)
		return rv;

	*out_total_count = 0;
	for (int32 i = 0; stack.GetPointerAt(i, &output); i++) {
		if (output->destination == media_destination::null)
			continue; // producer destination not connected
		out_active_outputs[i] = *output;
		*out_total_count += 1;
		buf_num_outputs -= 1;
		if (buf_num_outputs == 0)
			break;
	}
	
	PublishOutputs(node, &stack);
	return B_OK;
}


status_t 
BMediaRoster::GetAllOutputsFor(const media_node & node,
							   media_output * out_outputs,
							   int32 buf_num_outputs,
							   int32 * out_total_count)
{
	CALLED();
	if (node.node == 0 || (node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	if (out_outputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;
		
	Stack<media_output> stack;
	media_output *output;
	status_t rv;

	rv = GetAllOutputs(node, &stack);
	if (B_OK != rv)
		return rv;

	*out_total_count = 0;
	for (int32 i = 0; stack.GetPointerAt(i, &output); i++) {
		out_outputs[i] = *output;
		*out_total_count += 1;
		buf_num_outputs -= 1;
		if (buf_num_outputs == 0)
			break;
	}
	
	PublishOutputs(node, &stack);
	return B_OK;
}


status_t 
BMediaRoster::StartWatching(const BMessenger & where)
{
	CALLED();
	if (!where.IsValid()) {
		TRACE("BMediaRoster::StartWatching: messenger invalid!\n");
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
		TRACE("BMediaRoster::StartWatching: messenger invalid!\n");
		return B_BAD_VALUE;
	}
	if (false == BPrivate::media::notifications::IsValidNotificationRequest(false, notificationType)) {
		TRACE("BMediaRoster::StartWatching: notificationType invalid!\n");
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
		TRACE("BMediaRoster::StartWatching: messenger invalid!\n");
		return B_BAD_VALUE;
	}
	if (node.node == 0) {
		TRACE("BMediaRoster::StartWatching: node invalid!\n");
		return B_MEDIA_BAD_NODE;
	}
	if (false == BPrivate::media::notifications::IsValidNotificationRequest(true, notificationType)) {
		TRACE("BMediaRoster::StartWatching: notificationType invalid!\n");
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
		TRACE("BMediaRoster::StopWatching: notificationType invalid!\n");
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
	if (node.node == 0) {
		TRACE("BMediaRoster::StopWatching: node invalid!\n");
		return B_MEDIA_BAD_NODE;
	}
	if (false == BPrivate::media::notifications::IsValidNotificationRequest(true, notificationType)) {
		TRACE("BMediaRoster::StopWatching: notificationType invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Unregister(where, node, notificationType);
}


status_t 
BMediaRoster::RegisterNode(BMediaNode * node)
{
	CALLED();
	if (node == NULL)
		return B_BAD_VALUE;
		
	status_t rv;

	// XXX fix node registration
	xfer_node_registered msg;
	msg.node_id = 1;
	
	rv = node->HandleMessage(NODE_REGISTERED,&msg,sizeof(msg));
	
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

	return rv;
}


status_t 
BMediaRoster::UnregisterNode(BMediaNode * node)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


//	thread safe for multiple calls to Roster()
/* static */ BMediaRoster * 
BMediaRoster::Roster(status_t* out_error)
{
	CALLED();
	static BLocker locker("BMediaRoster::Roster locker");
	locker.Lock();
	if (_sDefault == NULL) {
		_sDefault = new BMediaRoster();
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
	CALLED();
	return _sDefault;
}

												
status_t 
BMediaRoster::SetTimeSourceFor(media_node_id node,
							   media_node_id time_source)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetParameterWebFor(const media_node & node, 
								 BParameterWeb ** out_web)
{
	UNIMPLEMENTED();
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
		int len = min_c(strlen(name),sizeof(msg.name) - 1);
		memcpy(msg.name,name,len);
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


status_t 
BMediaRoster::InstantiateDormantNode(const dormant_node_info & in_info,
									 media_node * out_node,
									 uint32 flags /* currently B_FLAVOR_IS_GLOBAL or B_FLAVOR_IS_LOCAL */ )
{
	CALLED();
	if ((flags & (B_FLAVOR_IS_GLOBAL | B_FLAVOR_IS_LOCAL)) == 0) {
		printf("Error: BMediaRoster::InstantiateDormantNode called without flags\n");
		return B_BAD_VALUE;
	}
	if (out_node == 0)
		return B_BAD_VALUE;
		
	// XXX we should not trust the values passed in by the user, 
	// XXX and ask the server to determine where to insta
	
	
// XXX SOMETHING IS VERY WRONG HERE
//	if ((in_info.flavor_flags & B_FLAVOR_IS_GLOBAL) == 0 && (flags & B_FLAVOR_IS_LOCAL)) {
	if (flags & B_FLAVOR_IS_LOCAL) {
		return InstantiateDormantNode(in_info,out_node);
	}

// XXX SOMETHING IS VERY WRONG HERE
//	if ((in_info.flavor_flags & B_FLAVOR_IS_GLOBAL) || (flags & B_FLAVOR_IS_GLOBAL)) {
	if (flags & B_FLAVOR_IS_GLOBAL) {
		// forward this request into the media_addon_server,
		// which in turn will call InstantiateDormantNode()
		// to create it there localy
		addonserver_instantiate_dormant_node_request request;
		addonserver_instantiate_dormant_node_reply reply;
		status_t rv;
		
		request.info = in_info;
		rv = QueryAddonServer(ADDONSERVER_INSTANTIATE_DORMANT_NODE, &request, sizeof(request), &reply, sizeof(reply));
		if (rv == B_OK) {
			*out_node = reply.node;
		}
		return rv;
	}

// XXX SOMETHING IS VERY WRONG HERE
	printf("Error: BMediaRoster::InstantiateDormantNode addon_id %d, flavor_id %d, flags %#08lx\n", (int)in_info.addon, (int)in_info.flavor_id, flags);

	return B_ERROR;
}

									 
status_t 
BMediaRoster::InstantiateDormantNode(const dormant_node_info & in_info,
									 media_node * out_node)
{
	CALLED();

	// to instantiate a dormant node in the current address space, we need to
	// either load the add-on from file and create a new BMediaAddOn class, or
	// reuse the cached BMediaAddOn from a previous call
	// call BMediaAddOn::InstantiateNodeFor()
	// and cache the BMediaAddOn after that for later reuse.
	// BeOS R5 does not seem to delete it when the application quits
	// if B_FLAVOR_IS_GLOBAL, we need to use the BMediaAddOn object that
	// resides in the media_addon_server

	// RegisterNode() is called automatically for nodes instantiated from add-ons

	//XXX TEST!
	BMediaAddOn *addon;
	BMediaNode *node;
	BMessage config;
	status_t out_error;
	status_t rv;
	addon = _DormantNodeManager->GetAddon(in_info.addon);
	if (!addon) {
		printf("BMediaRoster::InstantiateDormantNode: GetAddon failed\n");
		return B_ERROR;
	}
	flavor_info temp;
	temp.internal_id = in_info.flavor_id;
	node = addon->InstantiateNodeFor(&temp, &config, &out_error);
	if (!node) {
		printf("BMediaRoster::InstantiateDormantNode: InstantiateNodeFor failed\n");
		_DormantNodeManager->PutAddon(in_info.addon);
		return B_ERROR;
	}
	rv = RegisterNode(node);
	if (rv != B_OK) {
		printf("BMediaRoster::InstantiateDormantNode: RegisterNode failed\n");
		delete node;
		_DormantNodeManager->PutAddon(in_info.addon);
		return B_ERROR;
	}
	
	// XXX we must remember in_info.addon and call
	// XXX _DormantNodeManager->PutAddon when the
	// XXX node is unregistered
	
	*out_node = node->Node();
	return B_OK;
}


status_t 
BMediaRoster::GetDormantNodeFor(const media_node & node,
								dormant_node_info * out_info)
{
	UNIMPLEMENTED();
	
	return B_ERROR;
}


status_t 
BMediaRoster::GetDormantFlavorInfoFor(const dormant_node_info & in_dormant,
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
	
	msg.addon 		= in_dormant.addon;
	msg.flavor_id 	= in_dormant.flavor_id;
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
BMediaRoster::GetLatencyFor(const media_node & producer,
							bigtime_t * out_latency)
{
	UNIMPLEMENTED();
	*out_latency = 0;
	return B_ERROR;
}


status_t 
BMediaRoster::GetInitialLatencyFor(const media_node & producer,
								   bigtime_t * out_latency,
								   uint32 * out_flags)
{
	UNIMPLEMENTED();
	*out_latency = 0;
	*out_flags = 0;
	return B_ERROR;
}


status_t 
BMediaRoster::GetStartLatencyFor(const media_node & time_source,
								 bigtime_t * out_latency)
{
	UNIMPLEMENTED();
	*out_latency = 0;
	return B_ERROR;
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
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaRoster::GetInstancesFor(media_addon_id addon,
							  int32 flavor,
							  media_node_id * out_id,
							  int32 * io_count)
{
	UNIMPLEMENTED();
	return B_ERROR;
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
	UNIMPLEMENTED();
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
	BMessage msg(MEDIA_SERVER_UNREGISTER_APP);
	BMessage reply;
	msg.AddInt32("team",team);
	QueryServer(&msg, &reply);
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
	BLooper("BMediaRoster looper",B_NORMAL_PRIORITY,B_LOOPER_PORT_DEFAULT_CAPACITY)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_REGISTER_APP);
	BMessage reply;
	msg.AddInt32("team",team);
	QueryServer(&msg,&reply);
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

