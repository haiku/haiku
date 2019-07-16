/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_RECORDER_H
#define _MEDIA_RECORDER_H


#include <MediaDefs.h>
#include <MediaNode.h>


namespace BPrivate { namespace media {
class BMediaRecorderNode;
}
}


class BMediaRecorder {
public:
	enum notification {
		B_WILL_START = 1,	// performance_time
		B_WILL_STOP,		// performance_time immediate
		B_WILL_SEEK,		// performance_time media_time
		B_WILL_TIMEWARP,	// real_time performance_time
	};

	typedef void				(*ProcessFunc)(void* cookie,
									bigtime_t timestamp, void* data,
									size_t size, const media_format& format);

	typedef void				(*NotifyFunc)(void* cookie,
									notification what, ...);

public:
								BMediaRecorder(const char* name,
									media_type type
										= B_MEDIA_UNKNOWN_TYPE);

	virtual						~BMediaRecorder();

			status_t			InitCheck() const;

			status_t			SetHooks(ProcessFunc recordFunc = NULL,
									NotifyFunc notifyFunc = NULL,
									void* cookie = NULL);

			void				SetAcceptedFormat(
									const media_format& format);
			const media_format&	AcceptedFormat() const;

	virtual status_t			Start(bool force = false);
	virtual status_t			Stop(bool force = false);

	virtual status_t			Connect(const media_format& format);

	virtual status_t			Connect(const dormant_node_info& dormantInfo,
									const media_format& format);

	virtual status_t			Connect(const media_node& node,
									const media_output* output = NULL,
									const media_format* format = NULL);

	virtual status_t			Disconnect();

			bool				IsConnected() const;
			bool				IsRunning() const;

			const media_format&	Format() const;

protected:
			// Get the producer node source
			const media_source&	MediaSource() const;
			// This is the our own input
			const media_input	MediaInput() const;

	virtual	void				BufferReceived(void* buffer, size_t size,
									const media_header& header);

			status_t			SetUpConnection(media_source outputSource);

private:

			status_t			_Connect(const media_node& mediaNode,
									const media_output* output,
									const media_format& format);

	virtual	void				_ReservedMediaRecorder0();
	virtual	void				_ReservedMediaRecorder1();
	virtual	void				_ReservedMediaRecorder2();
	virtual	void				_ReservedMediaRecorder3();
	virtual	void				_ReservedMediaRecorder4();
	virtual	void				_ReservedMediaRecorder5();
	virtual	void				_ReservedMediaRecorder6();
	virtual	void				_ReservedMediaRecorder7();

			status_t			fInitErr;

			bool				fConnected;
			bool				fRunning;
			bool				fReleaseOutputNode;

			ProcessFunc			fRecordHook;
			NotifyFunc			fNotifyHook;

			media_node			fOutputNode;
			media_source		fOutputSource;

			BPrivate::media::BMediaRecorderNode* fNode;

			void*				fBufferCookie;
			uint32				fPadding[32];

			friend class		BPrivate::media::BMediaRecorderNode;
};

#endif	//	_MEDIA_RECORDER_H
