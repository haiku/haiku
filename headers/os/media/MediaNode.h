/*
 * Copyright 2009-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_NODE_H
#define _MEDIA_NODE_H


#include <MediaDefs.h>
#include <Point.h>

#include <new>


class BBufferConsumer;
class BBufferProducer;
class BControllable;
class BFileInterface;
class BMediaAddOn;
class BTimeSource;


class media_node {
public:
								media_node();
								~media_node();

			media_node_id		node;
			port_id				port;
			uint32				kind;

	static const media_node		null;

private:
			uint32				_reserved_[3];
};


struct media_input {
								media_input();
								~media_input();

			media_node			node;
			media_source		source;
			media_destination	destination;
			media_format		format;
			char				name[B_MEDIA_NAME_LENGTH];

private:
			uint32				_reserved_media_input_[4];
};


struct media_output {
								media_output();
								~media_output();

			media_node			node;
			media_source		source;
			media_destination	destination;
			media_format		format;
			char				name[B_MEDIA_NAME_LENGTH];

private:
			uint32				_reserved_media_output_[4];
};


struct live_node_info {
								live_node_info();
								~live_node_info();

			media_node			node;
			BPoint				hint_point;
			char				name[B_MEDIA_NAME_LENGTH];

private:
			char				reserved[160];
};


struct media_request_info {
			enum what_code {
				B_SET_VIDEO_CLIPPING_FOR = 1,
				B_REQUEST_FORMAT_CHANGE,
				B_SET_OUTPUT_ENABLED,
				B_SET_OUTPUT_BUFFERS_FOR,

				B_FORMAT_CHANGED = 4097
			};

			what_code			what;
			int32				change_tag;
			status_t			status;
			void*				cookie;
			void*				user_data;
			media_source		source;
			media_destination	destination;
			media_format		format;

			uint32				_reserved_[32];
};


struct media_node_attribute {
			enum {
				B_R40_COMPILED = 1,
				B_USER_ATTRIBUTE_NAME = 0x1000000,
				B_FIRST_USER_ATTRIBUTE
			};

			uint32				what;
			uint32				flags;
			int64				data;
};


namespace BPrivate {
	namespace media {
		class TimeSourceObject;
		class SystemTimeSourceObject;
		class BMediaRosterEx;
	}
} // BPrivate::media


/*!	BMediaNode is the indirect base class for all Media Kit participants.
	However, you should use the more specific BBufferConsumer, BBufferProducer
	and others rather than BMediaNode directly. It's OK to multiply inherit.
*/
class BMediaNode {
protected:
	// NOTE: Call Release() to destroy a node.
	virtual								~BMediaNode();

public:
			enum run_mode {
				B_OFFLINE = 1,
					// This mode has no realtime constraint.
				B_DECREASE_PRECISION,
					// When late, try to catch up by reducing quality.
				B_INCREASE_LATENCY,
					// When late, increase the presentation time offset.
				B_DROP_DATA,
					// When late, try to catch up by dropping buffers.
				B_RECORDING
					// For nodes on the receiving end of recording.
					// Buffers will always be late.
			};

			BMediaNode*			Acquire();
			BMediaNode*			Release();

			const char*			Name() const;
			media_node_id		ID() const;
			uint64				Kinds() const;
			media_node			Node() const;
			run_mode			RunMode() const;
			BTimeSource*		TimeSource() const;

	// ID of the port used to listen to control messages.
	virtual	port_id				ControlPort() const;

	// Who instantiated this node or NULL for application internal class.
	virtual	BMediaAddOn*		AddOn(int32* internalID) const = 0;

	// Message constants which will be sent to anyone watching the
	// MediaRoster. The message field "be:node_id" will contain the node ID.
			enum node_error {
				// Note that these belong with the notifications in
				// MediaDefs.h! They are here to provide compiler type
				// checking in ReportError().
				B_NODE_FAILED_START					= 'TRI0',
				B_NODE_FAILED_STOP					= 'TRI1',
				B_NODE_FAILED_SEEK					= 'TRI2',
				B_NODE_FAILED_SET_RUN_MODE			= 'TRI3',
				B_NODE_FAILED_TIME_WARP				= 'TRI4',
				B_NODE_FAILED_PREROLL				= 'TRI5',
				B_NODE_FAILED_SET_TIME_SOURCE_FOR	= 'TRI6',
				B_NODE_IN_DISTRESS					= 'TRI7'
				// What field 'TRIA' and up are used in MediaDefs.h
			};

protected:
	// Sends one of the above codes to anybody who's watching. You can
	// provide an optional message with additional information.
			status_t			ReportError(node_error what,
									const BMessage* info = NULL);

	// When you've handled a stop request, call this function. If anyone is
	// listening for stop information from you, they will be notified.
	// Especially important for offline capable Nodes.
			status_t			NodeStopped(bigtime_t performanceTime);
			void				TimerExpired(bigtime_t notifyPerformanceTime,
									int32 cookie, status_t error = B_OK);

	// NOTE: Constructor initializes the reference count to 1.
	explicit					BMediaNode(const char* name);

				status_t		WaitForMessage(bigtime_t waitUntil,
									uint32 flags = 0, void* _reserved_ = 0);

	// These don't return errors; instead, they use the global error condition
	// reporter. A node is required to have a queue of at least one pending
	// command (plus TimeWarp) and is recommended to allow for at least one
	// pending command of each type. Allowing an arbitrary number of
	// outstanding commands might be nice, but apps cannot depend on that
	// happening.
	virtual	void				Start(bigtime_t atPerformanceTime);
	virtual	void				Stop(bigtime_t atPerformanceTime,
									bool immediate);
	virtual	void				Seek(bigtime_t toMediaTime,
									bigtime_t atPerformanceTime);
	virtual	void				SetRunMode(run_mode mode);
	virtual	void				TimeWarp(bigtime_t atRealTime,
									bigtime_t toPerformanceTime);
	virtual	void				Preroll();
	virtual	void				SetTimeSource(BTimeSource* timeSource);

public:
	virtual	status_t			HandleMessage(int32 message, const void* data,
									size_t size);

	// Call this with messages you and your superclasses don't recognize.
			void				HandleBadMessage(int32 code,
									const void* buffer, size_t size);

	// Called from derived system classes; you don't need to
			void				AddNodeKind(uint64 kind);

	// These just call the default global versions for now.
			void*				operator new(size_t size);
			void*				operator new(size_t size,
									const std::nothrow_t&) throw();
			void				operator delete(void* ptr);
			void				operator delete(void* ptr,
									const std::nothrow_t&) throw();

protected:
	// Hook method which is called when requests have completed, or failed.
	virtual	status_t			RequestCompleted(
									const media_request_info & info);

	virtual	status_t			DeleteHook(BMediaNode* node);

	virtual	void				NodeRegistered();

public:

	// Fill out your attributes in the provided array, returning however
	// many you have.
	virtual	status_t			GetNodeAttributes(
									media_node_attribute* _attributes,
									size_t inMaxCount);

	virtual	status_t			AddTimer(bigtime_t atPerformanceTime,
									int32 cookie);

private:
			friend class BTimeSource;
			friend class BMediaRoster;
			friend class BBufferProducer;
			friend class BPrivate::media::TimeSourceObject;
			friend class BPrivate::media::SystemTimeSourceObject;
			friend class BPrivate::media::BMediaRosterEx;

			// Deprecated in BeOS R4.1
			int32				IncrementChangeTag();
			int32				ChangeTag();
			int32				MintChangeTag();
			status_t			ApplyChangeTag(int32 previouslyReserved);

private:
	// FBC padding and forbidden methods
			status_t			_Reserved_MediaNode_0(void*);
				// RequestCompletionHook()
			status_t			_Reserved_MediaNode_1(void*);
				// DeleteHook()
			status_t			_Reserved_MediaNode_2(void*);
				// NodeRegistered()
			status_t			_Reserved_MediaNode_3(void*);
				// GetNodeAttributes()
			status_t			_Reserved_MediaNode_4(void*);
				// AddTimer()
	virtual	status_t			_Reserved_MediaNode_5(void*);
	virtual	status_t			_Reserved_MediaNode_6(void*);
	virtual	status_t			_Reserved_MediaNode_7(void*);
	virtual	status_t			_Reserved_MediaNode_8(void*);
	virtual	status_t			_Reserved_MediaNode_9(void*);
	virtual	status_t			_Reserved_MediaNode_10(void*);
	virtual	status_t			_Reserved_MediaNode_11(void*);
	virtual	status_t			_Reserved_MediaNode_12(void*);
	virtual	status_t			_Reserved_MediaNode_13(void*);
	virtual	status_t			_Reserved_MediaNode_14(void*);
	virtual	status_t			_Reserved_MediaNode_15(void*);

								BMediaNode();
								BMediaNode(const BMediaNode& other);
			BMediaNode&			operator=(const BMediaNode& other);

private:
								BMediaNode(const char* name,
									media_node_id id, uint32 kinds);

			void				_InitObject(const char* name,
									media_node_id id, uint64 kinds);

private:
			media_node_id		fNodeID;
			BTimeSource*		fTimeSource;
			int32				fRefCount;
			char				fName[B_MEDIA_NAME_LENGTH];
			run_mode			fRunMode;

			int32				_reserved[2];

			uint64				fKinds;
			media_node_id		fTimeSourceID;

			BBufferProducer*	fProducerThis;
			BBufferConsumer*	fConsumerThis;
			BFileInterface*		fFileInterfaceThis;
			BControllable*		fControllableThis;
			BTimeSource*		fTimeSourceThis;

			bool				_reservedBool[4];

	mutable	port_id				fControlPort;

			uint32				_reserved_media_node_[8];

protected:
	static	int32				NewChangeTag();
		// for use by BBufferConsumer, mostly

private:
	// NOTE: Dont' rename this one, it's static and needed for binary
	// compatibility
	static	int32 _m_changeTag;
		// not to be confused with _mChangeCount
};


#endif // _MEDIA_NODE_H
