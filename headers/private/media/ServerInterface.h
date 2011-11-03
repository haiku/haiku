/*
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SERVER_INTERFACE_H_
#define _SERVER_INTERFACE_H_


#include <string.h>

#include <Buffer.h>
#include <Entry.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaNode.h>
#include <Messenger.h>


// BMessage based server communication
enum {
	// BMediaRoster notification service
	MEDIA_SERVER_REQUEST_NOTIFICATIONS = 1000,
	MEDIA_SERVER_CANCEL_NOTIFICATIONS,
	MEDIA_SERVER_SEND_NOTIFICATIONS,

	MEDIA_SERVER_GET_FORMATS,
	MEDIA_SERVER_MAKE_FORMAT_FOR,

	// add_system_beep_event()
	MEDIA_SERVER_ADD_SYSTEM_BEEP_EVENT,

	// media add-on server
	MEDIA_ADD_ON_SERVER_PLAY_MEDIA = '_TRU'
};

// Raw port based communication
enum {
	GENERAL_PURPOSE_WAKEUP = 0,	// when no action but wait termination needed
	
	ADD_ON_SERVER_RESCAN_ADD_ON_FLAVORS = 0x50,
	ADD_ON_SERVER_RESCAN_FINISHED_NOTIFY,
	ADD_ON_SERVER_INSTANTIATE_DORMANT_NODE,

	SERVER_MESSAGE_START = 0x100,
	SERVER_REGISTER_APP,
	SERVER_UNREGISTER_APP,
	SERVER_GET_NODE,
	SERVER_SET_NODE,
	SERVER_PUBLISH_INPUTS,
	SERVER_PUBLISH_OUTPUTS,
	SERVER_NODE_ID_FOR,
	SERVER_GET_LIVE_NODE_INFO,
	SERVER_GET_LIVE_NODES,
	SERVER_GET_NODE_FOR,
	SERVER_RELEASE_NODE,
	SERVER_RELEASE_NODE_ALL,
	SERVER_REGISTER_NODE,
	SERVER_UNREGISTER_NODE,
	SERVER_GET_DORMANT_NODE_FOR,
	SERVER_GET_INSTANCES_FOR,
	SERVER_GET_SHARED_BUFFER_AREA,
	SERVER_REGISTER_BUFFER,
	SERVER_UNREGISTER_BUFFER,
	SERVER_RESCAN_DEFAULTS,
	SERVER_SET_NODE_CREATOR,
	SERVER_CHANGE_FLAVOR_INSTANCES_COUNT,
	SERVER_GET_MEDIA_FILE_TYPES,
	SERVER_GET_MEDIA_FILE_ITEMS,
	SERVER_GET_REF_FOR,
	SERVER_SET_REF_FOR,
	SERVER_INVALIDATE_MEDIA_ITEM,
	SERVER_REMOVE_MEDIA_ITEM,
	SERVER_GET_ITEM_AUDIO_GAIN,
	SERVER_SET_ITEM_AUDIO_GAIN,
	SERVER_GET_FORMAT_FOR_DESCRIPTION,
	SERVER_GET_DESCRIPTION_FOR_FORMAT,
	SERVER_GET_READERS,
	SERVER_GET_DECODER_FOR_FORMAT,
	SERVER_GET_WRITER_FOR_FORMAT_FAMILY,
	SERVER_GET_FILE_FORMAT_FOR_COOKIE,
	SERVER_GET_CODEC_INFO_FOR_COOKIE,
	SERVER_GET_ENCODER_FOR_CODEC_INFO,
	SERVER_REGISTER_ADD_ON,
	SERVER_UNREGISTER_ADD_ON,
	SERVER_GET_ADD_ON_REF,
	SERVER_REGISTER_DORMANT_NODE,
	SERVER_GET_DORMANT_NODES,
	SERVER_GET_DORMANT_FLAVOR_INFO,
	SERVER_MESSAGE_END,

	NODE_MESSAGE_START = 0x200,
	NODE_START,
	NODE_STOP,
	NODE_SEEK,
	NODE_SET_RUN_MODE,
	NODE_TIME_WARP,
	NODE_PREROLL,
	NODE_SET_TIMESOURCE,
	NODE_GET_TIMESOURCE,
	NODE_REQUEST_COMPLETED,
	NODE_FINAL_RELEASE,
	NODE_MESSAGE_END,

	CONSUMER_MESSAGE_START = 0x300,
	CONSUMER_GET_NEXT_INPUT,
	CONSUMER_DISPOSE_INPUT_COOKIE,
	CONSUMER_ACCEPT_FORMAT,
	CONSUMER_CONNECTED,
	CONSUMER_DISCONNECTED,
	CONSUMER_BUFFER_RECEIVED,
	CONSUMER_PRODUCER_DATA_STATUS,
	CONSUMER_GET_LATENCY_FOR,
	CONSUMER_FORMAT_CHANGED,
	CONSUMER_SEEK_TAG_REQUESTED,
	CONSUMER_MESSAGE_END,

	PRODUCER_MESSAGE_START = 0x400,
	PRODUCER_GET_NEXT_OUTPUT,
	PRODUCER_DISPOSE_OUTPUT_COOKIE,
	PRODUCER_FORMAT_PROPOSAL,
	PRODUCER_PREPARE_TO_CONNECT,
	PRODUCER_CONNECT,
	PRODUCER_DISCONNECT,
	PRODUCER_LATE_NOTICE_RECEIVED,
	PRODUCER_LATENCY_CHANGED,
	PRODUCER_ADDITIONAL_BUFFER_REQUESTED,
	PRODUCER_VIDEO_CLIPPING_CHANGED,
	PRODUCER_FORMAT_CHANGE_REQUESTED,
	PRODUCER_SET_BUFFER_GROUP,
	PRODUCER_GET_LATENCY,
	PRODUCER_GET_INITIAL_LATENCY,
	PRODUCER_FORMAT_SUGGESTION_REQUESTED,
	PRODUCER_SET_PLAY_RATE,
	PRODUCER_ENABLE_OUTPUT,
	PRODUCER_SET_RUN_MODE_DELAY,
	PRODUCER_MESSAGE_END,

	FILEINTERFACE_MESSAGE_START = 0x500,
	FILEINTERFACE_SET_REF,
	FILEINTERFACE_GET_REF,
	FILEINTERFACE_SNIFF_REF,
	FILEINTERFACE_MESSAGE_END,

	CONTROLLABLE_MESSAGE_START = 0x600,
	CONTROLLABLE_GET_PARAMETER_WEB,
	CONTROLLABLE_GET_PARAMETER_DATA,
	CONTROLLABLE_SET_PARAMETER_DATA,
	CONTROLLABLE_START_CONTROL_PANEL,
	CONTROLLABLE_MESSAGE_END,

	TIMESOURCE_MESSAGE_START = 0x700,
	TIMESOURCE_OP, // datablock is a struct time_source_op_info
	TIMESOURCE_ADD_SLAVE_NODE,
	TIMESOURCE_REMOVE_SLAVE_NODE,
	TIMESOURCE_GET_START_LATENCY,
	TIMESOURCE_MESSAGE_END,
};


namespace BPrivate {
namespace media {


struct reply_data;
struct request_data;
struct command_data;


// The base struct used for all raw requests
struct request_data {
	port_id					reply_port;

	request_data();
	~request_data();

	status_t SendReply(status_t result, reply_data* reply,
		size_t replySize) const;
};

// The base struct used for all raw replys
struct reply_data {
	status_t				result;
};

// The base struct used for all raw commands (asynchronous, no answer)
struct command_data {
	// yes, it's empty ;)

#if __GNUC__ == 4
	int32 _padding;
		// GCC 2 and GCC 4 treat empty structures differently
#endif
};

// The base struct used for all requests using an area
struct area_request_data : request_data {
	area_id					area;
};

// The base struct used for all requests requesting an area
struct request_area_data : request_data {
	team_id					team;
};

// The base struct used for all replies using an area
struct area_reply_data : reply_data {
	area_id					area;
	void*					address;
};


/* We can't send an entry_ref through a port to another team,
 * but we can assign it to an xfer_entry_ref and send this one,
 * when we receive it we can assign it to a normal entry_ref
 */
struct xfer_entry_ref {
public:
	xfer_entry_ref()
	{
		device = -1;
		directory = -1;
		name[0] = 0;
	}

	operator entry_ref() const
	{
		entry_ref ref(device, directory, name);
		return ref;
	}

	xfer_entry_ref& operator=(const entry_ref& ref)
	{
		device = ref.device;
		directory = ref.directory;
		if (ref.name)
			strcpy(name, ref.name);
		else
			name[0] = 0;

		return *this;
	}

private:
	dev_t	device;
	ino_t	directory;
	char	name[B_FILE_NAME_LENGTH];
};


}	// namespace media
}	// namespace BPrivate

using namespace BPrivate::media;

// used by SERVER_GET_NODE and SERVER_SET_NODE
enum node_type {
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
enum {
	MAX_OUTPUTS = 8,
	MAX_INPUTS = 8,
};

// used by SERVER_GET_LIVE_NODES
enum {
	MAX_LIVE_INFO = 16,
};

// used by SERVER_GET_INSTANCES_FOR
enum {
	MAX_NODE_ID = 4000,
};

// used by SERVER_GET_READERS
enum {
	MAX_READERS = 40,
};


// #pragma mark - media add-on server commands


struct add_on_server_instantiate_dormant_node_request : request_data {
	media_addon_id			add_on_id;
	int32					flavor_id;
	team_id					creator_team;
};

struct add_on_server_instantiate_dormant_node_reply : reply_data {
	media_node				node;
};

struct add_on_server_rescan_flavors_command : command_data {
	media_addon_id			add_on_id;
};

struct add_on_server_rescan_finished_notify_command : command_data {
};


// #pragma mark - media server commands


struct server_set_node_request : request_data {
	node_type				type;
	bool					use_node;
	media_node				node;
	bool					use_dni;
	dormant_node_info		dni;
	bool					use_input;
	media_input				input;
};

struct server_set_node_reply : reply_data {
};

struct server_get_node_request : request_data {
	node_type				type;
	team_id					team;
};

struct server_get_node_reply : public reply_data {
	media_node				node;

	// for AUDIO_OUTPUT_EX
	char					input_name[B_MEDIA_NAME_LENGTH];
	int32					input_id;
};

struct server_publish_inputs_request : request_data {
	media_node				node;
	int32					count;
	area_id area;	// if count > MAX_INPUTS, inputs are in the area
					// area is created in the library, and also deleted
					// in the library after the reply has been received
	media_input inputs[MAX_INPUTS];
};

struct server_publish_inputs_reply : reply_data {
};

struct server_publish_outputs_request : area_request_data {
	media_node				node;
	int32					count;
		// if count > MAX_OUTPUTS, outputs are in the area
		// area is created in the library, and also deleted
		// in the library after the reply has been received
	media_output outputs[MAX_OUTPUTS];
};

struct server_publish_outputs_reply : reply_data {
};

struct server_register_app_request : request_data {
	team_id					team;
	BMessenger				messenger;
};

struct server_register_app_reply : reply_data {
};

struct server_unregister_app_request : request_data {
	team_id					team;
};

struct server_unregister_app_reply : reply_data {
};

struct server_set_node_creator_request : request_data {
	media_node_id			node;
	team_id					creator;
};

struct server_set_node_creator_reply : reply_data {
};

struct server_change_flavor_instances_count_request : request_data {
	media_addon_id			add_on_id;
	int32					flavor_id;
	int32					delta; // must be +1 or -1
	team_id					team;
};

struct server_change_flavor_instances_count_reply : reply_data {
};

struct server_register_node_request : request_data {
	media_addon_id			add_on_id;
	int32					flavor_id;
	char					name[B_MEDIA_NAME_LENGTH];
	uint64					kinds;
	port_id					port;
	team_id					team;
};

struct server_register_node_reply : reply_data {
	media_node_id			node_id;
};

struct server_unregister_node_request : request_data {
	media_node_id			node_id;
	team_id					team;
};

struct server_unregister_node_reply : reply_data {
	media_addon_id			add_on_id;
	int32					flavor_id;
};

struct server_get_live_node_info_request : request_data {
	media_node				node;
};

struct server_get_live_node_info_reply : reply_data {
	live_node_info			live_info;
};

struct server_get_live_nodes_request : request_area_data {
	int32					max_count;
	bool					has_input;
	bool					has_output;
	bool					has_name;
	media_format			input_format;
	media_format			output_format;
	char					name[B_MEDIA_NAME_LENGTH + 1];
								// +1 for a trailing "*"
	uint64					require_kinds;
};

struct server_get_live_nodes_reply : area_reply_data {
	int32					count;
		// if count > MAX_LIVE_INFO, live_node_infos are in the area
		// area is created in the server, but deleted in the library
	live_node_info			live_info[MAX_LIVE_INFO];
};

struct server_node_id_for_request : request_data {
	port_id					port;
};

struct server_node_id_for_reply : reply_data {
	media_node_id			node_id;
};

struct server_get_node_for_request : request_data {
	media_node_id			node_id;
	team_id					team;
};

struct server_get_node_for_reply : reply_data {
	media_node				clone;
};

struct server_release_node_request : request_data {
	media_node				node;
	team_id					team;
};

struct server_release_node_reply : reply_data {
};

struct server_get_dormant_node_for_request : request_data {
	media_node				node;
};

struct server_get_dormant_node_for_reply : reply_data {
	dormant_node_info		node_info;
};

struct server_get_instances_for_request : request_data {
	media_addon_id			add_on_id;
	int32					flavor_id;
	int32					max_count;
};

struct server_get_instances_for_reply : reply_data {
	int32					count;
	media_node_id			node_id[MAX_NODE_ID];
		// no area here, MAX_NODE_ID is really large
};

struct server_rescan_defaults_command : command_data {
};

struct server_register_add_on_request : request_data {
	xfer_entry_ref			ref;
};

struct server_register_add_on_reply : reply_data {
	media_addon_id			add_on_id;
};

struct server_unregister_add_on_command : command_data {
	media_addon_id			add_on_id;
};

struct server_get_add_on_ref_request : request_data {
	media_addon_id			add_on_id;
};

struct server_get_add_on_ref_reply : reply_data {
	xfer_entry_ref			ref;
};

struct server_get_shared_buffer_area_request : request_data {
};

struct server_get_shared_buffer_area_reply : area_reply_data {
};

struct server_register_buffer_request : request_data {
	team_id					team;
	buffer_clone_info		info;
		// either info.buffer is != 0, or the area, size, offset is used
};

struct server_register_buffer_reply : reply_data {
	buffer_clone_info		info;
};

struct server_unregister_buffer_command : command_data {
	team_id					team;
	media_buffer_id			buffer_id;
};

struct server_get_media_types_request : request_area_data {
};

struct server_get_media_types_reply : area_reply_data {
	int32					count;
};

struct server_get_media_items_request : request_area_data {
	char					type[B_MEDIA_NAME_LENGTH];
};

struct server_get_media_items_reply : area_reply_data {
	int32					count;
};

struct server_get_ref_for_request : request_data {
	char					type[B_MEDIA_NAME_LENGTH];
	char					item[B_MEDIA_NAME_LENGTH];
};

struct server_get_ref_for_reply : reply_data {
	xfer_entry_ref			ref;
};

struct server_set_ref_for_request : request_data {
	char					type[B_MEDIA_NAME_LENGTH];
	char					item[B_MEDIA_NAME_LENGTH];
	xfer_entry_ref			ref;
};

struct server_set_ref_for_reply : reply_data {
};

struct server_invalidate_item_request : request_data {
	char					type[B_MEDIA_NAME_LENGTH];
	char					item[B_MEDIA_NAME_LENGTH];
};

struct server_invalidate_item_reply : reply_data {
};

struct server_remove_media_item_request : request_data {
	char					type[B_MEDIA_NAME_LENGTH];
	char					item[B_MEDIA_NAME_LENGTH];
};

struct server_remove_media_item_reply : reply_data {
};

struct server_get_item_audio_gain_request : request_data {
	char					type[B_MEDIA_NAME_LENGTH];
	char					item[B_MEDIA_NAME_LENGTH];
};

struct server_get_item_audio_gain_reply : reply_data {
	float					gain;
};

struct server_set_item_audio_gain_request : request_data {
	char					type[B_MEDIA_NAME_LENGTH];
	char					item[B_MEDIA_NAME_LENGTH];
	float					gain;
};

struct server_set_item_audio_gain_reply : reply_data {
};

struct server_get_decoder_for_format_request : request_data {
	media_format			format;
};

struct server_get_decoder_for_format_reply : reply_data {
	xfer_entry_ref			ref;
		// a ref to the decoder
};

struct server_get_encoder_for_codec_info_request : request_data {
	int32					id;
};

struct server_get_encoder_for_codec_info_reply : reply_data {
	xfer_entry_ref			ref;
		// a ref to the encoder
};

struct server_get_readers_request : request_data {
};

struct server_get_readers_reply : reply_data {
	xfer_entry_ref			ref[MAX_READERS];
		// a list of refs to the reader
	int32					count;
};

struct server_get_writer_request : request_data {
	uint32					internal_id;
};

struct server_get_writer_reply : reply_data {
	xfer_entry_ref			ref;
		// a ref to the writer
};

struct server_get_file_format_request : request_data {
	int32					cookie;
};

struct server_get_file_format_reply : reply_data {
	media_file_format		file_format;
		// the file format matching the cookie
};

struct server_get_codec_info_request : request_data {
	int32					cookie;
};

struct server_get_codec_info_reply : reply_data {
	media_codec_info		codec_info;
	media_format_family		format_family;
	media_format			input_format;
	media_format			output_format;
		// the codec info matching the cookie
};

struct server_get_dormant_flavor_info_request : request_data {
	media_addon_id	add_on_id;
	int32			flavor_id;
};

struct server_get_dormant_flavor_info_reply : reply_data {
	type_code		type; // the flatten type_code
	size_t 			flattened_size;
	char 			flattened_data[1];
		// a flattened dormant_flavor_info, flattened_size large
};

struct server_get_dormant_nodes_request : request_data {
	int32			max_count;
	bool			has_input;
	media_format	input_format;
	bool			has_output;
	media_format	output_format;
	bool			has_name;
	char			name[B_MEDIA_NAME_LENGTH + 1]; // 1 for a trailing "*"
	uint64			require_kinds;
	uint64			deny_kinds;
};

struct server_get_dormant_nodes_reply : reply_data {
	int32			count;
		// if count > 0, a second reply containing count dormant_node_infos
		// is send
};

struct server_register_dormant_node_command : command_data {
	media_addon_id	purge_id;
		// if > 0, server must first remove all dormant_flavor_infos
		// belonging to that id
	type_code		type; // the flatten type_code
	size_t 			flattened_size;
	char 			flattened_data[1];
		// a flattened dormant_flavor_info, flattened_size large
};


// #pragma mark - buffer producer commands


struct producer_format_proposal_request : public request_data {
	media_source			output;
	media_format			format;
};

struct producer_format_proposal_reply : reply_data {
	media_format			format;
};

struct producer_prepare_to_connect_request : request_data {
	media_source			source;
	media_destination		destination;
	media_format			format;
	char					name[B_MEDIA_NAME_LENGTH];
};

struct producer_prepare_to_connect_reply : reply_data {
	media_format			format;
	media_source			out_source;
	char					name[B_MEDIA_NAME_LENGTH];
};

struct producer_connect_request : request_data {
	status_t				error;
	media_source			source;
	media_destination		destination;
	media_format			format;
	char					name[B_MEDIA_NAME_LENGTH];
};

struct producer_connect_reply : reply_data {
	char					name[B_MEDIA_NAME_LENGTH];
};

struct producer_disconnect_request : request_data {
	media_source			source;
	media_destination		destination;
};

struct producer_disconnect_reply : reply_data {
};

struct producer_format_suggestion_requested_request : request_data {
	media_type				type;
	int32					quality;
};

struct producer_format_suggestion_requested_reply : reply_data {
	media_format			format;
};

struct producer_set_play_rate_request : request_data {
	int32					numer;
	int32					denom;
};

struct producer_set_play_rate_reply : reply_data {
};

struct producer_get_initial_latency_request : request_data {
};

struct producer_get_initial_latency_reply : reply_data {
	bigtime_t				initial_latency;
	uint32					flags;
};

struct producer_get_latency_request : request_data {
};

struct producer_get_latency_reply : reply_data {
	bigtime_t				latency;
};

struct producer_set_buffer_group_command : command_data {
	media_source			source;
	media_destination		destination;
	void*					user_data;
	int32					change_tag;
	int32					buffer_count;
	media_buffer_id			buffers[1];
};

struct producer_format_change_requested_command : command_data {
	media_source			source;
	media_destination		destination;
	media_format			format;
	void*					user_data;
	int32					change_tag;
};

struct producer_video_clipping_changed_command : command_data {
	media_source			source;
	media_destination		destination;
	media_video_display_info display;
	void*					user_data;
	int32					change_tag;
	int32					short_count;
	int16					shorts[1];
};

struct producer_additional_buffer_requested_command : command_data {
	media_source			source;
	media_buffer_id			prev_buffer;
	bigtime_t				prev_time;
	bool					has_seek_tag;
	media_seek_tag			prev_tag;
};

struct producer_latency_changed_command : command_data {
	media_source			source;
	media_destination		destination;
	bigtime_t				latency;
	uint32					flags;
};

struct producer_enable_output_command : command_data {
	media_source			source;
	media_destination		destination;
	bool					enabled;
	void*					user_data;
	int32					change_tag;
};

struct producer_late_notice_received_command : command_data {
	media_source			source;
	bigtime_t				how_much;
	bigtime_t				performance_time;
};

struct producer_set_run_mode_delay_command : command_data {
	BMediaNode::run_mode	mode;
	bigtime_t				delay;
};

struct producer_get_next_output_request : request_data {
	int32					cookie;
};

struct producer_get_next_output_reply : reply_data
{
	int32					cookie;
	media_output			output;
};

struct producer_dispose_output_cookie_request : request_data
{
	int32					cookie;
};

struct producer_dispose_output_cookie_reply : reply_data {
};


// #pragma mark - buffer consumer commands


struct consumer_accept_format_request : request_data {
	media_destination		dest;
	media_format			format;
};

struct consumer_accept_format_reply : reply_data {
	media_format			format;
};

struct consumer_connected_request : request_data {
	media_input				input;
};

struct consumer_connected_reply : reply_data {
	media_input				input;
};

struct consumer_get_next_input_request : request_data {
	int32					cookie;
};

struct consumer_get_next_input_reply : reply_data {
	int32					cookie;
	media_input				input;
};

struct consumer_dispose_input_cookie_request : request_data {
	int32					cookie;
};

struct consumer_dispose_input_cookie_reply : reply_data {
};

struct consumer_disconnected_request : request_data {
	media_source			source;
	media_destination		destination;
};

struct consumer_disconnected_reply : reply_data {
};

struct consumer_buffer_received_command : command_data {
	media_buffer_id			buffer;
	media_header			header;
};

struct consumer_producer_data_status_command : command_data {
	media_destination		for_whom;
	int32					status;
	bigtime_t				at_performance_time;
};

struct consumer_get_latency_for_request : request_data {
	media_destination		for_whom;
};

struct consumer_get_latency_for_reply : reply_data {
	bigtime_t				latency;
	media_node_id			timesource;
};

struct consumer_format_changed_request : request_data {
	media_source			producer;
	media_destination		consumer;
	int32					change_tag;
	media_format			format;
};

struct consumer_format_changed_reply : reply_data {
};

struct consumer_seek_tag_requested_request : request_data {
	media_destination		destination;
	bigtime_t				target_time;
	uint32					flags;
};

struct consumer_seek_tag_requested_reply : reply_data {
	media_seek_tag			seek_tag;
	bigtime_t				tagged_time;
	uint32					flags;
};


// #pragma mark - node commands


struct node_request_completed_command : command_data {
	media_request_info		info;
};

struct node_start_command : command_data {
	bigtime_t				performance_time;
};

struct node_stop_command : command_data {
	bigtime_t				performance_time;
	bool					immediate;
};

struct node_seek_command : command_data {
	bigtime_t				media_time;
	bigtime_t				performance_time;
};

struct node_set_run_mode_command : command_data {
	BMediaNode::run_mode	mode;
};

struct node_time_warp_command : command_data {
	bigtime_t				at_real_time;
	bigtime_t				to_performance_time;
};

struct node_set_timesource_command : command_data {
	media_node_id			timesource_id;
};

struct node_get_timesource_request : request_data {
};

struct node_get_timesource_reply : reply_data {
	media_node_id			timesource_id;
};

struct node_final_release_command : command_data {
};


// #pragma mark - time source commands


struct timesource_add_slave_node_command : command_data {
	media_node				node;
};

struct timesource_remove_slave_node_command : command_data {
	media_node				node;
};

struct timesource_get_start_latency_request : request_data {
};

struct timesource_get_start_latency_reply : reply_data {
	bigtime_t				start_latency;
};


// #pragma mark - file interface commands


struct fileinterface_set_ref_request : request_data {
	dev_t					device;
	ino_t					directory;
	char					name[B_FILE_NAME_LENGTH];
	bigtime_t				duration;
	bool					create;
};

struct fileinterface_set_ref_reply : reply_data {
	bigtime_t				duration;
};

struct fileinterface_get_ref_request : request_data {
};

struct fileinterface_get_ref_reply : reply_data {
	dev_t					device;
	ino_t					directory;
	char					name[B_FILE_NAME_LENGTH];
	char					mimetype[B_MIME_TYPE_LENGTH];
};

struct fileinterface_sniff_ref_request : request_data {
	dev_t					device;
	ino_t					directory;
	char					name[B_FILE_NAME_LENGTH];
};

struct fileinterface_sniff_ref_reply : reply_data {
	char					mimetype[B_MIME_TYPE_LENGTH];
	float					capability;
};


// #pragma mark - controllable commands


struct controllable_get_parameter_web_request : area_request_data {
	int32					max_size;
};

struct controllable_get_parameter_web_reply : reply_data {
	type_code				code;
	int32					size;
		// = -1: parameter web data too large,
		// = 0: no p.w., > 0: flattened p.w. data
};

#define MAX_PARAMETER_DATA (B_MEDIA_MESSAGE_SIZE - 100)

struct controllable_get_parameter_data_request : area_request_data {
	int32					parameter_id;
	size_t					request_size;
};

struct controllable_get_parameter_data_reply : reply_data {
	bigtime_t				last_change;
	char					raw_data[MAX_PARAMETER_DATA];
	size_t					size;
};

struct controllable_set_parameter_data_request : area_request_data {
	int32					parameter_id;
	bigtime_t				when;
	size_t					size;
	char					raw_data[MAX_PARAMETER_DATA];
};

struct controllable_set_parameter_data_reply : reply_data {
};

struct controllable_start_control_panel_request : request_data {
	media_node				node;
};

struct controllable_start_control_panel_reply : reply_data {
	team_id					team;
};


#endif	// _SERVER_INTERFACE_H_
