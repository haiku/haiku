/*
 * Copyright 2008 Maurice Kalinowski, haiku@kaldience.com
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

/*
 * Copyright (c) 2002-2006 Marcus Overhagen <Marcus@Overhagen.de>
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
 */


/* to comply with the license above, do not remove the following line */
char __dont_remove_copyright_from_binary[] = "Copyright (c) 2002-2006 Marcus "
	"Overhagen <Marcus@Overhagen.de>";


#include <MediaRoster.h>

#include <new>

#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <MimeType.h>
#include <OS.h>
#include <ParameterWeb.h>
#include <StopWatch.h>
#include <String.h>
#include <TimeSource.h>

#include <AppMisc.h>

#include <debug.h>
#include <DataExchange.h>
#include <DormantNodeManager.h>
#include <MediaRosterEx.h>
#include <MediaMisc.h>
#include <Notifications.h>
#include <ServerInterface.h>
#include <SharedBufferList.h>

#include "TimeSourceObjectManager.h"


namespace BPrivate {
namespace media {


class MediaInitializer {
public:
	MediaInitializer()
	{
		InitDataExchange();
	}

	~MediaInitializer()
	{
		if (BMediaRoster::CurrentRoster() != NULL) {
			BMediaRoster::CurrentRoster()->Lock();
			BMediaRoster::CurrentRoster()->Quit();
		}
	}
};


}	// namespace media
}	// namespace BPrivate

using namespace BPrivate::media;


static MediaInitializer sInitializer;


BMediaRosterEx::BMediaRosterEx(status_t* _error)
	:
	BMediaRoster()
{
	InitDataExchange();

	gDormantNodeManager = new DormantNodeManager;
	gTimeSourceObjectManager = new TimeSourceObjectManager;

	// register this application with the media server
	server_register_app_request request;
	server_register_app_reply reply;
	request.team = BPrivate::current_team();
	request.messenger = BMessenger(NULL, this);

	status_t status = QueryServer(SERVER_REGISTER_APP, &request,
		sizeof(request), &reply, sizeof(reply));
	if (status != B_OK)
		*_error = B_MEDIA_SYSTEM_FAILURE;
	else
		*_error = B_OK;
}


status_t
BMediaRosterEx::SaveNodeConfiguration(BMediaNode* node)
{
	int32 flavorID;
	BMediaAddOn* addon = node->AddOn(&flavorID);
	if (addon == NULL) {
		// NOTE: This node could have been created by an application,
		// it does not mean there is an error.
		// TODO: this check incorrectly triggers on BeOS R5 BT848 node
		TRACE("BMediaRosterEx::SaveNodeConfiguration node %ld not instantiated "
			"from BMediaAddOn!\n", node->ID());
		return B_ERROR;
	}

	media_addon_id addonID = addon->AddonID();

	// TODO: fix this
	printf("### BMediaRosterEx::SaveNodeConfiguration should save addon-id "
		"%ld, flavor-id %ld config NOW!\n", addonID, flavorID);
	return B_OK;
}


status_t
BMediaRosterEx::LoadNodeConfiguration(media_addon_id addonID, int32 flavorID,
	BMessage *_msg)
{
	// TODO: fix this
	_msg->MakeEmpty(); // to be fully R5 compliant
	printf("### BMediaRosterEx::LoadNodeConfiguration should load addon-id "
		"%ld, flavor-id %ld config NOW!\n", addonID, flavorID);
	return B_OK;
}


status_t
BMediaRosterEx::IncrementAddonFlavorInstancesCount(media_addon_id addonID,
	int32 flavorID)
{
	server_change_flavor_instances_count_request request;
	server_change_flavor_instances_count_reply reply;

	request.add_on_id = addonID;
	request.flavor_id = flavorID;
	request.delta = 1;
	request.team = BPrivate::current_team();
	return QueryServer(SERVER_CHANGE_FLAVOR_INSTANCES_COUNT, &request,
		sizeof(request), &reply, sizeof(reply));
}


status_t
BMediaRosterEx::DecrementAddonFlavorInstancesCount(media_addon_id addonID,
	int32 flavorID)
{
	server_change_flavor_instances_count_request request;
	server_change_flavor_instances_count_reply reply;

	request.add_on_id = addonID;
	request.flavor_id = flavorID;
	request.delta = -1;
	request.team = BPrivate::current_team();
	return QueryServer(SERVER_CHANGE_FLAVOR_INSTANCES_COUNT, &request,
		sizeof(request), &reply, sizeof(reply));
}


status_t
BMediaRosterEx::ReleaseNodeAll(const media_node& node)
{
		CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	if (node.kind & NODE_KIND_NO_REFCOUNTING) {
		printf("BMediaRoster::ReleaseNodeAll, trying to release reference "
			"counting disabled timesource, node %ld, port %ld, team %ld\n",
			node.node, node.port, BPrivate::current_team());
		return B_OK;
	}

	server_release_node_request request;
	server_release_node_reply reply;
	status_t rv;

	request.node = node;
	request.team = BPrivate::current_team();

	TRACE("BMediaRoster::ReleaseNodeAll, node %ld, port %ld, team %ld\n",
		node.node, node.port, BPrivate::current_team());

	rv = QueryServer(SERVER_RELEASE_NODE_ALL, &request, sizeof(request), &reply,
		sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaRoster::ReleaseNodeAll FAILED, node %ld, port %ld, team "
			"%ld!\n", node.node, node.port, BPrivate::current_team());
	}
	return rv;
}


status_t
BMediaRosterEx::SetNodeCreator(media_node_id node, team_id creator)
{
	server_set_node_creator_request request;
	server_set_node_creator_reply reply;

	request.node = node;
	request.creator = creator;
	return QueryServer(SERVER_SET_NODE_CREATOR, &request, sizeof(request),
		&reply, sizeof(reply));
}


status_t
BMediaRosterEx::GetNode(node_type type, media_node* out_node,
	int32* out_input_id, BString* out_input_name)
{
	if (out_node == NULL)
		return B_BAD_VALUE;

	server_get_node_request request;
	server_get_node_reply reply;
	status_t rv;

	request.type = type;
	request.team = BPrivate::current_team();
	rv = QueryServer(SERVER_GET_NODE, &request, sizeof(request), &reply,
		sizeof(reply));
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
BMediaRosterEx::SetNode(node_type type, const media_node* node,
	const dormant_node_info* info, const media_input* input)
{
	server_set_node_request request;
	server_set_node_reply reply;

	request.type = type;
	request.use_node = node != NULL;
	if (node != NULL)
		request.node = *node;
	request.use_dni = info != NULL;
	if (info != NULL)
		request.dni = *info;
	request.use_input = input != NULL;
	if (input != NULL)
		request.input = *input;

	return QueryServer(SERVER_SET_NODE, &request, sizeof(request), &reply,
		sizeof(reply));
}


status_t
BMediaRosterEx::GetAllOutputs(const media_node& node, List<media_output>* list)
{
	int32 cookie;
	status_t rv;
	status_t result;

	PRINT(4, "BMediaRosterEx::GetAllOutputs() node %ld, port %ld\n", node.node,
		node.port);

	if (!(node.kind & B_BUFFER_PRODUCER)) {
		ERROR("BMediaRosterEx::GetAllOutputs: node %ld is not a "
			"B_BUFFER_PRODUCER\n", node.node);
		return B_MEDIA_BAD_NODE;
	}

	result = B_OK;
	cookie = 0;
	list->MakeEmpty();
	for (;;) {
		producer_get_next_output_request request;
		producer_get_next_output_reply reply;
		request.cookie = cookie;
		rv = QueryPort(node.port, PRODUCER_GET_NEXT_OUTPUT, &request,
			sizeof(request), &reply, sizeof(reply));
		if (rv != B_OK)
			break;
		cookie = reply.cookie;
		if (!list->Insert(reply.output)) {
			ERROR("GetAllOutputs: list->Insert failed\n");
			result = B_ERROR;
		}
		#if DEBUG >= 3
			PRINT(3," next cookie %ld, ", cookie);
			PRINT_OUTPUT("output ", reply.output);
		#endif
	}

	producer_dispose_output_cookie_request request;
	producer_dispose_output_cookie_reply reply;
	QueryPort(node.port, PRODUCER_DISPOSE_OUTPUT_COOKIE, &request,
		sizeof(request), &reply, sizeof(reply));

	return result;
}


status_t
BMediaRosterEx::GetAllOutputs(BBufferProducer* node, List<media_output>* list)
{
	int32 cookie;
	status_t result;

	PRINT(4, "BMediaRosterEx::GetAllOutputs() (by pointer) node %ld, port %ld\n", node->ID(), node->ControlPort());

	result = B_OK;
	cookie = 0;
	list->MakeEmpty();
	for (;;) {
		media_output output;
		if (B_OK != node->GetNextOutput(&cookie, &output))
			break;
		if (!list->Insert(output)) {
			ERROR("GetAllOutputs: list->Insert failed\n");
			result = B_ERROR;
		}
		#if DEBUG >= 3
			PRINT(3," next cookie %ld, ", cookie);
			PRINT_OUTPUT("output ", output);
		#endif
	}
	node->DisposeOutputCookie(cookie);
	return result;
}


status_t
BMediaRosterEx::GetAllInputs(const media_node& node, List<media_input>* list)
{
	int32 cookie;
	status_t rv;
	status_t result;

	PRINT(4, "BMediaRosterEx::GetAllInputs() node %ld, port %ld\n", node.node,
		node.port);

	if (!(node.kind & B_BUFFER_CONSUMER)) {
		ERROR("BMediaRosterEx::GetAllInputs: node %ld is not a "
			"B_BUFFER_CONSUMER\n", node.node);
		return B_MEDIA_BAD_NODE;
	}

	result = B_OK;
	cookie = 0;
	list->MakeEmpty();
	for (;;) {
		consumer_get_next_input_request request;
		consumer_get_next_input_reply reply;
		request.cookie = cookie;
		rv = QueryPort(node.port, CONSUMER_GET_NEXT_INPUT, &request,
			sizeof(request), &reply, sizeof(reply));
		if (rv != B_OK)
			break;
		cookie = reply.cookie;
		if (!list->Insert(reply.input)) {
			ERROR("GetAllInputs: list->Insert failed\n");
			result = B_ERROR;
		}
		#if DEBUG >= 3
			PRINT(3," next cookie %ld, ", cookie);
			PRINT_OUTPUT("input ", reply.input);
		#endif
	}

	consumer_dispose_input_cookie_request request;
	consumer_dispose_input_cookie_reply reply;
	QueryPort(node.port, CONSUMER_DISPOSE_INPUT_COOKIE, &request,
		sizeof(request), &reply, sizeof(reply));

	return result;
}


status_t
BMediaRosterEx::GetAllInputs(BBufferConsumer* node, List<media_input>* list)
{
	int32 cookie;
	status_t result;

	PRINT(4, "BMediaRosterEx::GetAllInputs() (by pointer) node %ld, port %ld\n", node->ID(), node->ControlPort());

	result = B_OK;
	cookie = 0;
	list->MakeEmpty();
	for (;;) {
		media_input input;
		if (B_OK != node->GetNextInput(&cookie, &input))
			break;
		if (!list->Insert(input)) {
			ERROR("GetAllInputs: list->Insert failed\n");
			result = B_ERROR;
		}
		#if DEBUG >= 3
			PRINT(3," next cookie %ld, ", cookie);
			PRINT_INPUT("input ", input);
		#endif
	}
	node->DisposeInputCookie(cookie);
	return result;
}


status_t
BMediaRosterEx::PublishOutputs(const media_node& node, List<media_output>* list)
{
	server_publish_outputs_request request;
	server_publish_outputs_reply reply;
	media_output* output;
	media_output* outputs;
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
		request.area = create_area("publish outputs", &start_addr,
			B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (request.area < B_OK) {
			ERROR("PublishOutputs: failed to create area, %#lx\n",
				request.area);
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

	rv = QueryServer(SERVER_PUBLISH_OUTPUTS, &request, sizeof(request),
		&reply, sizeof(reply));

	if (request.area != -1)
		delete_area(request.area);

	return rv;
}


status_t
BMediaRosterEx::PublishInputs(const media_node& node, List<media_input>* list)
{
	server_publish_inputs_request request;
	server_publish_inputs_reply reply;
	media_input* input;
	media_input* inputs;
	int32 count;
	status_t rv;

	count = list->CountItems();
	TRACE("PublishInputs: publishing %ld\n", count);

	request.node = node;
	request.count = count;
	if (count > MAX_INPUTS) {
		void* start_addr;
		size_t size;
		size = ROUND_UP_TO_PAGE(count * sizeof(media_input));
		request.area = create_area("publish inputs", &start_addr,
			B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (request.area < B_OK) {
			ERROR("PublishInputs: failed to create area, %#lx\n", request.area);
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

	rv = QueryServer(SERVER_PUBLISH_INPUTS, &request, sizeof(request),
		&reply, sizeof(reply));

	if (request.area != -1)
		delete_area(request.area);

	return rv;
}


BTimeSource*
BMediaRosterEx::MakeTimeSourceObject(media_node_id timeSourceID)
{
	media_node clone;
	status_t status = GetNodeFor(timeSourceID, &clone);
	if (status != B_OK) {
		ERROR("BMediaRosterEx::MakeTimeSourceObject: GetNodeFor failed: %s\n",
			strerror(status));
		return NULL;
	}

	BTimeSource* source = gTimeSourceObjectManager->GetTimeSource(clone);
	if (source == NULL) {
		ERROR("BMediaRosterEx::MakeTimeSourceObject: GetTimeSource failed\n");
		return NULL;
	}

	// TODO: release?
	ReleaseNode(clone);

	return source;
}


//	#pragma mark - public BMediaRoster


status_t
BMediaRoster::GetVideoInput(media_node* _node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(VIDEO_INPUT, _node);
}


status_t
BMediaRoster::GetAudioInput(media_node* _node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(AUDIO_INPUT, _node);
}


status_t
BMediaRoster::GetVideoOutput(media_node* _node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(VIDEO_OUTPUT, _node);
}


status_t
BMediaRoster::GetAudioMixer(media_node* _node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(AUDIO_MIXER, _node);
}


status_t
BMediaRoster::GetAudioOutput(media_node* _node)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(AUDIO_OUTPUT, _node);
}


status_t
BMediaRoster::GetAudioOutput(media_node* _node, int32* _inputID,
	BString* _inputName)
{
	CALLED();
	return MediaRosterEx(this)->GetNode(AUDIO_OUTPUT_EX, _node, _inputID,
		_inputName);
}


status_t
BMediaRoster::GetTimeSource(media_node* _node)
{
	CALLED();
	status_t rv;

	// TODO: need to do this in a nicer way.

	rv = MediaRosterEx(this)->GetNode(TIME_SOURCE, _node);
	if (rv != B_OK)
		return rv;

	// We don't do reference counting for timesources, that's why we
	// release the node immediately.
	ReleaseNode(*_node);

	// we need to remember to not use this node with server side reference counting
	_node->kind |= NODE_KIND_NO_REFCOUNTING;
	return B_OK;
}


status_t
BMediaRoster::SetVideoInput(const media_node& producer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(VIDEO_INPUT, &producer);
}


status_t
BMediaRoster::SetVideoInput(const dormant_node_info& producer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(VIDEO_INPUT, NULL, &producer);
}


status_t
BMediaRoster::SetAudioInput(const media_node& producer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_INPUT, &producer);
}


status_t
BMediaRoster::SetAudioInput(const dormant_node_info& producer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_INPUT, NULL, &producer);
}


status_t
BMediaRoster::SetVideoOutput(const media_node& consumer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(VIDEO_OUTPUT, &consumer);
}


status_t
BMediaRoster::SetVideoOutput(const dormant_node_info& consumer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(VIDEO_OUTPUT, NULL, &consumer);
}


status_t
BMediaRoster::SetAudioOutput(const media_node& consumer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_OUTPUT, &consumer);
}


status_t
BMediaRoster::SetAudioOutput(const media_input& input)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_OUTPUT, NULL, NULL, &input);
}


status_t
BMediaRoster::SetAudioOutput(const dormant_node_info& consumer)
{
	CALLED();
	return MediaRosterEx(this)->SetNode(AUDIO_OUTPUT, NULL, &consumer);
}


status_t
BMediaRoster::GetNodeFor(media_node_id node, media_node* clone)
{
	CALLED();
	if (clone == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_NODEID(node))
		return B_MEDIA_BAD_NODE;

	server_get_node_for_request request;
	server_get_node_for_reply reply;
	status_t rv;

	request.node_id = node;
	request.team = BPrivate::current_team();

	rv = QueryServer(SERVER_GET_NODE_FOR, &request, sizeof(request), &reply,
		sizeof(reply));
	if (rv != B_OK)
		return rv;

	*clone = reply.clone;
	return B_OK;
}


status_t
BMediaRoster::GetSystemTimeSource(media_node* clone)
{
	CALLED();
	status_t rv;

	// TODO: need to do this in a nicer way.

	rv = MediaRosterEx(this)->GetNode(SYSTEM_TIME_SOURCE, clone);
	if (rv != B_OK)
		return rv;

	// We don't do reference counting for timesources, that's why we
	// release the node immediately.
	ReleaseNode(*clone);

	// we need to remember to not use this node with server side reference
	// counting
	clone->kind |= NODE_KIND_NO_REFCOUNTING;

	return B_OK;
}


status_t
BMediaRoster::ReleaseNode(const media_node& node)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	if (node.kind & NODE_KIND_NO_REFCOUNTING) {
		printf("BMediaRoster::ReleaseNode, trying to release reference "
			"counting disabled timesource, node %ld, port %ld, team %ld\n",
			node.node, node.port, BPrivate::current_team());
		return B_OK;
	}

	server_release_node_request request;
	server_release_node_reply reply;
	status_t rv;

	request.node = node;
	request.team = BPrivate::current_team();

	TRACE("BMediaRoster::ReleaseNode, node %ld, port %ld, team %ld\n",
		node.node, node.port, BPrivate::current_team());

	rv = QueryServer(SERVER_RELEASE_NODE, &request, sizeof(request), &reply,
		sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaRoster::ReleaseNode FAILED, node %ld, port %ld, team "
			"%ld!\n", node.node, node.port, BPrivate::current_team());
	}
	return rv;
}


BTimeSource*
BMediaRoster::MakeTimeSourceFor(const media_node& forNode)
{
	// MakeTimeSourceFor() returns a BTimeSource object
	// corresponding to the specified node's time source.

	CALLED();

	if (IS_SYSTEM_TIMESOURCE(forNode)) {
		// special handling for the system time source
		TRACE("BMediaRoster::MakeTimeSourceFor, asked for system time "
			"source\n");
		return MediaRosterEx(this)->MakeTimeSourceObject(
			NODE_SYSTEM_TIMESOURCE_ID);
	}

	if (IS_INVALID_NODE(forNode)) {
		ERROR("BMediaRoster::MakeTimeSourceFor: for_node invalid, node %ld, "
			"port %ld, kinds 0x%lx\n", forNode.node, forNode.port,
			forNode.kind);
		return NULL;
	}

	TRACE("BMediaRoster::MakeTimeSourceFor: node %ld enter\n", forNode.node);

	node_get_timesource_request request;
	node_get_timesource_reply reply;
	BTimeSource *source;
	status_t rv;

	// ask the node to get it's current timesource id
	rv = QueryPort(forNode.port, NODE_GET_TIMESOURCE, &request,
		sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaRoster::MakeTimeSourceFor: request failed\n");
		return NULL;
	}

	source = MediaRosterEx(this)->MakeTimeSourceObject(reply.timesource_id);

	TRACE("BMediaRoster::MakeTimeSourceFor: node %ld leave\n", forNode.node);

	return source;
}


status_t
BMediaRoster::Connect(const media_source& from, const media_destination& to,
	media_format* _format, media_output* _output, media_input* _input)
{
	return BMediaRoster::Connect(from, to, _format, _output, _input, 0);
}


status_t
BMediaRoster::Connect(const media_source& from, const media_destination& to,
	media_format* io_format, media_output* out_output, media_input* out_input,
	uint32 in_flags, void* _reserved)
{
	CALLED();
	if (io_format == NULL || out_output == NULL || out_input == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_SOURCE(from)) {
		ERROR("BMediaRoster::Connect: media_source invalid\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (IS_INVALID_DESTINATION(to)) {
		ERROR("BMediaRoster::Connect: media_destination invalid\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	status_t rv;

	// find the output and input nodes
	// TODO: isn't there a easier way?
	media_node sourcenode;
	media_node destnode;
	rv = GetNodeFor(NodeIDFor(from.port), &sourcenode);
	if (rv != B_OK) {
		ERROR("BMediaRoster::Connect: Can't find source node for port %ld\n",
			from.port);
		return B_MEDIA_BAD_SOURCE;
	}
	ReleaseNode(sourcenode);
	rv = GetNodeFor(NodeIDFor(to.port), &destnode);
	if (rv != B_OK) {
		ERROR("BMediaRoster::Connect: Can't find destination node for port "
			"%ld\n", to.port);
		return B_MEDIA_BAD_DESTINATION;
	}
	ReleaseNode(destnode);

	if (!(sourcenode.kind & B_BUFFER_PRODUCER)) {
		ERROR("BMediaRoster::Connect: source node %ld is not a "
			"B_BUFFER_PRODUCER\n", sourcenode.node);
		return B_MEDIA_BAD_SOURCE;
	}
	if (!(destnode.kind & B_BUFFER_CONSUMER)) {
		ERROR("BMediaRoster::Connect: destination node %ld is not a "
			"B_BUFFER_CONSUMER\n", destnode.node);
		return B_MEDIA_BAD_DESTINATION;
	}

	producer_format_proposal_request request1;
	producer_format_proposal_reply reply1;

	PRINT_FORMAT("BMediaRoster::Connect calling "
		"BBufferProducer::FormatProposal with format  ", *io_format);

	// BBufferProducer::FormatProposal
	request1.output = from;
	request1.format = *io_format;
	rv = QueryPort(from.port, PRODUCER_FORMAT_PROPOSAL, &request1,
		sizeof(request1), &reply1, sizeof(reply1));
	if (rv != B_OK) {
		ERROR("BMediaRoster::Connect: aborted after "
			"BBufferProducer::FormatProposal, status = %#lx\n",rv);
		return rv;
	}
	// reply1.format now contains the format proposed by the producer

	consumer_accept_format_request request2;
	consumer_accept_format_reply reply2;

	PRINT_FORMAT("BMediaRoster::Connect calling "
		"BBufferConsumer::AcceptFormat with format    ", reply1.format);

	// BBufferConsumer::AcceptFormat
	request2.dest = to;
	request2.format = reply1.format;
	rv = QueryPort(to.port, CONSUMER_ACCEPT_FORMAT, &request2,
		sizeof(request2), &reply2, sizeof(reply2));
	if (rv != B_OK) {
		ERROR("BMediaRoster::Connect: aborted after "
			"BBufferConsumer::AcceptFormat, status = %#lx\n",rv);
		return rv;
	}
	// reply2.format now contains the format accepted by the consumer

	// BBufferProducer::PrepareToConnect
	producer_prepare_to_connect_request request3;
	producer_prepare_to_connect_reply reply3;

	PRINT_FORMAT("BMediaRoster::Connect calling "
		"BBufferProducer::PrepareToConnect with format", reply2.format);

	request3.source = from;
	request3.destination = to;
	request3.format = reply2.format;
	strcpy(request3.name, "XXX some default name"); // TODO: fix this
	rv = QueryPort(from.port, PRODUCER_PREPARE_TO_CONNECT, &request3,
		sizeof(request3), &reply3, sizeof(reply3));
	if (rv != B_OK) {
		ERROR("BMediaRoster::Connect: aborted after "
			"BBufferProducer::PrepareToConnect, status = %#lx\n",rv);
		return rv;
	}
	// reply3.format is still our pretty media format
	// reply3.out_source the real source to be used for the connection
	// reply3.name the name BBufferConsumer::Connected will see in the
	// outInput->name argument

	// BBufferConsumer::Connected
	consumer_connected_request request4;
	consumer_connected_reply reply4;
	status_t con_status;

	PRINT_FORMAT("BMediaRoster::Connect calling BBufferConsumer::Connected() "
		"with format       ", reply3.format);

	request4.input.node = destnode;
	request4.input.source = reply3.out_source;
	request4.input.destination = to;
	request4.input.format = reply3.format;
	strcpy(request4.input.name, reply3.name);

	con_status = QueryPort(to.port, CONSUMER_CONNECTED, &request4,
		sizeof(request4), &reply4, sizeof(reply4));
	if (con_status != B_OK) {
		ERROR("BMediaRoster::Connect: aborting after "
			"BBufferConsumer::Connected, status = %#lx\n",con_status);
		// we do NOT return here!
	}
	// con_status contains the status code to be supplied to
	// BBufferProducer::Connect's status argument
	// reply4.input contains the media_input that describes the connection
	// from the consumer point of view

	// BBufferProducer::Connect
	producer_connect_request request5;
	producer_connect_reply reply5;

	PRINT_FORMAT("BMediaRoster::Connect calling BBufferProducer::Connect with "
		"format         ", reply4.input.format);

	request5.error = con_status;
	request5.source = reply3.out_source;
	request5.destination = reply4.input.destination;
	request5.format = reply4.input.format;
	strcpy(request5.name, reply4.input.name);
	rv = QueryPort(reply4.input.source.port, PRODUCER_CONNECT, &request5,
		sizeof(request5), &reply5, sizeof(reply5));
	if (con_status != B_OK) {
		ERROR("BMediaRoster::Connect: aborted\n");
		return con_status;
	}
	if (rv != B_OK) {
		ERROR("BMediaRoster::Connect: aborted after BBufferProducer::Connect()"
			", status = %#lx\n",rv);
		return rv;
	}
	// reply5.name contains the name assigned to the connection by the producer

	// initilize connection info
	*io_format = reply4.input.format;
	*out_input = reply4.input;
	out_output->node = sourcenode;
	out_output->source = reply4.input.source;
	out_output->destination = reply4.input.destination;
	out_output->format = reply4.input.format;
	strcpy(out_output->name, reply5.name);

	// the connection is now made
	printf("BMediaRoster::Connect connection established!\n");
	PRINT_FORMAT("   format", *io_format);
	PRINT_INPUT("   input", *out_input);
	PRINT_OUTPUT("   output", *out_output);

	// TODO: register connection with server
	// TODO: we should just send a notification, instead of republishing all
	// endpoints
	List<media_output> outlist;
	List<media_input> inlist;
	if (MediaRosterEx(this)->GetAllOutputs(out_output->node , &outlist) == B_OK)
		MediaRosterEx(this)->PublishOutputs(out_output->node , &outlist);
	if (MediaRosterEx(this)->GetAllInputs(out_input->node , &inlist) == B_OK)
		MediaRosterEx(this)->PublishInputs(out_input->node, &inlist);

	// TODO: if (mute) BBufferProducer::EnableOutput(false)
	if (in_flags & B_CONNECT_MUTED) {
	}

	// send a notification
	BPrivate::media::notifications::ConnectionMade(*out_input, *out_output,
		*io_format);

	return B_OK;
};


status_t
BMediaRoster::Disconnect(media_node_id source_nodeid,
	const media_source& source, media_node_id destination_nodeid,
	const media_destination& destination)
{
	CALLED();
	if (IS_INVALID_NODEID(source_nodeid)) {
		ERROR("BMediaRoster::Disconnect: source media_node_id invalid\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (IS_INVALID_NODEID(destination_nodeid)) {
		ERROR("BMediaRoster::Disconnect: destination media_node_id invalid\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	if (IS_INVALID_SOURCE(source)) {
		ERROR("BMediaRoster::Disconnect: media_source invalid\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (IS_INVALID_DESTINATION(destination)) {
		ERROR("BMediaRoster::Disconnect: media_destination invalid\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	producer_disconnect_request request2;
	producer_disconnect_reply reply2;
	consumer_disconnected_request request1;
	consumer_disconnected_reply reply1;
	status_t rv1, rv2;

	// TODO: we should ask the server if this connection really exists

	request1.source = source;
	request1.destination = destination;
	request2.source = source;
	request2.destination = destination;

	rv1 = QueryPort(source.port, PRODUCER_DISCONNECT, &request1,
		sizeof(request1), &reply1, sizeof(reply1));
	rv2 = QueryPort(destination.port, CONSUMER_DISCONNECTED, &request2,
		sizeof(request2), &reply2, sizeof(reply2));

	// TODO: unregister connection with server
	// TODO: we should just send a notification, instead of republishing all
	// endpoints
	List<media_output> outlist;
	List<media_input> inlist;
	media_node sourcenode;
	media_node destnode;
	if (GetNodeFor(source_nodeid, &sourcenode) == B_OK) {
		if (!(sourcenode.kind & B_BUFFER_PRODUCER)) {
			ERROR("BMediaRoster::Disconnect: source_nodeid %ld is not a "
				"B_BUFFER_PRODUCER\n", source_nodeid);
		}
		if (MediaRosterEx(this)->GetAllOutputs(sourcenode , &outlist) == B_OK)
			MediaRosterEx(this)->PublishOutputs(sourcenode , &outlist);
		ReleaseNode(sourcenode);
	} else {
		ERROR("BMediaRoster::Disconnect: GetNodeFor source_nodeid %ld failed\n", source_nodeid);
	}
	if (GetNodeFor(destination_nodeid, &destnode) == B_OK) {
		if (!(destnode.kind & B_BUFFER_CONSUMER)) {
			ERROR("BMediaRoster::Disconnect: destination_nodeid %ld is not a "
				"B_BUFFER_CONSUMER\n", destination_nodeid);
		}
		if (MediaRosterEx(this)->GetAllInputs(destnode , &inlist) == B_OK)
			MediaRosterEx(this)->PublishInputs(destnode, &inlist);
		ReleaseNode(destnode);
	} else {
		ERROR("BMediaRoster::Disconnect: GetNodeFor destination_nodeid %ld "
			"failed\n", destination_nodeid);
	}

	// send a notification
	BPrivate::media::notifications::ConnectionBroken(source, destination);

	return rv1 != B_OK || rv2 != B_OK ? B_ERROR : B_OK;
}


status_t
BMediaRoster::Disconnect(const media_output& output, const media_input& input)
{
	if (IS_INVALID_NODEID(output.node.node)) {
		printf("BMediaRoster::Disconnect: output.node.node %ld invalid\n",
			output.node.node);
		return B_MEDIA_BAD_SOURCE;
	}
	if (IS_INVALID_NODEID(input.node.node)) {
		printf("BMediaRoster::Disconnect: input.node.node %ld invalid\n",
			input.node.node);
		return B_MEDIA_BAD_DESTINATION;
	}
	if (!(output.node.kind & B_BUFFER_PRODUCER)) {
		printf("BMediaRoster::Disconnect: output.node.kind 0x%lx is no "
			"B_BUFFER_PRODUCER\n", output.node.kind);
		return B_MEDIA_BAD_SOURCE;
	}
	if (!(input.node.kind & B_BUFFER_CONSUMER)) {
		printf("BMediaRoster::Disconnect: input.node.kind 0x%lx is no "
			"B_BUFFER_PRODUCER\n", input.node.kind);
		return B_MEDIA_BAD_DESTINATION;
	}
	if (input.source.port != output.source.port) {
		printf("BMediaRoster::Disconnect: input.source.port %ld doesn't match "
			"output.source.port %ld\n", input.source.port, output.source.port);
		return B_MEDIA_BAD_SOURCE;
	}
	if (input.source.id != output.source.id) {
		printf("BMediaRoster::Disconnect: input.source.id %ld doesn't match "
			"output.source.id %ld\n", input.source.id, output.source.id);
		return B_MEDIA_BAD_SOURCE;
	}
	if (input.destination.port != output.destination.port) {
		printf("BMediaRoster::Disconnect: input.destination.port %ld doesn't "
			"match output.destination.port %ld\n", input.destination.port,
			output.destination.port);
		return B_MEDIA_BAD_DESTINATION;
	}
	if (input.destination.id != output.destination.id) {
		printf("BMediaRoster::Disconnect: input.destination.id %ld doesn't "
			"match output.destination.id %ld\n", input.destination.id,
			output.destination.id);
		return B_MEDIA_BAD_DESTINATION;
	}

	return Disconnect(output.node.node, output.source, input.node.node,
		input.destination);
}


status_t
BMediaRoster::StartNode(const media_node& node, bigtime_t atPerformanceTime)
{
	CALLED();
	if (node.node <= 0)
		return B_MEDIA_BAD_NODE;

	TRACE("BMediaRoster::StartNode, node %ld, at perf %Ld\n", node.node,
		atPerformanceTime);

	node_start_command command;
	command.performance_time = atPerformanceTime;

	return SendToPort(node.port, NODE_START, &command, sizeof(command));
}


status_t
BMediaRoster::StopNode(const media_node& node, bigtime_t atPerformanceTime,
	bool immediate)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	TRACE("BMediaRoster::StopNode, node %ld, at perf %Ld %s\n", node.node,
		atPerformanceTime, immediate ? "NOW" : "");

	node_stop_command command;
	command.performance_time = atPerformanceTime;
	command.immediate = immediate;

	return SendToPort(node.port, NODE_STOP, &command, sizeof(command));
}


status_t
BMediaRoster::SeekNode(const media_node& node, bigtime_t toMediaTime,
	bigtime_t atPerformanceTime)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	TRACE("BMediaRoster::SeekNode, node %ld, at perf %Ld, to perf %Ld\n",
		node.node, atPerformanceTime, toMediaTime);

	node_seek_command command;
	command.media_time = toMediaTime;
	command.performance_time = atPerformanceTime;

	return SendToPort(node.port, NODE_SEEK, &command, sizeof(command));
}


status_t
BMediaRoster::StartTimeSource(const media_node& node, bigtime_t atRealTime)
{
	CALLED();
	if (IS_SYSTEM_TIMESOURCE(node)) {
		// TODO: debug this
		//ERROR("BMediaRoster::StartTimeSource node %ld is system timesource\n", node.node);
		return B_OK;
	}
//	if (IS_SHADOW_TIMESOURCE(node)) {
//		// TODO: debug this
//		ERROR("BMediaRoster::StartTimeSource node %ld is shadow timesource\n", node.node);
//		return B_OK;
//	}
	if (IS_INVALID_NODE(node)) {
		ERROR("BMediaRoster::StartTimeSource node %ld invalid\n", node.node);
		return B_MEDIA_BAD_NODE;
	}
	if ((node.kind & B_TIME_SOURCE) == 0) {
		ERROR("BMediaRoster::StartTimeSource node %ld is no timesource\n",
			node.node);
		return B_MEDIA_BAD_NODE;
	}

	TRACE("BMediaRoster::StartTimeSource, node %ld, at real %Ld\n", node.node,
		atRealTime);

	BTimeSource::time_source_op_info msg;
	msg.op = BTimeSource::B_TIMESOURCE_START;
	msg.real_time = atRealTime;

	return write_port(node.port, TIMESOURCE_OP, &msg, sizeof(msg));
}


status_t
BMediaRoster::StopTimeSource(const media_node& node, bigtime_t atRealTime,
	bool immediate)
{
	CALLED();
	if (IS_SYSTEM_TIMESOURCE(node)) {
		// TODO: debug this
		//ERROR("BMediaRoster::StopTimeSource node %ld is system timesource\n", node.node);
		return B_OK;
	}
//	if (IS_SHADOW_TIMESOURCE(node)) {
//		// TODO: debug this
//		ERROR("BMediaRoster::StopTimeSource node %ld is shadow timesource\n", node.node);
//		return B_OK;
//	}
	if (IS_INVALID_NODE(node)) {
		ERROR("BMediaRoster::StopTimeSource node %ld invalid\n", node.node);
		return B_MEDIA_BAD_NODE;
	}
	if ((node.kind & B_TIME_SOURCE) == 0) {
		ERROR("BMediaRoster::StopTimeSource node %ld is no timesource\n",
			node.node);
		return B_MEDIA_BAD_NODE;
	}

	TRACE("BMediaRoster::StopTimeSource, node %ld, at real %Ld %s\n",
		node.node, atRealTime, immediate ? "NOW" : "");

	BTimeSource::time_source_op_info msg;
	msg.op = immediate ? BTimeSource::B_TIMESOURCE_STOP_IMMEDIATELY
		: BTimeSource::B_TIMESOURCE_STOP;
	msg.real_time = atRealTime;

	return write_port(node.port, TIMESOURCE_OP, &msg, sizeof(msg));
}


status_t
BMediaRoster::SeekTimeSource(const media_node& node,
	bigtime_t toPerformanceTime, bigtime_t atRealTime)
{
	CALLED();
	if (IS_SYSTEM_TIMESOURCE(node)) {
		// TODO: debug this
		// ERROR("BMediaRoster::SeekTimeSource node %ld is system timesource\n", node.node);
		// you can't seek the system time source, but
		// returning B_ERROR would break StampTV
		return B_OK;
	}
//	if (IS_SHADOW_TIMESOURCE(node)) {
//		// TODO: debug this
//		ERROR("BMediaRoster::SeekTimeSource node %ld is shadow timesource\n", node.node);
//		return B_OK;
//	}
	if (IS_INVALID_NODE(node)) {
		ERROR("BMediaRoster::SeekTimeSource node %ld invalid\n", node.node);
		return B_MEDIA_BAD_NODE;
	}
	if ((node.kind & B_TIME_SOURCE) == 0) {
		ERROR("BMediaRoster::SeekTimeSource node %ld is no timesource\n",
			node.node);
		return B_MEDIA_BAD_NODE;
	}

	TRACE("BMediaRoster::SeekTimeSource, node %ld, at real %Ld, to perf %Ld\n",
		node.node, atRealTime, toPerformanceTime);

	BTimeSource::time_source_op_info msg;
	msg.op = BTimeSource::B_TIMESOURCE_SEEK;
	msg.real_time = atRealTime;
	msg.performance_time = toPerformanceTime;

	return write_port(node.port, TIMESOURCE_OP, &msg, sizeof(msg));
}


status_t
BMediaRoster::SyncToNode(const media_node& node, bigtime_t atTime,
	bigtime_t timeout)
{
	UNIMPLEMENTED();
	return B_OK;
}


status_t
BMediaRoster::SetRunModeNode(const media_node& node, BMediaNode::run_mode mode)
{
	TRACE("BMediaRoster::SetRunModeNode, node %ld, mode %d\n", node.node, mode);
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	node_set_run_mode_command msg;
	msg.mode = mode;

	return write_port(node.port, NODE_SET_RUN_MODE, &msg, sizeof(msg));
}


status_t
BMediaRoster::PrerollNode(const media_node& node)
{
	CALLED();
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	char dummy;
	return write_port(node.port, NODE_PREROLL, &dummy, sizeof(dummy));
}


status_t
BMediaRoster::RollNode(const media_node& node, bigtime_t startPerformance,
	bigtime_t stopPerformance, bigtime_t atMediaTime)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::SetProducerRunModeDelay(const media_node& node,
	bigtime_t delay, BMediaNode::run_mode mode)
{
	TRACE("BMediaRoster::SetProducerRunModeDelay, node %ld, delay %Ld, "
		"mode %d\n", node.node, delay, mode);
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;
	if ((node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;

	producer_set_run_mode_delay_command command;
	command.mode = mode;
	command.delay = delay;

	return SendToPort(node.port, PRODUCER_SET_RUN_MODE_DELAY, &command,
		sizeof(command));
}


status_t
BMediaRoster::SetProducerRate(const media_node& producer, int32 numer,
	int32 denom)
{
	CALLED();
	if (IS_INVALID_NODE(producer))
		return B_MEDIA_BAD_NODE;
	if ((producer.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;

	producer_set_play_rate_request request;
	request.numer = numer;
	request.denom = denom;
	status_t status = write_port(producer.node, PRODUCER_SET_PLAY_RATE,
		&request, sizeof(request));
	if (status != B_OK)
		return status;

	producer_set_play_rate_reply reply;
	int32 code;
	status = read_port(request.reply_port, &code, &reply, sizeof(reply));

	return status < B_OK ? status : reply.result;
}


/*!	Nodes will have available inputs/outputs as long as they are capable
	of accepting more connections. The node may create an additional
	output or input as the currently available is taken into usage.
*/
status_t
BMediaRoster::GetLiveNodeInfo(const media_node& node,
	live_node_info* out_live_info)
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

	rv = QueryServer(SERVER_GET_LIVE_NODE_INFO, &request, sizeof(request),
		&reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*out_live_info = reply.live_info;
	return B_OK;
}


status_t
BMediaRoster::GetLiveNodes(live_node_info* liveNodes, int32* _totalCount,
	const media_format* hasInput, const media_format* hasOutput,
	const char* name, uint64 nodeKinds)
{
	CALLED();
	if (liveNodes == NULL || _totalCount == NULL || *_totalCount <= 0)
		return B_BAD_VALUE;

	// TODO: we also support the wildcard search as GetDormantNodes does.
	// This needs to be documented

	server_get_live_nodes_request request;
	request.team = BPrivate::current_team();

	request.max_count = *_totalCount;
	request.has_input = hasInput != NULL;
	if (hasInput != NULL) {
		// TODO: we should not make a flat copy of media_format
		request.input_format = *hasInput;
	}
	request.has_output = hasOutput != NULL;
	if (hasOutput != NULL) {
		// TODO: we should not make a flat copy of media_format
		request.output_format = *hasOutput;
	}
	request.has_name = name != NULL;
	if (name != NULL)
		strlcpy(request.name, name, sizeof(request.name));
	request.require_kinds = nodeKinds;

	server_get_live_nodes_reply reply;
	status_t status = QueryServer(SERVER_GET_LIVE_NODES, &request,
		sizeof(request), &reply, sizeof(reply));
	if (status != B_OK) {
		ERROR("BMediaRoster::GetLiveNodes failed querying server: %s\n",
			strerror(status));
		*_totalCount = 0;
		return status;
	}

	const live_node_info* info;
	if (reply.area >= 0)
		info = (live_node_info*)reply.address;
	else
		info = reply.live_info;

	for (int32 i = 0; i < reply.count; i++)
		liveNodes[i] = info[i];

	if (reply.area >= 0)
		delete_area(reply.area);

	*_totalCount = reply.count;
	return B_OK;
}


status_t
BMediaRoster::GetFreeInputsFor(const media_node& node,
	media_input * out_free_inputs, int32 buf_num_inputs,
	int32 * out_total_count, media_type filter_type)
{
	CALLED();
	if (IS_INVALID_NODE(node)) {
		ERROR("BMediaRoster::GetFreeInputsFor: node %ld, port %ld invalid\n",
			node.node, node.port);
		return B_MEDIA_BAD_NODE;
	}
	if ((node.kind & B_BUFFER_CONSUMER) == 0) {
		ERROR("BMediaRoster::GetFreeInputsFor: node %ld, port %ld is not a "
			"consumer\n", node.node, node.port);
		return B_MEDIA_BAD_NODE;
	}
	if (out_free_inputs == NULL || out_total_count == NULL)
		return B_BAD_VALUE;

	List<media_input> list;
	media_input *input;
	status_t rv;

	*out_total_count = 0;

	rv = MediaRosterEx(this)->GetAllInputs(node, &list);
	if (B_OK != rv)
		return rv;

	PRINT(4, "BMediaRoster::GetFreeInputsFor node %ld, max %ld, filter-type "
		"%ld\n", node.node, buf_num_inputs, filter_type);

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&input);) {
		if (filter_type != B_MEDIA_UNKNOWN_TYPE
			&& filter_type != input->format.type) {
			// media_type used, but doesn't match
			continue;
		}
		if (input->source != media_source::null) {
			// consumer source already connected
			continue;
		}

		out_free_inputs[i] = *input;
		*out_total_count += 1;
		buf_num_inputs -= 1;
		#if DEBUG >= 3
			PRINT_OUTPUT("  input", out_free_inputs[i]);
		#endif
		if (buf_num_inputs == 0)
			break;
		i++;
	}

	MediaRosterEx(this)->PublishInputs(node, &list);
	return B_OK;
}


status_t
BMediaRoster::GetConnectedInputsFor(const media_node& node,
	media_input* out_active_inputs, int32 buf_num_inputs,
	int32* out_total_count)
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

	PRINT(4, "BMediaRoster::GetConnectedInputsFor node %ld, max %ld\n",
		node.node, buf_num_inputs);

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&input);) {
		if (input->source == media_source::null)
			continue; // consumer source not connected
		out_active_inputs[i] = *input;
		*out_total_count += 1;
		buf_num_inputs -= 1;
		#if DEBUG >= 3
			PRINT_OUTPUT("  input ", out_active_inputs[i]);
		#endif
		if (buf_num_inputs == 0)
			break;
		i++;
	}

	MediaRosterEx(this)->PublishInputs(node, &list);
	return B_OK;
}


status_t
BMediaRoster::GetAllInputsFor(const media_node& node, media_input* out_inputs,
	int32 buf_num_inputs, int32* out_total_count)
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

	PRINT(4, "BMediaRoster::GetAllInputsFor node %ld, max %ld\n", node.node,
		buf_num_inputs);

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&input); i++) {
		out_inputs[i] = *input;
		*out_total_count += 1;
		buf_num_inputs -= 1;
		#if DEBUG >= 3
			PRINT_OUTPUT("  input ", out_inputs[i]);
		#endif
		if (buf_num_inputs == 0)
			break;
	}

	MediaRosterEx(this)->PublishInputs(node, &list);
	return B_OK;
}


status_t
BMediaRoster::GetFreeOutputsFor(const media_node& node,
	media_output* out_free_outputs, int32 buf_num_outputs,
	int32* out_total_count, media_type filter_type)
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

	PRINT(4, "BMediaRoster::GetFreeOutputsFor node %ld, max %ld, filter-type "
		"%ld\n", node.node, buf_num_outputs, filter_type);

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&output);) {
		if (filter_type != B_MEDIA_UNKNOWN_TYPE
			&& filter_type != output->format.type) {
			// media_type used, but doesn't match
			continue;
		}
		if (output->destination != media_destination::null) {
			// producer destination already connected
			continue;
		}

		out_free_outputs[i] = *output;
		*out_total_count += 1;
		buf_num_outputs -= 1;
		#if DEBUG >= 3
			PRINT_OUTPUT("  output ", out_free_outputs[i]);
		#endif
		if (buf_num_outputs == 0)
			break;
		i++;
	}

	MediaRosterEx(this)->PublishOutputs(node, &list);
	return B_OK;
}


status_t
BMediaRoster::GetConnectedOutputsFor(const media_node& node,
	media_output* out_active_outputs, int32 buf_num_outputs,
	int32* out_total_count)
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

	PRINT(4, "BMediaRoster::GetConnectedOutputsFor node %ld, max %ld\n",
		node.node, buf_num_outputs);

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&output);) {
		if (output->destination == media_destination::null) {
			// producer destination not connected
			continue;
		}
		out_active_outputs[i] = *output;
		*out_total_count += 1;
		buf_num_outputs -= 1;
		#if DEBUG >= 3
			PRINT_OUTPUT("  output ", out_active_outputs[i]);
		#endif
		if (buf_num_outputs == 0)
			break;
		i++;
	}

	MediaRosterEx(this)->PublishOutputs(node, &list);
	return B_OK;
}


status_t
BMediaRoster::GetAllOutputsFor(const media_node& node,
	media_output* out_outputs, int32 buf_num_outputs, int32* out_total_count)
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

	PRINT(4, "BMediaRoster::GetAllOutputsFor node %ld, max %ld\n", node.node,
		buf_num_outputs);

	int32 i;
	for (i = 0, list.Rewind(); list.GetNext(&output); i++) {
		out_outputs[i] = *output;
		*out_total_count += 1;
		buf_num_outputs -= 1;
		#if DEBUG >= 3
			PRINT_OUTPUT("  output ", out_outputs[i]);
		#endif
		if (buf_num_outputs == 0)
			break;
	}

	MediaRosterEx(this)->PublishOutputs(node, &list);
	return B_OK;
}


status_t
BMediaRoster::StartWatching(const BMessenger& where)
{
	CALLED();
	if (!where.IsValid()) {
		ERROR("BMediaRoster::StartWatching: messenger invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Register(where, media_node::null,
		B_MEDIA_WILDCARD);
}


status_t
BMediaRoster::StartWatching(const BMessenger & where, int32 notificationType)
{
	CALLED();
	if (!where.IsValid()) {
		ERROR("BMediaRoster::StartWatching: messenger invalid!\n");
		return B_BAD_VALUE;
	}
	if (!BPrivate::media::notifications::IsValidNotificationRequest(false,
			notificationType)) {
		ERROR("BMediaRoster::StartWatching: notificationType invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Register(where, media_node::null,
		notificationType);
}


status_t
BMediaRoster::StartWatching(const BMessenger& where, const media_node& node,
	int32 notificationType)
{
	CALLED();
	if (!where.IsValid()) {
		ERROR("BMediaRoster::StartWatching: messenger invalid!\n");
		return B_BAD_VALUE;
	}
	if (IS_INVALID_NODE(node)) {
		ERROR("BMediaRoster::StartWatching: node invalid!\n");
		return B_MEDIA_BAD_NODE;
	}
	if (!BPrivate::media::notifications::IsValidNotificationRequest(true,
			notificationType)) {
		ERROR("BMediaRoster::StartWatching: notificationType invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Register(where, node,
		notificationType);
}


status_t
BMediaRoster::StopWatching(const BMessenger& where)
{
	CALLED();
	// messenger may already be invalid, so we don't check this
	return BPrivate::media::notifications::Unregister(where, media_node::null,
		B_MEDIA_WILDCARD);
}


status_t
BMediaRoster::StopWatching(const BMessenger& where, int32 notificationType)
{
	CALLED();
	// messenger may already be invalid, so we don't check this
	if (!BPrivate::media::notifications::IsValidNotificationRequest(false,
			notificationType)) {
		ERROR("BMediaRoster::StopWatching: notificationType invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Unregister(where, media_node::null,
		notificationType);
}


status_t
BMediaRoster::StopWatching(const BMessenger& where, const media_node& node,
	int32 notificationType)
{
	CALLED();
	// messenger may already be invalid, so we don't check this
	if (IS_INVALID_NODE(node)) {
		ERROR("BMediaRoster::StopWatching: node invalid!\n");
		return B_MEDIA_BAD_NODE;
	}
	if (!BPrivate::media::notifications::IsValidNotificationRequest(true,
			notificationType)) {
		ERROR("BMediaRoster::StopWatching: notificationType invalid!\n");
		return B_BAD_VALUE;
	}
	return BPrivate::media::notifications::Unregister(where, node,
		notificationType);
}


status_t
BMediaRoster::RegisterNode(BMediaNode* node)
{
	CALLED();
	// addon-id = -1 (unused), addon-flavor-id = 0 (unused, too)
	return MediaRosterEx(this)->RegisterNode(node, -1, 0);
}


status_t
BMediaRosterEx::RegisterNode(BMediaNode* node, media_addon_id addOnID,
	int32 flavorID)
{
	CALLED();
	if (node == NULL)
		return B_BAD_VALUE;

	// some sanity check
	// I'm not sure if the media kit warrants to call BMediaNode::AddOn() here.
	// Perhaps we don't need it.
	DEBUG_ONLY(
		int32 testFlavorID;
		BMediaAddOn* addon = node->AddOn(&testFlavorID);

		ASSERT(addOnID == (addon != NULL ? addon->AddonID() : -1));
//		ASSERT(flavorID == testFlavorID);
	);

	server_register_node_request request;
	server_register_node_reply reply;

	request.add_on_id = addOnID;
	request.flavor_id = flavorID;
	strcpy(request.name, node->Name());
	request.kinds = node->Kinds();
	request.port = node->ControlPort();
	request.team = BPrivate::current_team();

	TRACE("BMediaRoster::RegisterNode: sending SERVER_REGISTER_NODE: port "
		"%ld, kinds 0x%Lx, team %ld, name '%s'\n", request.port, request.kinds,
		request.team, request.name);

	status_t status = QueryServer(SERVER_REGISTER_NODE, &request,
		sizeof(request), &reply, sizeof(reply));
	if (status != B_OK) {
		ERROR("BMediaRoster::RegisterNode: failed to register node %s: %s\n",
			node->Name(), strerror(status));
		return status;
	}

	TRACE("BMediaRoster::RegisterNode: QueryServer SERVER_REGISTER_NODE "
		"finished\n");

	// we are a friend class of BMediaNode and initialize this member variable
	node->fNodeID = reply.node_id;
	ASSERT(reply.node_id == node->Node().node);
	ASSERT(reply.node_id == node->ID());

	// call the callback
	node->NodeRegistered();

	TRACE("BMediaRoster::RegisterNode: NodeRegistered callback finished\n");

	// if the BMediaNode also inherits from BTimeSource, we need to call
	// BTimeSource::FinishCreate()
	if ((node->Kinds() & B_TIME_SOURCE) != 0) {
		if (BTimeSource* timeSource = dynamic_cast<BTimeSource*>(node))
			timeSource->FinishCreate();
	}

	TRACE("BMediaRoster::RegisterNode: publishing inputs/outputs\n");

	// register existing inputs and outputs with the
	// media_server, this allows GetLiveNodes() to work
	// with created, but unconnected nodes.
	// The node control loop might not be running, or might deadlock
	// if we send a message and wait for a reply here.
	// We have a pointer to the node, and thus call the functions directly

	if ((node->Kinds() & B_BUFFER_PRODUCER) != 0) {
		if (BBufferProducer* producer = dynamic_cast<BBufferProducer*>(node)) {
			List<media_output> list;
			if (GetAllOutputs(producer, &list) == B_OK)
				PublishOutputs(node->Node(), &list);
		}
	}
	if ((node->Kinds() & B_BUFFER_CONSUMER) != 0) {
		if (BBufferConsumer* consumer = dynamic_cast<BBufferConsumer*>(node)) {
			List<media_input> list;
			if (GetAllInputs(consumer, &list) == B_OK)
				PublishInputs(node->Node(), &list);
		}
	}

	TRACE("BMediaRoster::RegisterNode: sending NodesCreated\n");

	BPrivate::media::notifications::NodesCreated(&reply.node_id, 1);

	TRACE("BMediaRoster::RegisterNode: finished\n");

/*
	TRACE("BMediaRoster::RegisterNode: registered node name '%s', id %ld,
		addon %ld, flavor %ld\n", node->Name(), node->ID(), addOnID, flavorID);
	TRACE("BMediaRoster::RegisterNode: node this               %p\n", node);
	TRACE("BMediaRoster::RegisterNode: node fConsumerThis      %p\n",
		node->fConsumerThis);
	TRACE("BMediaRoster::RegisterNode: node fProducerThis      %p\n",
		node->fProducerThis);
	TRACE("BMediaRoster::RegisterNode: node fFileInterfaceThis %p\n",
		node->fFileInterfaceThis);
	TRACE("BMediaRoster::RegisterNode: node fControllableThis  %p\n",
		node->fControllableThis);
	TRACE("BMediaRoster::RegisterNode: node fTimeSourceThis    %p\n",
		node->fTimeSourceThis);
*/
	return B_OK;
}


status_t
BMediaRoster::UnregisterNode(BMediaNode* node)
{
	CALLED();
	if (node == NULL)
		return B_BAD_VALUE;

	TRACE("BMediaRoster::UnregisterNode %ld (%p)\n", node->ID(), node);

	if ((node->fKinds & NODE_KIND_NO_REFCOUNTING) !=0) {
		TRACE("BMediaRoster::UnregisterNode, trying to unregister reference "
			"counting disabled timesource, node %ld, port %ld, team %ld\n",
			node->ID(), node->ControlPort(), BPrivate::current_team());
		return B_OK;
	}
	if (node->ID() == NODE_UNREGISTERED_ID) {
		PRINT(1, "Warning: BMediaRoster::UnregisterNode: node id %ld, name "
			"'%s' already unregistered\n", node->ID(), node->Name());
		return B_OK;
	}
	if (node->fRefCount != 0) {
		PRINT(1, "Warning: BMediaRoster::UnregisterNode: node id %ld, name "
			"'%s' has local reference count of %ld\n", node->ID(), node->Name(),
			node->fRefCount);
		// no return here, we continue and unregister!
	}

	// Calling BMediaAddOn::GetConfigurationFor(BMediaNode *node,
	// BMessage *config) if this node was instanciated by an add-on needs to
	// be done *somewhere*
	// We can't do it here because it is already to late (destructor of the node
	// might have been called).

	server_unregister_node_request request;
	request.node_id = node->ID();
	request.team = BPrivate::current_team();

	// send a notification
	BPrivate::media::notifications::NodesDeleted(&request.node_id, 1);

	server_unregister_node_reply reply;
	status_t status = QueryServer(SERVER_UNREGISTER_NODE, &request,
		sizeof(request), &reply, sizeof(reply));
	if (status != B_OK) {
		ERROR("BMediaRoster::UnregisterNode: failed to unregister node id %ld, "
			"name '%s': %s\n", node->ID(), node->Name(), strerror(status));
		return status;
	}

	if (reply.add_on_id != -1) {
		// TODO: this doesn't look right
		// Small problem here, we can't use DormantNodeManager::PutAddOn(), as
		// UnregisterNode() is called by a dormant node itself (by the
		// destructor).
		// The add-on that contains the node needs to remain in memory until the
		// destructor execution is finished.
		// DormantNodeManager::PutAddOnDelayed() will delay unloading.
		gDormantNodeManager->PutAddOnDelayed(reply.add_on_id);

		status = MediaRosterEx(this)->DecrementAddonFlavorInstancesCount(
			reply.add_on_id, reply.flavor_id);
		if (status != B_OK) {
			ERROR("BMediaRoster::UnregisterNode: "
				"DecrementAddonFlavorInstancesCount() failed\n");
			// this is really a problem, but we can't fail now
		}
	}

	// we are a friend class of BMediaNode and invalidate this member variable
	node->fNodeID = NODE_UNREGISTERED_ID;

	return B_OK;
}


//!	Thread safe for multiple calls to Roster()
/*static*/ BMediaRoster*
BMediaRoster::Roster(status_t* out_error)
{
	static BLocker locker("BMediaRoster::Roster locker");
	locker.Lock();
	if (out_error)
		*out_error = B_OK;
	if (sDefaultInstance == NULL) {
		status_t err;
		sDefaultInstance = new (std::nothrow) BMediaRosterEx(&err);
		if (sDefaultInstance == NULL)
			err = B_NO_MEMORY;
		else if (err != B_OK) {
			if (sDefaultInstance) {
				sDefaultInstance->Lock();
				sDefaultInstance->Quit();
				sDefaultInstance = NULL;
			}
			if (out_error)
				*out_error = err;
		}
	}
	locker.Unlock();
	return sDefaultInstance;
}


/*static*/ BMediaRoster*
BMediaRoster::CurrentRoster()
{
	return sDefaultInstance;
}


status_t
BMediaRoster::SetTimeSourceFor(media_node_id node, media_node_id time_source)
{
	CALLED();
	if (IS_INVALID_NODEID(node) || IS_INVALID_NODEID(time_source))
		return B_BAD_VALUE;

	media_node clone;
	status_t rv, result;

	TRACE("BMediaRoster::SetTimeSourceFor: node %ld will be assigned time "
		"source %ld\n", node, time_source);
	TRACE("BMediaRoster::SetTimeSourceFor: node %ld time source %ld enter\n",
		node, time_source);

	// we need to get a clone of the node to have a port id
	rv = GetNodeFor(node, &clone);
	if (rv != B_OK) {
		ERROR("BMediaRoster::SetTimeSourceFor, GetNodeFor failed, node id "
			"%ld\n", node);
		return B_ERROR;
	}

	// we just send the request to set time_source-id as timesource to the node,
	// the NODE_SET_TIMESOURCE handler code will do the real assignment
	result = B_OK;
	node_set_timesource_command cmd;
	cmd.timesource_id = time_source;
	rv = SendToPort(clone.port, NODE_SET_TIMESOURCE, &cmd, sizeof(cmd));
	if (rv != B_OK) {
		ERROR("BMediaRoster::SetTimeSourceFor, sending NODE_SET_TIMESOURCE "
			"failed, node id %ld\n", node);
		result = B_ERROR;
	}

	// we release the clone
	rv = ReleaseNode(clone);
	if (rv != B_OK) {
		ERROR("BMediaRoster::SetTimeSourceFor, ReleaseNode failed, node id "
			"%ld\n", node);
		result = B_ERROR;
	}

	TRACE("BMediaRoster::SetTimeSourceFor: node %ld time source %ld leave\n",
		node, time_source);

	return result;
}


status_t
BMediaRoster::GetParameterWebFor(const media_node& node, BParameterWeb** _web)
{
	CALLED();
	if (_web == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;
	if ((node.kind & B_CONTROLLABLE) == 0)
		return B_MEDIA_BAD_NODE;

	controllable_get_parameter_web_request request;
	controllable_get_parameter_web_reply reply;
	int32 requestsize[] = {B_PAGE_SIZE, 4 * B_PAGE_SIZE, 16 * B_PAGE_SIZE,
		64 * B_PAGE_SIZE, 128 * B_PAGE_SIZE, 256 * B_PAGE_SIZE, 0};
	int32 size;

	// TODO: it might be better to query the node for the (current) parameter
	// size first
	for (int i = 0; (size = requestsize[i]) != 0; i++) {
		status_t rv;
		area_id area;
		void *data;
		area = create_area("parameter web data", &data, B_ANY_ADDRESS, size,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < B_OK) {
			ERROR("BMediaRoster::GetParameterWebFor couldn't create area of "
				"size %ld\n", size);
			return B_ERROR;
		}
		request.max_size = size;
		request.area = area;
		rv = QueryPort(node.port, CONTROLLABLE_GET_PARAMETER_WEB, &request,
			sizeof(request), &reply, sizeof(reply));
		if (rv != B_OK) {
			ERROR("BMediaRoster::GetParameterWebFor "
				"CONTROLLABLE_GET_PARAMETER_WEB failed\n");
			delete_area(area);
			return B_ERROR;
		}
		if (reply.size == 0) {
			// no parameter web available
			// TODO: should we return an error?
			ERROR("BMediaRoster::GetParameterWebFor node %ld has no parameter "
				"web\n", node.node);
			*_web = new (std::nothrow) BParameterWeb();
			delete_area(area);
			return *_web != NULL ? B_OK : B_NO_MEMORY;
		}
		if (reply.size > 0) {
			// we got a flattened parameter web!
			*_web = new (std::nothrow) BParameterWeb();
			if (*_web == NULL)
				rv = B_NO_MEMORY;
			else {
				printf("BMediaRoster::GetParameterWebFor Unflattening %ld "
					"bytes, 0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx\n",
					reply.size, ((uint32*)data)[0], ((uint32*)data)[1],
					((uint32*)data)[2], ((uint32*)data)[3]);

				rv = (*_web)->Unflatten(reply.code, data, reply.size);
			}
			if (rv != B_OK) {
				ERROR("BMediaRoster::GetParameterWebFor Unflatten failed, "
					"%s\n", strerror(rv));
				delete *_web;
			}
			delete_area(area);
			return rv;
		}
		delete_area(area);
		ASSERT(reply.size == -1);
		// parameter web data was too large
		// loop and try a larger size
	}
	ERROR("BMediaRoster::GetParameterWebFor node %ld has no parameter web "
		"larger than %ld\n", node.node, size);
	return B_ERROR;
}


status_t
BMediaRoster::StartControlPanel(const media_node& node, BMessenger* _messenger)
{
	CALLED();

	controllable_start_control_panel_request request;
	controllable_start_control_panel_reply reply;

	request.node = node;

	status_t rv;
	rv = QueryPort(node.port, CONTROLLABLE_START_CONTROL_PANEL, &request,
		sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	if (reply.team != -1 && _messenger != NULL)
		*_messenger = BMessenger(NULL, reply.team);

	return B_OK;
}


status_t
BMediaRoster::GetDormantNodes(dormant_node_info* _info, int32* _count,
	const media_format* hasInput, const media_format* hasOutput,
	const char* name, uint64 requireKinds, uint64 denyKinds)
{
	CALLED();
	if (_info == NULL || _count == NULL || *_count <= 0)
		return B_BAD_VALUE;

	server_get_dormant_nodes_request request;
	request.max_count = *_count;
	request.has_input = hasInput != NULL;
	if (hasInput != NULL) {
		// TODO: we should not make a flat copy of media_format
		request.input_format = *hasInput;
	}
	request.has_output = hasOutput != NULL;
	if (hasOutput != NULL) {
		// TODO: we should not make a flat copy of media_format
		request.output_format = *hasOutput;
	}

	request.has_name = name != NULL;
	if (name != NULL)
		strlcpy(request.name, name, sizeof(request.name));

	request.require_kinds = requireKinds;
	request.deny_kinds = denyKinds;

	server_get_dormant_nodes_reply reply;
	status_t status = QueryServer(SERVER_GET_DORMANT_NODES, &request,
		sizeof(request), &reply, sizeof(reply));
	if (status != B_OK)
		return status;

	*_count = reply.count;

	if (reply.count > 0) {
		int32 code;
		status = read_port(request.reply_port, &code, _info,
			reply.count * sizeof(dormant_node_info));
		if (status < B_OK)
			reply.result = status;
	}

	return reply.result;
}


/*!	This function is used to do the real work of instantiating a dormant node.
	It is either called by the media_addon_server to instantiate a global node,
	or it gets called from BMediaRoster::InstantiateDormantNode() to create a
	local one.

	Checks concerning global/local are not done here.
*/
status_t
BMediaRosterEx::InstantiateDormantNode(media_addon_id addonID, int32 flavorID,
	team_id creator, media_node *_node)
{
	// This function is always called from the correct context, if the node
	// is supposed to be global, it is called from the media_addon_server.

	// if B_FLAVOR_IS_GLOBAL, we need to use the BMediaAddOn object that
	// resides in the media_addon_server

	// RegisterNode() must be called for nodes instantiated from add-ons,
	// since the media kit warrants that it's done automatically.

	// addonID		Indicates the ID number of the media add-on in which the
	//				node resides.
	// flavorID		Indicates the internal ID number that the add-on uses to
	//				identify the flavor, this is the number that was published
	//				by BMediaAddOn::GetFlavorAt() in the
	//				flavor_info::internal_id field.
	// creator		The creator team is -1 if nodes are created locally. If
	//				created globally, it will contain (while called in
	//				media_addon_server context) the team-id of the team that
	//				requested the instantiation.

	TRACE("BMediaRosterEx::InstantiateDormantNode: addonID %ld, flavorID "
		"%ld\n", addonID, flavorID);

	// Get flavor_info from the server
	dormant_flavor_info info;
	status_t rv;
	rv = GetDormantFlavorInfo(addonID, flavorID, &info);
	if (rv != B_OK) {
		ERROR("BMediaRosterEx::InstantiateDormantNode error: failed to get "
			"dormant_flavor_info for addon-id %ld, flavor-id %ld\n", addonID,
			flavorID);
		return B_ERROR;
	}

	ASSERT(info.internal_id == flavorID);

	// load the BMediaAddOn object
	BMediaAddOn* addon = gDormantNodeManager->GetAddOn(addonID);
	if (addon == NULL) {
		ERROR("BMediaRosterEx::InstantiateDormantNode: GetAddon failed\n");
		return B_ERROR;
	}

	// Now we need to try to increment the use count of this addon flavor
	// in the server. This can fail if the total number instances of this
	// flavor is limited.
	rv = IncrementAddonFlavorInstancesCount(addonID, flavorID);
	if (rv != B_OK) {
		ERROR("BMediaRosterEx::InstantiateDormantNode error: can't create "
			"more nodes for addon-id %ld, flavor-id %ld\n", addonID, flavorID);
		// Put the addon back into the pool
		gDormantNodeManager->PutAddOn(addonID);
		return B_ERROR;
	}

	BMessage config;
	rv = LoadNodeConfiguration(addonID, flavorID, &config);
	if (rv != B_OK) {
		ERROR("BMediaRosterEx::InstantiateDormantNode: couldn't load "
			"configuration for addon-id %ld, flavor-id %ld\n", addonID,
			flavorID);
		// do not return, this is a minor problem, not a reason to fail
	}

	status_t status = B_OK;
	BMediaNode* node = addon->InstantiateNodeFor(&info, &config, &status);
	if (node == NULL) {
		ERROR("BMediaRosterEx::InstantiateDormantNode: InstantiateNodeFor "
			"failed\n");

		// Put the addon back into the pool
		gDormantNodeManager->PutAddOn(addonID);

		// We must decrement the use count of this addon flavor in the
		// server to compensate the increment done in the beginning.
		rv = DecrementAddonFlavorInstancesCount(addonID, flavorID);
		if (rv != B_OK) {
			ERROR("BMediaRosterEx::InstantiateDormantNode: DecrementAddon"
				"FlavorInstancesCount failed\n");
		}
		return status != B_OK ? status : B_ERROR;
	}

	rv = RegisterNode(node, addonID, flavorID);
	if (rv != B_OK) {
		ERROR("BMediaRosterEx::InstantiateDormantNode: RegisterNode failed\n");
		delete node;
		// Put the addon back into the pool
		gDormantNodeManager->PutAddOn(addonID);
		// We must decrement the use count of this addon flavor in the
		// server to compensate the increment done in the beginning.
		rv = DecrementAddonFlavorInstancesCount(addonID, flavorID);
		if (rv != B_OK) {
			ERROR("BMediaRosterEx::InstantiateDormantNode: DecrementAddon"
				"FlavorInstancesCount failed\n");
		}
		return B_ERROR;
	}

	if (creator != -1) {
		// send a message to the server to assign team "creator" as creator
		// of node "node->ID()"
		printf("!!! BMediaRosterEx::InstantiateDormantNode assigning team "
			"%ld as creator of node %ld\n", creator, node->ID());

		rv = MediaRosterEx(this)->SetNodeCreator(node->ID(), creator);
		if (rv != B_OK) {
			ERROR("BMediaRosterEx::InstantiateDormantNode failed to assign "
				"team %ld as creator of node %ld\n", creator, node->ID());
			// do not return, this is a minor problem, not a reason to fail
		}
	}

	// RegisterNode() does remember the add-on id in the server
	// and UnregisterNode() will call DormantNodeManager::PutAddon()
	// when the node is unregistered.

	*_node = node->Node();

	TRACE("BMediaRosterEx::InstantiateDormantNode: addon-id %ld, flavor_id "
		"%ld instanciated as node %ld, port %ld in team %ld\n", addonID,
		flavorID, _node->node, _node->port, BPrivate::current_team());

	return B_OK;
}


status_t
BMediaRoster::InstantiateDormantNode(const dormant_node_info& info,
	media_node* _node, uint32 flags)
{
	CALLED();
	if (_node == NULL)
		return B_BAD_VALUE;
	if (info.addon <= B_OK) {
		ERROR("BMediaRoster::InstantiateDormantNode error: addon-id %ld "
			"invalid.\n", info.addon);
		return B_BAD_VALUE;
	}

	printf("BMediaRoster::InstantiateDormantNode: addon-id %ld, flavor_id "
		"%ld, flags 0x%lX\n", info.addon, info.flavor_id, flags);

	// Get flavor_info from the server
	// TODO: this is a little overhead, as we get the full blown
	// dormant_flavor_info,
	// TODO: but only need the flags.
	dormant_flavor_info flavorInfo;
	status_t rv;
	rv = MediaRosterEx(this)->GetDormantFlavorInfo(info.addon, info.flavor_id,
		&flavorInfo);
	if (rv != B_OK) {
		ERROR("BMediaRoster::InstantiateDormantNode: failed to get "
			"dormant_flavor_info for addon-id %ld, flavor-id %ld\n",
			info.addon, info.flavor_id);
		return B_NAME_NOT_FOUND;
	}

	ASSERT(flavorInfo.internal_id == info.flavor_id);

#if DEBUG
	printf("BMediaRoster::InstantiateDormantNode: name \"%s\", info \"%s\", "
		"flavor_flags 0x%lX, internal_id %ld, possible_count %ld\n",
		flavorInfo.name, flavorInfo.info, flavorInfo.flavor_flags,
		flavorInfo.internal_id, flavorInfo.possible_count);

	if ((flags & B_FLAVOR_IS_LOCAL) != 0) {
		printf("BMediaRoster::InstantiateDormantNode: caller requested "
			"B_FLAVOR_IS_LOCAL\n");
	}
	if ((flags & B_FLAVOR_IS_GLOBAL) != 0) {
		printf("BMediaRoster::InstantiateDormantNode: caller requested "
			"B_FLAVOR_IS_GLOBAL\n");
	}
	if ((flavorInfo.flavor_flags & B_FLAVOR_IS_LOCAL) != 0) {
		printf("BMediaRoster::InstantiateDormantNode: node requires "
			"B_FLAVOR_IS_LOCAL\n");
	}
	if ((flavorInfo.flavor_flags & B_FLAVOR_IS_GLOBAL) != 0) {
		printf("BMediaRoster::InstantiateDormantNode: node requires "
			"B_FLAVOR_IS_GLOBAL\n");
	}
#endif

	// Make sure that flags demanded by the dormant node and those requested
	// by the caller are not incompatible.
	if ((flavorInfo.flavor_flags & B_FLAVOR_IS_GLOBAL) != 0
		&& (flags & B_FLAVOR_IS_LOCAL) != 0) {
		ERROR("BMediaRoster::InstantiateDormantNode: requested "
			"B_FLAVOR_IS_LOCAL, but dormant node has B_FLAVOR_IS_GLOBAL\n");
		return B_NAME_NOT_FOUND;
	}
	if ((flavorInfo.flavor_flags & B_FLAVOR_IS_LOCAL) != 0
		&& (flags & B_FLAVOR_IS_GLOBAL) != 0) {
		ERROR("BMediaRoster::InstantiateDormantNode: requested "
			"B_FLAVOR_IS_GLOBAL, but dormant node has B_FLAVOR_IS_LOCAL\n");
		return B_NAME_NOT_FOUND;
	}

	// If either the node, or the caller requested to make the instance global
	// we will do it by forwarding this request into the media_addon_server,
	// which in turn will call BMediaRosterEx::InstantiateDormantNode to create
	// the node there and make it globally available.
	if ((flavorInfo.flavor_flags & B_FLAVOR_IS_GLOBAL) != 0
		|| (flags & B_FLAVOR_IS_GLOBAL) != 0) {
		TRACE("BMediaRoster::InstantiateDormantNode: creating global object "
			"in media_addon_server\n");

		add_on_server_instantiate_dormant_node_request request;
		add_on_server_instantiate_dormant_node_reply reply;
		request.add_on_id = info.addon;
		request.flavor_id = info.flavor_id;
		request.creator_team = BPrivate::current_team();
			// creator team is allowed to also release global nodes
		rv = QueryAddOnServer(ADD_ON_SERVER_INSTANTIATE_DORMANT_NODE, &request,
			sizeof(request), &reply, sizeof(reply));
		if (rv == B_OK)
			*_node = reply.node;
	} else {
		// creator team = -1, as this is a local node
		rv = MediaRosterEx(this)->InstantiateDormantNode(info.addon,
			info.flavor_id, -1, _node);
	}
	if (rv != B_OK) {
		*_node = media_node::null;
		return B_NAME_NOT_FOUND;
	}
	return B_OK;
}


status_t
BMediaRoster::InstantiateDormantNode(const dormant_node_info& info,
	media_node* _node)
{
	return InstantiateDormantNode(info, _node, 0);
}


status_t
BMediaRoster::GetDormantNodeFor(const media_node& node,
	dormant_node_info* _info)
{
	CALLED();
	if (_info == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;

	server_get_dormant_node_for_request request;
	server_get_dormant_node_for_reply reply;
	status_t rv;

	request.node = node;

	rv = QueryServer(SERVER_GET_DORMANT_NODE_FOR, &request, sizeof(request),
		&reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*_info = reply.node_info;
	return B_OK;
}


status_t
BMediaRosterEx::GetDormantFlavorInfo(media_addon_id addonID, int32 flavorID,
	dormant_flavor_info* _flavor)
{
	CALLED();
	if (_flavor == NULL)
		return B_BAD_VALUE;

	// TODO: better use an area here as well!

	server_get_dormant_flavor_info_reply* reply
		= (server_get_dormant_flavor_info_reply*)malloc(16300);
	if (reply == NULL)
		return B_NO_MEMORY;

	server_get_dormant_flavor_info_request request;
	request.add_on_id = addonID;
	request.flavor_id = flavorID;

	status_t status = QueryServer(SERVER_GET_DORMANT_FLAVOR_INFO, &request,
		sizeof(request), reply, 16300);
	if (status != B_OK) {
		free(reply);
		return status;
	}

	if (reply->result == B_OK) {
		status = _flavor->Unflatten(reply->type, &reply->flattened_data,
			reply->flattened_size);
	} else
		status = reply->result;

	free(reply);
	return status;
}


status_t
BMediaRoster::GetDormantFlavorInfoFor(const dormant_node_info& dormant,
	dormant_flavor_info* _flavor)
{
	return MediaRosterEx(this)->GetDormantFlavorInfo(dormant.addon,
		dormant.flavor_id, _flavor);
}


// Reports in outLatency the maximum latency found downstream from
// the specified BBufferProducer, producer, given the current connections.
status_t
BMediaRoster::GetLatencyFor(const media_node& producer, bigtime_t* _latency)
{
	CALLED();
	if (_latency == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_NODE(producer)
		|| (producer.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;

	producer_get_latency_request request;
	producer_get_latency_reply reply;
	status_t rv;

	rv = QueryPort(producer.port, PRODUCER_GET_LATENCY, &request,
		sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*_latency = reply.latency;

//	printf("BMediaRoster::GetLatencyFor producer %ld has maximum latency %Ld\n", producer.node, *out_latency);
	return B_OK;
}


status_t
BMediaRoster::GetInitialLatencyFor(const media_node& producer,
	bigtime_t* _latency, uint32* _flags)
{
	CALLED();
	if (_latency == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_NODE(producer)
		|| (producer.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;

	producer_get_initial_latency_request request;
	producer_get_initial_latency_reply reply;
	status_t rv;

	rv = QueryPort(producer.port, PRODUCER_GET_INITIAL_LATENCY, &request,
		sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*_latency = reply.initial_latency;
	if (_flags != NULL)
		*_flags = reply.flags;

	TRACE("BMediaRoster::GetInitialLatencyFor producer %ld has maximum "
		"initial latency %Ld\n", producer.node, *_latency);
	return B_OK;
}


status_t
BMediaRoster::GetStartLatencyFor(const media_node& timeSource,
	bigtime_t* _latency)
{
	CALLED();
	if (_latency == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_NODE(timeSource)
		|| (timeSource.kind & B_TIME_SOURCE) == 0)
		return B_MEDIA_BAD_NODE;

	timesource_get_start_latency_request request;
	timesource_get_start_latency_reply reply;
	status_t rv;

	rv = QueryPort(timeSource.port, TIMESOURCE_GET_START_LATENCY, &request,
		sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*_latency = reply.start_latency;

	TRACE("BMediaRoster::GetStartLatencyFor timesource %ld has maximum "
		"initial latency %Ld\n", timeSource.node, *_latency);
	return B_OK;
}


status_t
BMediaRoster::GetFileFormatsFor(const media_node& fileInterface,
	media_file_format* _formats, int32* _numFormats)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::SetRefFor(const media_node& file_interface, const entry_ref& file,
	bool createAndTruncate, bigtime_t* _length)
{
	CALLED();

	fileinterface_set_ref_request request;
	fileinterface_set_ref_reply reply;
	status_t rv;

	request.device = file.device;
	request.directory = file.directory;
	strcpy(request.name, file.name);
	request.create = createAndTruncate;
	if (_length != NULL)
		request.duration = *_length;

	rv = QueryPort(file_interface.port, FILEINTERFACE_SET_REF, &request,
		sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	if (!createAndTruncate && _length)
		*_length = reply.duration;

	return B_OK;
}


status_t
BMediaRoster::GetRefFor(const media_node& node, entry_ref* _file,
	BMimeType* mimeType)
{
	CALLED();

	if (!_file)
		return B_BAD_VALUE;

	fileinterface_get_ref_request request;
	fileinterface_get_ref_reply reply;
	status_t rv;

	rv = QueryPort(node.port, FILEINTERFACE_GET_REF, &request, sizeof(request),
		&reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*_file = entry_ref(reply.device, reply.directory, reply.name);

	if (mimeType)
		mimeType->SetTo(reply.mimetype);

	return B_OK;
}


status_t
BMediaRoster::SniffRefFor(const media_node& file_interface,
	const entry_ref& file, BMimeType* mimeType, float* _capability)
{
	CALLED();
	if (mimeType == NULL || _capability == NULL)
		return B_BAD_VALUE;

	fileinterface_sniff_ref_request request;
	fileinterface_sniff_ref_reply reply;
	status_t rv;

	request.device = file.device;
	request.directory = file.directory;
	strcpy(request.name, file.name);

	rv = QueryPort(file_interface.port, FILEINTERFACE_SNIFF_REF, &request,
		sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	mimeType->SetTo(reply.mimetype);
	*_capability = reply.capability;

	return B_OK;
}


/*!	This is the generic "here's a file, now can someone please play it"
	interface.
*/
status_t
BMediaRoster::SniffRef(const entry_ref& file, uint64 requireNodeKinds,
	dormant_node_info* _node, BMimeType* mimeType)
{
	CALLED();

	TRACE("BMediaRoster::SniffRef looking for a node to handle %s : %Ld\n",file.name, requireNodeKinds);

	if (_node == NULL)
		return B_BAD_VALUE;

	BMimeType aMimeType;

	dormant_node_info nodes[30];
	int32 count = 30;
	int32 highestCapability = -1;
	float capability;

	media_node node;

	// Get all dormant nodes using GetDormantNodes
	if (GetDormantNodes(nodes, &count, NULL, NULL, NULL, requireNodeKinds | B_FILE_INTERFACE, 0) == B_OK) {
		// Call SniffRefFor on each node that matches requireNodeKinds
		for (int32 i=0;i<count;i++) {
			if (InstantiateDormantNode(nodes[i], &node) == B_OK) {

				if (SniffRefFor(node, file, &aMimeType, &capability) == B_OK) {
					// find the first node that has 100% capability
					TRACE("%s has a %f%% chance of playing file\n",nodes[i].name, capability * 100.0);
					if (capability == 1.0) {
						highestCapability = i;
						break;
					}
				}
				ReleaseNode(node);
			}
		}

		if (highestCapability != -1) {
			*_node = nodes[highestCapability];

			TRACE("BMediaRoster::SniffRef: found a node %s addon-id %ld, flavor_id %ld\n",
			nodes[highestCapability].name, nodes[highestCapability].addon, nodes[highestCapability].flavor_id);

			if (mimeType != NULL) {
				//*mimeType = aMimeType; -- need a copy constructor
			}

			return B_OK;
		}

	}

	return B_ERROR;
}


status_t
BMediaRoster::GetDormantNodeForType(const BMimeType& type,
	uint64 requireNodeKinds, dormant_node_info* _node)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::GetReadFileFormatsFor(const dormant_node_info& node,
	media_file_format* _readFormats, int32 readCount, int32* _readCount)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::GetWriteFileFormatsFor(const dormant_node_info& node,
	media_file_format* _write_formats, int32 writeCount, int32* _writeCount)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::GetFormatFor(const media_output& output, media_format* _format,
	uint32 flags)
{
	CALLED();
	if (_format == NULL)
		return B_BAD_VALUE;
	if ((output.node.kind & B_BUFFER_PRODUCER) == 0)
		return B_MEDIA_BAD_NODE;
	if (IS_INVALID_SOURCE(output.source))
		return B_MEDIA_BAD_SOURCE;

	producer_format_suggestion_requested_request request;
	producer_format_suggestion_requested_reply reply;
	status_t rv;

	request.type = B_MEDIA_UNKNOWN_TYPE;
	request.quality = 0; // TODO: what should this be?

	rv = QueryPort(output.source.port, PRODUCER_FORMAT_SUGGESTION_REQUESTED,
		&request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*_format = reply.format;
	return B_OK;
}


status_t
BMediaRoster::GetFormatFor(const media_input& input, media_format* _format,
	uint32 flags)
{
	CALLED();
	if (_format == NULL)
		return B_BAD_VALUE;
	if ((input.node.kind & B_BUFFER_CONSUMER) == 0)
		return B_MEDIA_BAD_NODE;
	if (IS_INVALID_DESTINATION(input.destination))
		return B_MEDIA_BAD_DESTINATION;

	consumer_accept_format_request request;
	consumer_accept_format_reply reply;
	status_t rv;

	request.dest = input.destination;
	memset(&request.format, 0, sizeof(request.format)); // wildcard

	rv = QueryPort(input.destination.port, CONSUMER_ACCEPT_FORMAT, &request,
		sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*_format = reply.format;
	return B_OK;
}


status_t
BMediaRoster::GetFormatFor(const media_node& node, media_format* _format,
	float quality)
{
	UNIMPLEMENTED();
	if (_format == NULL)
		return B_BAD_VALUE;
	if (IS_INVALID_NODE(node))
		return B_MEDIA_BAD_NODE;
	if ((node.kind & (B_BUFFER_CONSUMER | B_BUFFER_PRODUCER)) == 0)
		return B_MEDIA_BAD_NODE;

	return B_ERROR;
}


ssize_t
BMediaRoster::GetNodeAttributesFor(const media_node& node,
	media_node_attribute* _array, size_t maxCount)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


media_node_id
BMediaRoster::NodeIDFor(port_id port)
{
	CALLED();

	server_node_id_for_request request;
	server_node_id_for_reply reply;
	status_t rv;

	request.port = port;

	rv = QueryServer(SERVER_NODE_ID_FOR, &request, sizeof(request), &reply,
		sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaRoster::NodeIDFor: failed (error %#lx)\n", rv);
		return -1;
	}

	return reply.node_id;
}


status_t
BMediaRoster::GetInstancesFor(media_addon_id addon, int32 flavor,
	media_node_id* _id, int32* _count)
{
	CALLED();
	if (_id == NULL)
		return B_BAD_VALUE;
	if (_count && *_count <= 0)
		return B_BAD_VALUE;

	server_get_instances_for_request request;
	server_get_instances_for_reply reply;
	status_t rv;

	request.max_count = (_count ? *_count : 1);
	request.add_on_id = addon;
	request.flavor_id = flavor;

	rv = QueryServer(SERVER_GET_INSTANCES_FOR, &request, sizeof(request),
		&reply, sizeof(reply));
	if (rv != B_OK) {
		ERROR("BMediaRoster::GetLiveNodes failed\n");
		return rv;
	}

	if (_count)
		*_count = reply.count;
	if (reply.count > 0)
		memcpy(_id, reply.node_id, sizeof(media_node_id) * reply.count);

	return B_OK;
}


status_t
BMediaRoster::SetRealtimeFlags(uint32 enabled)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::GetRealtimeFlags(uint32* _enabled)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


ssize_t
BMediaRoster::AudioBufferSizeFor(int32 channelCount, uint32 sampleFormat,
	float frameRate, bus_type busKind)
{
	bigtime_t bufferDuration;
	ssize_t bufferSize;

	system_info info;
	get_system_info(&info);

	if (info.cpu_clock_speed > 2000000000)	// 2 GHz
		bufferDuration = 2500;
	else if (info.cpu_clock_speed > 1000000000)
		bufferDuration = 5000;
	else if (info.cpu_clock_speed > 600000000)
		bufferDuration = 10000;
	else if (info.cpu_clock_speed > 200000000)
		bufferDuration = 20000;
	else if (info.cpu_clock_speed > 100000000)
		bufferDuration = 30000;
	else
		bufferDuration = 50000;

	if ((busKind == B_ISA_BUS || busKind == B_PCMCIA_BUS)
		&& bufferDuration < 25000)
		bufferDuration = 25000;

	bufferSize = (sampleFormat & 0xf) * channelCount
		* (ssize_t)((frameRate * bufferDuration) / 1000000.0);

	printf("Suggested buffer duration %Ld, size %ld\n", bufferDuration,
		bufferSize);

	return bufferSize;
}


/*!	Use MediaFlags to inquire about specific features of the Media Kit.
	Returns < 0 for "not present", positive size for output data size.
	0 means that the capability is present, but no data about it.
*/
/*static*/ ssize_t
BMediaRoster::MediaFlags(media_flags cap, void* buffer, size_t maxSize)
{
	UNIMPLEMENTED();
	return 0;
}


//	#pragma mark - BLooper overrides


void
BMediaRoster::MessageReceived(BMessage* message)
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
			// TODO: If a node is released using BMediaRoster::ReleaseNode()
			// TODO: instead of using BMediaNode::Release() / BMediaNode::Acquire()
			// TODO: fRefCount of the BMediaNode will not be correct.

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


bool
BMediaRoster::QuitRequested()
{
	UNIMPLEMENTED();
	return true;
}


BHandler*
BMediaRoster::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return BLooper::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BMediaRoster::GetSupportedSuites(BMessage* data)
{
	return BLooper::GetSupportedSuites(data);
}


BMediaRoster::~BMediaRoster()
{
	CALLED();

	delete gTimeSourceObjectManager;
	delete gDormantNodeManager;

	// unregister this application with the media server
	server_unregister_app_request request;
	server_unregister_app_reply reply;
	request.team = BPrivate::current_team();
	QueryServer(SERVER_UNREGISTER_APP, &request, sizeof(request), &reply,
		sizeof(reply));

	BPrivate::SharedBufferList::Invalidate();

	// Unset the global instance pointer, the destructor is also called
	// if a client app calls Lock(); and Quit(); directly.
	sDefaultInstance = NULL;
}


//	#pragma mark - private BMediaRoster


//! Deprecated call.
status_t
BMediaRoster::SetOutputBuffersFor(const media_source& output,
	BBufferGroup* group, bool willReclaim)
{
	UNIMPLEMENTED();
	debugger("BMediaRoster::SetOutputBuffersFor missing\n");
	return B_ERROR;
}


// FBC reserved virtuals
status_t BMediaRoster::_Reserved_MediaRoster_0(void*) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_1(void*) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_2(void*) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_3(void*) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_4(void*) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_5(void*) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_6(void*) { return B_ERROR; }
status_t BMediaRoster::_Reserved_MediaRoster_7(void*) { return B_ERROR; }


BMediaRoster::BMediaRoster()
	:
	BLooper("_BMediaRoster_", B_URGENT_DISPLAY_PRIORITY,
		B_LOOPER_PORT_DEFAULT_CAPACITY)
{
	CALLED();

	// start the looper
	Run();
}


// TODO: Looks like these can be safely removed:
/*static*/ status_t
BMediaRoster::ParseCommand(BMessage& reply)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::GetDefaultInfo(media_node_id forDefault, BMessage& config)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BMediaRoster::SetRunningDefault(media_node_id forDefault,
	const media_node& node)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


// #pragma mark - static variables


BMediaRoster* BMediaRoster::sDefaultInstance = NULL;

