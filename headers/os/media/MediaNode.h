/*******************************************************************************
/
/	File:			MediaNode.h
/
/   Description:  BMediaNode is the indirect base class for all Media Kit participants.
/	However, you should use the more specific BBufferConsumer, BBufferProducer
/	and others rather than BMediaNode directly. It's OK to multiply inherit.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#if !defined(_MEDIA_NODE_H)
#define _MEDIA_NODE_H

#include <MediaDefs.h>
#include <Point.h>

#include <new>

class BMediaAddOn;



class media_node {

public:

	media_node();
	~media_node();

	media_node_id node;
	port_id port;
	uint32 kind;

static media_node null;

private:
	uint32 _reserved_[3];
};


struct media_input {
	media_input();
	~media_input();
	media_node node;
	media_source source;
	media_destination destination;
	media_format format;
	char name[B_MEDIA_NAME_LENGTH];
private:
	uint32 _reserved_media_input_[4];
};

struct media_output {
	media_output();
	~media_output();
	media_node node;
	media_source source;
	media_destination destination;
	media_format format;
	char name[B_MEDIA_NAME_LENGTH];
private:
	uint32 _reserved_media_output_[4];
};

struct live_node_info {
	live_node_info();
	~live_node_info();
	media_node node;
	BPoint hint_point;
	char name[B_MEDIA_NAME_LENGTH];
private:
	char reserved[160];
};


struct media_request_info
{
	enum what_code
	{
		B_SET_VIDEO_CLIPPING_FOR = 1, 
		B_REQUEST_FORMAT_CHANGE, 
		B_SET_OUTPUT_ENABLED,
		B_SET_OUTPUT_BUFFERS_FOR,

		B_FORMAT_CHANGED = 4097
	};
	what_code what;
	int32 change_tag;
	status_t status;
	int32 cookie;
	void * user_data;
	media_source source;
	media_destination destination;
	media_format format;
	uint32 _reserved_[32];
};

struct media_node_attribute
{
	enum {
		B_R40_COMPILED = 1,		//	has this attribute if compiled using R4.0 headers
		B_USER_ATTRIBUTE_NAME = 0x1000000,
		B_FIRST_USER_ATTRIBUTE
	};
	uint32		what;
	uint32		flags;			//	per attribute
	int64		data;			//	per attribute
};


class BMediaNode
{
protected:
		/* this has to be on top rather than bottom to force a vtable in mwcc */
virtual	~BMediaNode();	/* should be called through Release() */
public:

		enum run_mode {
			B_OFFLINE = 1,				/* data accurate, no realtime constraint */
			B_DECREASE_PRECISION,		/* when slipping, try to catch up */
			B_INCREASE_LATENCY,			/* when slipping, increase playout delay */
			B_DROP_DATA,				/* when slipping, skip data */
			B_RECORDING					/* you're on the receiving end of recording; buffers are guaranteed to be late */
		};


		BMediaNode * Acquire();	/* return itself */
		BMediaNode * Release();	/* release will decrement refcount, and delete if 0 */

		const char * Name() const;
		media_node_id ID() const;
		uint64 Kinds() const;
		media_node Node() const;
		run_mode RunMode() const;
		BTimeSource * TimeSource() const;

	/* this port is what a media node listens to for commands */
virtual	port_id ControlPort() const;

virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const = 0;	/* Who instantiated you -- or NULL for app class */

	/* These will be sent to anyone watching the MediaRoster. */
	/* The message field "be:node_id" will contain the node ID. */
		enum node_error {
			/* Note that these belong with the notifications in MediaDefs.h! */
			/* They are here to provide compiler type checking in ReportError(). */
			B_NODE_FAILED_START = 'TRI0',
			B_NODE_FAILED_STOP,						// TRI1
			B_NODE_FAILED_SEEK,						// TRI2
			B_NODE_FAILED_SET_RUN_MODE,				// TRI3
			B_NODE_FAILED_TIME_WARP,				// TRI4
			B_NODE_FAILED_PREROLL,					// TRI5
			B_NODE_FAILED_SET_TIME_SOURCE_FOR,		// TRI6
			/* display this node with a blinking exclamation mark or something */
			B_NODE_IN_DISTRESS						// TRI7
			/* TRIA and up are used in MediaDefs.h */
		};

protected:

		/* Send one of the above codes to anybody who's watching. */
		status_t ReportError(
				node_error what,
				const BMessage * info = NULL);	/* String "message" for instance */

		/* When you've handled a stop request, call this function. If anyone is */
		/* listening for stop information from you, they will be notified. Especially */
		/* important for offline capable Nodes. */
		status_t NodeStopped(
				bigtime_t whenPerformance);	//	performance time
		void TimerExpired(
				bigtime_t notifyPoint,		//	performance time
				int32 cookie,
				status_t error = B_OK);

explicit 	BMediaNode(		/* constructor sets refcount to 1 */
				const char * name);

		status_t WaitForMessage(
				bigtime_t waitUntil,
				uint32 flags = 0,
				void * _reserved_ = 0);

		/* These don't return errors; instead, they use the global error condition reporter. */
		/* A node is required to have a queue of at least one pending command (plus TimeWarp) */
		/* and is recommended to allow for at least one pending command of each type. */
		/* Allowing an arbitrary number of outstanding commands might be nice, but apps */
		/* cannot depend on that happening. */
virtual	void Start(
				bigtime_t performance_time);
virtual	void Stop(
				bigtime_t performance_time,
				bool immediate);
virtual	void Seek(
				bigtime_t media_time,
				bigtime_t performance_time);
virtual	void SetRunMode(
				run_mode mode);
virtual	void TimeWarp(
				bigtime_t at_real_time,
				bigtime_t to_performance_time);
virtual	void Preroll();
virtual	void SetTimeSource(
				BTimeSource * time_source);

public:

virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);
		void HandleBadMessage(		/* call this with messages you and your superclasses don't recognize */
				int32 code,
				const void * buffer,
				size_t size);

		/* Called from derived system classes; you don't need to */
		void AddNodeKind(
				uint64 kind);

		//	These were not in 4.0.
		//	We added them in 4.1 for future use. They just call
		//	the default global versions for now.
		void * operator new(
				size_t size);
		void * operator new(
				size_t size,
				const nothrow_t &) throw();
		void operator delete(
				void * ptr);
#if !__MWERKS__
		//	there's a bug in MWCC under R4.1 and earlier
		void operator delete(
				void * ptr, 
				const nothrow_t &) throw();
#endif

protected:

		/* Called when requests have completed, or failed. */
virtual	status_t RequestCompleted(	/* reserved 0 */
				const media_request_info & info);

private:
	friend class BTimeSource;
	friend class _BTimeSourceP;
	friend class BMediaRoster;
	friend class _BMediaRosterP;
	friend class MNodeManager;
	friend class BBufferProducer;	//	for getting _mNodeID

		// Deprecated in 4.1
		int32 IncrementChangeTag();
		int32 ChangeTag();
		int32 MintChangeTag();
		status_t ApplyChangeTag(
				int32 previously_reserved);

		/* Mmmh, stuffing! */
protected:

virtual		status_t DeleteHook(BMediaNode * node);		/* reserved 1 */

virtual		void NodeRegistered();	/* reserved 2 */

public:

		/* fill out your attributes in the provided array, returning however many you have. */
virtual		status_t GetNodeAttributes(	/* reserved 3 */
					media_node_attribute * outAttributes,
					size_t inMaxCount);

virtual		status_t AddTimer(
					bigtime_t at_performance_time,
					int32 cookie);

private:

			status_t _Reserved_MediaNode_0(void *);		/* DeleteHook() */
			status_t _Reserved_MediaNode_1(void *);		/* RequestCompletionHook() */
			status_t _Reserved_MediaNode_2(void *);		/* NodeRegistered() */
			status_t _Reserved_MediaNode_3(void *);		/* GetNodeAttributes() */
			status_t _Reserved_MediaNode_4(void *);		/* AddTimer() */
virtual		status_t _Reserved_MediaNode_5(void *);
virtual		status_t _Reserved_MediaNode_6(void *);
virtual		status_t _Reserved_MediaNode_7(void *);
virtual		status_t _Reserved_MediaNode_8(void *);
virtual		status_t _Reserved_MediaNode_9(void *);
virtual		status_t _Reserved_MediaNode_10(void *);
virtual		status_t _Reserved_MediaNode_11(void *);
virtual		status_t _Reserved_MediaNode_12(void *);
virtual		status_t _Reserved_MediaNode_13(void *);
virtual		status_t _Reserved_MediaNode_14(void *);
virtual		status_t _Reserved_MediaNode_15(void *);

		BMediaNode();	/* private unimplemented */
		BMediaNode(
				const BMediaNode & clone);
		BMediaNode & operator=(
				const BMediaNode & clone);

		BMediaNode(		/* constructor sets refcount to 1 */
				const char * name,
				media_node_id id,
				uint32 kinds);

		media_node_id fNodeID;
		BTimeSource * fTimeSource;
		int32 fRefCount;
		char fName[B_MEDIA_NAME_LENGTH];
		run_mode fRunMode;
		int32 _mChangeCount;			//	deprecated
		int32 _mChangeCountReserved;	//	deprecated
		uint64 fKinds;
		media_node_id fTimeSourceID;
		bool _mReservedBool[4];

mutable	port_id fControlPort;

	uint32 _reserved_media_node_[13];



protected:

static	int32 NewChangeTag();	//	for use by BBufferConsumer, mostly

private:
		// dont' rename this one, it's static and needed for binary compatibility
static	int32 _m_changeTag;		//	not to be confused with _mChangeCount
};



#endif /* _MEDIA_NODE_H */

