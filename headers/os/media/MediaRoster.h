/*
 * Copyright (c) 2008 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the the MIT licence.
 *
 */

//! The BMediaRoster is the main API to the Media Kit.

#ifndef _MEDIA_ROSTER_H
#define _MEDIA_ROSTER_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <config_manager.h>

class BBufferGroup;
class BMediaAddOn;
class BMimeType;
class BParameterWeb;
class BString;

struct dormant_flavor_info;
struct entry_ref;

namespace BPrivate { namespace media {
	class DefaultDeleter;
	class BMediaRosterEx;
} } // BPrivate::media


class BMediaRoster : public BLooper {
public:

	// Retrieving the global instance of the BMediaRoster:
	static	BMediaRoster*		Roster(status_t* _error = NULL);
				// This version will create a roster instance if there isn't
				// one already. Thread safe for multiple calls to Roster().

	static	BMediaRoster*		CurrentRoster();
				// This version won't create an instance if there isn't one
				// already. It is not thread safe if you call Roster() at the
				// same time.

	// Getting common instances of system nodes:
			status_t			GetVideoInput(media_node* _node);
			status_t			GetAudioInput(media_node* _node);
			status_t			GetVideoOutput(media_node* _node);
			status_t			GetAudioMixer(media_node* _node);
			status_t			GetAudioOutput(media_node* _node);
				// The output should not be used directly in common use cases.
				// Use the mixer node instead.
			status_t			GetAudioOutput(media_node* _node,
									int32* _inputId, BString* _inputName);
			status_t			GetTimeSource(media_node* _node);

	// Setting common system nodes:
			status_t			SetVideoInput(const media_node& producer);
			status_t			SetVideoInput(
									const dormant_node_info& producer);

			status_t			SetAudioInput(const media_node& producer);
			status_t			SetAudioInput(
									const dormant_node_info& producer);

			status_t			SetVideoOutput(const media_node& consumer);
			status_t			SetVideoOutput(
									const dormant_node_info& consumer);

			status_t			SetAudioOutput(
									const media_node& consumer);
			status_t			SetAudioOutput(
									const media_input& inputToOutput);
			status_t			SetAudioOutput(
									const dormant_node_info& consumer);

	// Get a media_node from a node ID -- this is how you reference your
	// own nodes!
			status_t			GetNodeFor(media_node_id node,
									media_node* clone);
			status_t			GetSystemTimeSource(media_node* clone);
				// Typically, you'll want to use GetTimeSource() instead.
			status_t			ReleaseNode(const media_node& node);
				// This method might free the node if there are no
				// more references.
			BTimeSource*		MakeTimeSourceFor(const media_node& for_node);
				// Release() the returned object when done!

			status_t			Connect(const media_source& from,
									const media_destination& to,
									media_format* _inOutFormat,
									media_output* _output,
									media_input* _input);
				// Note that the media_source and media_destination found in
				// _output and _input are the ones actually used. From and to
				// are only "hints" that the app should not use once a real
				// connection has been established.

			enum connect_flags {
				B_CONNECT_MUTED = 0x1
			};
		
			status_t			Connect(const media_source& from,
									const media_destination& to,
									media_format* _inOutFormat,
									media_output* _output,
									media_input* _input,
									uint32 flags, void* _reserved = NULL);
				
			status_t			Disconnect(media_node_id sourceNode,
									const media_source& source,
									media_node_id destinationNode,
									const media_destination& destination);

		
			status_t			Disconnect(const media_output& output,
									const media_input& input);
				// NOTE: This is a Haiku extension.

			status_t			StartNode(const media_node& node,
									bigtime_t atPerformanceTime);
			status_t			StopNode(const media_node& node,
									bigtime_t atPerformanceTime,
									bool immediate = false);
				// If "immediate" is true, "atPerformanceTime" is ignored.
			status_t			SeekNode(const media_node& node,
									bigtime_t toMediaTime,
									bigtime_t atPerformanceTime = 0);
				// NOTE: The node needs to be running.

			status_t			StartTimeSource(const media_node& node,
									bigtime_t atRealTime);
			status_t			StopTimeSource(const media_node& node,
									bigtime_t atRealTime,
									bool immediate = false);
			status_t			SeekTimeSource(const media_node& node,
									bigtime_t toPerformanceTime,
									bigtime_t atRealTime);

			status_t			SyncToNode(const media_node& node,
									bigtime_t atTime,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
			status_t			SetRunModeNode(const media_node& node,
									BMediaNode::run_mode mode);
			status_t			PrerollNode(const media_node& node);
				// NOTE: This method is synchronous.
			status_t			RollNode(const media_node& node, 
									bigtime_t startPerformance,
									bigtime_t stopPerformance,
									bigtime_t atMediaTime
										= -B_INFINITE_TIMEOUT);

			status_t			SetProducerRunModeDelay(const media_node& node,
									bigtime_t delay,
									BMediaNode::run_mode mode
										= BMediaNode::B_RECORDING);
				// NOTE: Should only be used with B_RECORDING
			status_t			SetProducerRate(const media_node& producer,
									int32 numer, int32 denom);
				// NOTE: This feature is not necessarily supported by the node.

	// Retrieving information about available inputs/outputs.
	// Nodes will have available inputs/outputs as long as they are capable
	// of accepting more connections. The node may create an additional
	// output or input as the currently available is taken into usage.
			status_t			GetLiveNodeInfo(const media_node& node,
									live_node_info* _liveInfo);
			status_t			GetLiveNodes(live_node_info* _liveNodes,
									int32* inOutTotalCount,
									const media_format* hasInput = NULL,
									const media_format* hasOutput = NULL,
									const char* name = NULL,
									uint64 nodeKinds = 0);
										// B_BUFFER_PRODUCER etc.

			status_t			GetFreeInputsFor(const media_node& node,
									media_input* _freeInputsBuffer,
									int32 bufferCapacity,
									int32* _foundCount,
									media_type filterType
										= B_MEDIA_UNKNOWN_TYPE);
			status_t			GetConnectedInputsFor(const media_node& node,
									media_input* _activeInputsBuffer,
									int32 bufferCapacity,
									int32* _foundCount);
			status_t			GetAllInputsFor(const media_node& node,
									media_input* _inputsBuffer,
									int32 bufferCapacity,
									int32* _foundCount);
			status_t			GetFreeOutputsFor(const media_node& node,
									media_output* _freeOutputsBuffer,
									int32 bufferCapacity,
									int32* _foundCount,
									media_type filterType
										= B_MEDIA_UNKNOWN_TYPE);
			status_t			GetConnectedOutputsFor(const media_node& node,
									media_output* _activeOutputsBuffer,
									int32 bufferCapacity,
									int32* _foundCount);
			status_t			GetAllOutputsFor(const media_node& node,
									media_output* _outputsBuffer,
									int32 bufferCapacity,
									int32* _foundCount);

	// Event notification support:
			status_t			StartWatching(const BMessenger& target);
			status_t			StartWatching(const BMessenger& target,
									int32 notificationType);
			status_t			StartWatching(const BMessenger& target,
									const media_node& node,
									int32 notificationType);
			status_t			StopWatching(const BMessenger& target);
			status_t			StopWatching(const BMessenger& target,
									int32 notificationType);
			status_t			StopWatching(const BMessenger& target,
									const media_node& node,
									int32 notificationType);

			status_t			RegisterNode(BMediaNode* node);
			status_t			UnregisterNode(BMediaNode* node);

			status_t			SetTimeSourceFor(media_node_id node,
									media_node_id timeSource);

	// Building a control GUI for a node:
			status_t			GetParameterWebFor(const media_node& node, 
									BParameterWeb** _web);
			status_t			StartControlPanel(const media_node& node,
									BMessenger* _messenger = NULL);

	// Information about nodes which are not running, but could
	// be instantiated from add-ons:
			status_t			GetDormantNodes(dormant_node_info* _info,
									int32* _inOutCount,
									const media_format* _hasInput = NULL,
									const media_format* _hasOutput = NULL,
									const char* name = NULL,
									uint64 requireKinds = 0,
									uint64 denyKinds = 0);
			status_t			InstantiateDormantNode(
									const dormant_node_info& info,
									media_node* _node,
									uint32 flags);
				// NOTE: Supported flags are currently B_FLAVOR_IS_GLOBAL
				// or B_FLAVOR_IS_LOCAL
			status_t			InstantiateDormantNode(
									const dormant_node_info& info,
									media_node* _node);
			status_t			GetDormantNodeFor(const media_node& node,
									dormant_node_info* _info);
			status_t			GetDormantFlavorInfoFor(
									const dormant_node_info& info,
									dormant_flavor_info* _flavor);

	// Latency information:
			status_t			GetLatencyFor(const media_node& producer,
									bigtime_t* _latency);
			status_t			GetInitialLatencyFor(
									const media_node& producer,
									bigtime_t* _latency,
									uint32* _flags = NULL);
			status_t			GetStartLatencyFor(
									const media_node& timeSource,
									bigtime_t* _latency);

	// Finding a suitable node to playback a file:
			status_t			GetFileFormatsFor(
									const media_node& fileInterface, 
									media_file_format* _formatsBuffer,
									int32* _inOutNumInfos);
			status_t			SetRefFor(const media_node& fileInterface,
									const entry_ref& file,
									bool createAndTruncate,
									bigtime_t* _length);
										// if create is false
			status_t			GetRefFor(const media_node& node,
									entry_ref* _ref,
									BMimeType* mimeType = NULL);
			status_t			SniffRefFor(const media_node& fileInterface,
									const entry_ref& ref, BMimeType* _mimeType,
									float* _capability);
	// This is the generic "here's a file, now can someone please play it"
	// interface:
			status_t			SniffRef(const entry_ref& ref,
									uint64 requireNodeKinds,
										// if you need an EntityInterface
										// or BufferConsumer or something
									dormant_node_info* _node,
									BMimeType* _mimeType = NULL);
			status_t			GetDormantNodeForType(const BMimeType& type,
									uint64 requireNodeKinds,
									dormant_node_info* _info);
			status_t			GetReadFileFormatsFor(
									const dormant_node_info& node,
									media_file_format* _readFormatsBuffer,
									int32 bufferCapacity, int32* _foundCount);
			status_t			GetWriteFileFormatsFor(
									const dormant_node_info& node,
									media_file_format* _writeFormatsBuffer,
									int32 bufferCapacity, int32* _foundCount);

			status_t			GetFormatFor(const media_output& output,
									media_format* _inOutFormat,
									uint32 flags = 0);
			status_t			GetFormatFor(const media_input& input,
									media_format* _inOutFormat,
									uint32 flags = 0);
			status_t			GetFormatFor(const media_node& node,
									media_format* _inOutFormat,
									float quality = B_MEDIA_ANY_QUALITY);
			ssize_t				GetNodeAttributesFor(const media_node& node,
									media_node_attribute* outArray,
									size_t inMaxCount);
			media_node_id		NodeIDFor(port_id sourceOrDestinationPort);
			status_t			GetInstancesFor(media_addon_id addon,
									int32 flavor,media_node_id* _id,
									int32* _inOutCount = NULL);
										// _inOutCount defaults to 1
										// if unspecified (NULL).

	// General MediaKit configuration:
			status_t			SetRealtimeFlags(uint32 enabledFlags);
			status_t			GetRealtimeFlags(uint32* _enabledFlags);
			ssize_t				AudioBufferSizeFor(int32 channelCount,
									uint32 sampleFormat, float frameRate,
									bus_type busType = B_UNKNOWN_BUS);

	// Use MediaFlags to inquire about specific features of the Media Kit.
	// Returns < 0 for "not present", positive size for output data size.
	// 0 means that the capability is present, but no data about it.
	static	ssize_t				MediaFlags(media_flags cap, void* buffer,
									size_t maxSize);

	// BLooper overrides
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual						~BMediaRoster();

private:
	// This method is deprecated:
			status_t			SetOutputBuffersFor(const media_source& output,
									BBufferGroup* group,
									bool willReclaim = false);

	// Reserving virtual function slots.
	virtual	status_t			_Reserved_MediaRoster_0(void*);
	virtual	status_t			_Reserved_MediaRoster_1(void*);
	virtual	status_t			_Reserved_MediaRoster_2(void*);
	virtual	status_t			_Reserved_MediaRoster_3(void*);
	virtual	status_t			_Reserved_MediaRoster_4(void*);
	virtual	status_t			_Reserved_MediaRoster_5(void*);
	virtual	status_t			_Reserved_MediaRoster_6(void*);
	virtual	status_t			_Reserved_MediaRoster_7(void*);

	friend class BPrivate::media::DefaultDeleter;
	friend class BPrivate::media::BMediaRosterEx;

	// Constructor is private, since you are supposed to use
	// Roster() or CurrentRoster().
								BMediaRoster();

	// TODO: Looks like these can be safely removed:
	static	status_t			ParseCommand(BMessage& reply);

			status_t			GetDefaultInfo(media_node_id forDefault,
									BMessage& _config);
			status_t			SetRunningDefault(media_node_id forDefault,
									const media_node& node);

private:
			uint32				_reserved_media_roster_[67];

	static	BMediaRoster*		sDefaultInstance;
};


#endif // _MEDIA_ROSTER_H

