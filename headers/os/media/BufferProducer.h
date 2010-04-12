/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BUFFER_PRODUCER_H
#define _BUFFER_PRODUCER_H


#include <MediaNode.h>


class BBuffer;
class BBufferGroup;
class BRegion;


namespace BPrivate {
	namespace media {
		class BMediaRosterEx;
	}
}


class BBufferProducer : public virtual BMediaNode {
protected:
	// NOTE: This has to be at the top to force a vtable.
	virtual						~BBufferProducer();

public:

	// Supported formats for low-level clipping data
	enum {
		B_CLIP_SHORT_RUNS = 1
	};

	// Handy conversion function for dealing with clip information.
	static	status_t			ClipDataToRegion(int32 format, int32 size,
									const void* data,  BRegion* region);

			media_type			ProducerType();

protected:
	explicit					BBufferProducer(media_type producer_type
									/* = B_MEDIA_UNKNOWN_TYPE */);

	enum suggestion_quality {
		B_ANY_QUALITY		= 0,
		B_LOW_QUALITY		= 10,
		B_MEDIUM_QUALITY	= 50,
		B_HIGH_QUALITY		= 100
	};

	// BBufferProducer interface
	virtual	status_t			FormatSuggestionRequested(media_type type,
									int32 quality, media_format* format) = 0;
	virtual	status_t			FormatProposal(const media_source& output,
									media_format* ioFormat) = 0;

	// If the format isn't good, put a good format into ioFormat and
	// return error.
	// If format has wildcard, specialize to what you can do (and change).
	// If you can change the format, return OK.
	// The request comes from your destination sychronously, so you cannot ask
	// it whether it likes it -- you should assume it will since it asked.
	virtual	status_t			FormatChangeRequested(
									const media_source& source,
									const media_destination& destination,
									media_format* ioFormat,
									int32* _deprecated_) = 0;
	virtual	status_t			GetNextOutput(
									int32* ioCookie,
									media_output* _output) = 0;
	virtual	status_t			DisposeOutputCookie(int32 cookie) = 0;

	// In this function, you should either pass on the group to your upstream
	// guy, or delete your current group and hang on to this group. Deleting
	// the previous group (unless you passed it on with the reclaim flag set
	// to false) is very important, else you will 1) leak memory and 2) block
	// someone who may want to reclaim the buffers living in that group.
	virtual	status_t			SetBufferGroup(const media_source& forSource,
									BBufferGroup* group) = 0;

	// Format of clipping is (as int16-s): <from line> <npairs> <startclip>
	// <endclip>. Repeat for each line where the clipping is different from
	// the previous line. If <npairs> is negative, use the data from line
	// -<npairs> (there are 0 pairs after a negative <npairs>. Yes, we only
	// support 32k*32k frame buffers for clipping. Any non-0 field of
	// 'display' means that that field changed, and if you don't support that
	// change, you should return an error and ignore the request. Note that
	// the buffer offset values do not have wildcards; 0 (or -1, or whatever)
	// are real values and must be adhered to.
	virtual	status_t			VideoClippingChanged(
									const media_source& forSource,
									int16 numShorts,
									int16* clipData,
									const media_video_display_info& display,
									int32 * _deprecated_);

	// Iterates over all outputs and maxes the latency found
	virtual	status_t			GetLatency(bigtime_t* _lantency);

	virtual	status_t			PrepareToConnect(const media_source& what,
									const media_destination& where,
									media_format* format,
									media_source* _source,
									char* _name) = 0;
	virtual	void				Connect(status_t error,
									const media_source& source,
									const media_destination& destination,
									const media_format& format,
									char* ioName) = 0;
	virtual	void				Disconnect(const media_source& what,
									const media_destination& where) = 0;

	virtual	void				LateNoticeReceived(const media_source& what,
									bigtime_t howMuch,
									bigtime_t performanceTime) = 0;

	virtual	void				EnableOutput(const media_source& what,
									bool enabled, int32* _deprecated_) = 0;

	virtual	status_t			SetPlayRate(int32 numer, int32 denom);

	// NOTE: Call this from the thread that listens to the port!
	virtual	status_t			HandleMessage(int32 message, const void* data,
									size_t size);

	virtual	void				AdditionalBufferRequested(
									const media_source& source,
									media_buffer_id previousBuffer,
									bigtime_t previousTime,
									const media_seek_tag* previousTag
										/* = NULL */);

	virtual	void				LatencyChanged(const media_source& source,
									const media_destination& destination,
									bigtime_t newLatency, uint32 flags);

	// NOTE: Use this function to pass on the buffer on to the BBufferConsumer.
			status_t			SendBuffer(BBuffer* buffer,
									const media_source& source, 
									const media_destination& destination);

			status_t			SendDataStatus(int32 status,
									const media_destination& destination,
									bigtime_t atTime);

	// Check in advance if a target is prepared to accept a format. You may
	// want to call this from Connect(), although that's not required.
			status_t			ProposeFormatChange(media_format* format,
									const media_destination& forDestination);

	// Tell consumer to accept a proposed format change
	// NOTE: You must not call SendBuffer while this call is pending!
			status_t			ChangeFormat(const media_source& forSource,
									const media_destination& forDestination,
									media_format* format);

	// Check how much latency the down-stream graph introduces.
			status_t			FindLatencyFor(
									const media_destination& forDestination,
									bigtime_t* _latency,
									media_node_id* _timesource);

	// Find the tag of a previously seen buffer to expedite seeking
			status_t			FindSeekTag(
									const media_destination& forDestination,
									bigtime_t inTargetTime,
									media_seek_tag* _tag,
									bigtime_t* _taggedTime, uint32* _flags = 0,
									uint32 flags = 0);

	// Set the initial latency, which is the maximum additional latency
	// that will be imposed while starting/syncing to a signal (such as
	// starting a TV capture card in the middle of a field). Most nodes
	// have this at 0 (the default); only TV input Nodes need it currently
	// because they slave to a low-resolution (59.94 Hz) clock that arrives
	// from the outside world. Call this from the constructor if you need it.
			void				SetInitialLatency(bigtime_t inInitialLatency,
									uint32 flags = 0);

	// TODO: Needs a Perform() virtual method!

private:
	// FBC padding and forbidden methods
								BBufferProducer();
								BBufferProducer(const BBufferProducer& other);
			BBufferProducer&	operator=(const BBufferProducer& other);

			status_t			_Reserved_BufferProducer_0(void*);
				// was AdditionalBufferRequested()
			status_t			_Reserved_BufferProducer_1(void*);
				// was LatencyChanged()
	virtual	status_t			_Reserved_BufferProducer_2(void*);
	virtual	status_t			_Reserved_BufferProducer_3(void*);
	virtual	status_t			_Reserved_BufferProducer_4(void*);
	virtual	status_t			_Reserved_BufferProducer_5(void*);
	virtual	status_t			_Reserved_BufferProducer_6(void*);
	virtual	status_t			_Reserved_BufferProducer_7(void*);
	virtual	status_t			_Reserved_BufferProducer_8(void*);
	virtual	status_t			_Reserved_BufferProducer_9(void*);
	virtual	status_t			_Reserved_BufferProducer_10(void*);
	virtual	status_t			_Reserved_BufferProducer_11(void*);
	virtual	status_t			_Reserved_BufferProducer_12(void*);
	virtual	status_t			_Reserved_BufferProducer_13(void*);
	virtual	status_t			_Reserved_BufferProducer_14(void*);
	virtual	status_t			_Reserved_BufferProducer_15(void*);

	// deprecated calls
			status_t			SendBuffer(BBuffer* buffer,
									const media_destination& destination);

private:
			friend class BBufferConsumer;
			friend class BMediaNode;
			friend class BMediaRoster;
			friend class BPrivate::media::BMediaRosterEx;

	static	status_t			clip_shorts_to_region(const int16* data,
									int count, BRegion* output);
	static	status_t			clip_region_to_shorts(const BRegion* input,
									int16* data, int maxCount, int* _count);

private:
			media_type			fProducerType;
			bigtime_t			fInitialLatency;
			uint32				fInitialFlags;
			bigtime_t			fDelay;

			uint32				_reserved_buffer_producer_[12];
};

#endif // _BUFFER_PRODUCER_H

