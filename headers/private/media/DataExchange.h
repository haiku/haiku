/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _DATA_EXCHANGE_H
#define _DATA_EXCHANGE_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaAddOn.h>
#include <Entry.h>

namespace BPrivate {
namespace media {
namespace dataexchange {

struct reply_data;
struct request_data;

// BMessage based data exchange with the media_server
status_t SendToServer(BMessage *msg);
status_t QueryServer(BMessage *request, BMessage *reply);

// Raw data based data exchange with the media_server
status_t SendToServer(int32 msgcode, void *msg, int size);
status_t QueryServer(int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize);

// Raw data based data exchange with the media_addon_server
status_t SendToAddonServer(int32 msgcode, void *msg, int size);
status_t QueryAddonServer(int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize);

// Raw data based data exchange with the media_server
status_t SendToPort(port_id sendport, int32 msgcode, void *msg, int size);
status_t QueryPort(port_id requestport, int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize);

// The base struct used for all raw requests
struct request_data
{
	port_id reply_port;

	void SendReply(status_t result, reply_data *reply, int replysize) const;
};

// The base struct used for all raw replys
struct reply_data
{
	status_t result;
};


}; // dataexchange
}; // media
}; // BPrivate

using namespace BPrivate::media::dataexchange;

// BMessage based server communication
enum {

	// BMediaRoster notification service
	MEDIA_SERVER_REQUEST_NOTIFICATIONS = 1000,
	MEDIA_SERVER_CANCEL_NOTIFICATIONS,
	MEDIA_SERVER_SEND_NOTIFICATIONS

};

// Raw port based communication
enum {
	SERVER_GET_NODE = 0x1000,
	SERVER_SET_NODE,
	SERVER_PUBLISH_INPUTS,
	SERVER_PUBLISH_OUTPUTS,
	SERVER_NODE_ID_FOR,
	SERVER_GET_LIVE_NODE_INFO,
	SERVER_GET_LIVE_NODES,
	SERVER_GET_NODE_FOR,
	SERVER_RELEASE_NODE,
	SERVER_REGISTER_NODE,
	SERVER_UNREGISTER_NODE,
	CONSUMER_GET_NEXT_INPUT = 0x2000,
	CONSUMER_DISPOSE_INPUT_COOKIE,
	CONSUMER_ACCEPT_FORMAT,
	CONSUMER_CONNECTED,
	CONSUMER_DISCONNECTED,
	PRODUCER_GET_NEXT_OUTPUT = 0x3000,
	PRODUCER_DISPOSE_OUTPUT_COOKIE,
	PRODUCER_FORMAT_PROPOSAL,
	PRODUCER_PREPARE_TO_CONNECT,
	PRODUCER_CONNECT,
	PRODUCER_DISCONNECT,
};

// used by SERVER_GET_NODE and SERVER_SET_NODE
enum node_type 
{ 
	VIDEO_INPUT, 
	AUDIO_INPUT, 
	VIDEO_OUTPUT, 
	AUDIO_MIXER, 
	AUDIO_OUTPUT, 
	AUDIO_OUTPUT_EX, 
	TIME_SOURCE, 
	SYSTEM_TIME_SOURCE 
};

// used by SERVER_PUBLISH_INPUTS and SERVER_PUBLISH_OUTPUTS
enum
{
	MAX_OUTPUTS = 48,
	MAX_INPUTS = 48,
};

// used by SERVER_GET_LIVE_NODES
enum
{
	MAX_LIVE_INFO = 62,
};

struct addonserver_instantiate_dormant_node_request : public request_data
{
	dormant_node_info info;
};

struct addonserver_instantiate_dormant_node_reply : public reply_data
{
	media_node node;
};

struct server_set_node_request : public request_data
{
	node_type type;
	bool use_node;
	media_node node;
	bool use_dni;
	dormant_node_info dni;
	bool use_input;
	media_input input;
};

struct server_set_node_reply : public reply_data
{
};

struct server_get_node_request : public request_data
{
	node_type type;
	team_id team;
};

struct server_get_node_reply : public reply_data
{
	media_node node;

	// for AUDIO_OUTPUT_EX
	char input_name[B_MEDIA_NAME_LENGTH];
	int32 input_id;
};

struct producer_format_proposal_request : public request_data
{
	media_source output;
	media_format format;
};

struct producer_format_proposal_reply : public reply_data
{
	media_format format;
};

struct producer_prepare_to_connect_request : public request_data
{
	media_source source;
	media_destination destination;
	media_format format;
	char name[B_MEDIA_NAME_LENGTH];
};

struct producer_prepare_to_connect_reply : public reply_data
{
	media_format format;
	media_source out_source;
	char name[B_MEDIA_NAME_LENGTH];
};

struct producer_connect_request : public request_data
{
	status_t error;
	media_source source;
	media_destination destination;
	media_format format;
	char name[B_MEDIA_NAME_LENGTH];
};

struct producer_connect_reply : public reply_data
{
	char name[B_MEDIA_NAME_LENGTH];
};

struct consumer_accept_format_request : public request_data
{
	media_destination dest;
	media_format format;
};

struct consumer_accept_format_reply : public reply_data
{
	media_format format;
};

struct consumer_connected_request : public request_data
{
	media_source producer;
	media_destination where;
	media_format with_format;
};

struct consumer_connected_reply : public reply_data
{
	media_input input;
};

struct server_publish_inputs_request : public request_data
{
	media_node node;
	int32 count;
	area_id area;	// if count > MAX_INPUTS, inputs are in the area
					// area is created in the server, and also deleted
					// in the server after the reply has been received
	media_input inputs[MAX_INPUTS];
};

struct server_publish_inputs_reply : public reply_data
{
};

struct server_publish_outputs_request : public request_data
{
	media_node node;
	int32 count;
	area_id area; // if count > MAX_OUTPUTS, outputs are in the area
	media_output outputs[MAX_OUTPUTS];
};

struct server_publish_outputs_reply : public reply_data
{
};

struct producer_get_next_output_request : public request_data
{
	int32 cookie;
};

struct producer_get_next_output_reply : public reply_data
{
	int32 cookie;
	media_output output;
};

struct producer_dispose_output_cookie_request : public request_data
{
	int32 cookie;
};

struct producer_dispose_output_cookie_reply : public reply_data
{
};

struct consumer_get_next_input_request : public request_data
{
	int32 cookie;
};

struct consumer_get_next_input_reply : public reply_data
{
	int32 cookie;
	media_input input;
};

struct consumer_dispose_input_cookie_request : public request_data
{
	int32 cookie;
};

struct consumer_dispose_input_cookie_reply : public reply_data
{
};

struct consumer_disconnected_request : public request_data
{
	media_source source;
	media_destination destination;
};

struct consumer_disconnected_reply : public reply_data
{
};

struct producer_disconnect_request : public request_data
{
	media_source source;
	media_destination destination;
};

struct producer_disconnect_reply : public reply_data
{
};

struct server_register_node_request : public request_data
{
	media_addon_id addon_id;
	int32 addon_flavor_id;
	char name[B_MEDIA_NAME_LENGTH];
	uint64 kinds;
	port_id port;
	team_id team;
};

struct server_register_node_reply : public reply_data
{
	media_node_id nodeid;
};

struct server_unregister_node_request : public request_data
{
	media_node_id nodeid;
	team_id team;
};

struct server_unregister_node_reply : public reply_data
{
	media_addon_id addon_id;
};

struct server_get_live_node_info_request : public request_data
{
	media_node node;
};

struct server_get_live_node_info_reply : public reply_data
{
	live_node_info live_info;
};

struct server_get_live_nodes_request : public request_data
{
	int32 maxcount;
	bool has_input;
	bool has_output;
	bool has_name;
	media_format inputformat;
	media_format outputformat;
	char name[B_MEDIA_NAME_LENGTH + 1]; // 1 for a trailing "*"
	uint64 require_kinds;
};

struct server_get_live_nodes_reply : public reply_data
{
	int32 count;
	area_id area; 	// if count > MAX_LIVE_INFO, live_node_infos are in the area
					// area is created in the server, but deleted in the application
	live_node_info live_info[MAX_LIVE_INFO]; 
};

struct server_node_id_for_request : public request_data
{
	port_id port;
};

struct server_node_id_for_reply : public reply_data
{
	media_node_id nodeid;
};

struct server_get_node_for_request : public request_data
{
	media_node_id nodeid;
	team_id team;
};

struct server_get_node_for_reply : public reply_data
{
	media_node clone;
};

struct server_release_node_request : public request_data
{
	media_node node;
	team_id team;
};

struct server_release_node_reply : public reply_data
{
};


#endif // _DATA_EXCHANGE_H
