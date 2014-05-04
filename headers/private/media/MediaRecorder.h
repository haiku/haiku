/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_RECORDER_H
#define _MEDIA_RECORDER_H


#include <MediaNode.h>
#include <TimeSource.h>

#include "MediaRecorderNode.h"
#include "SoundUtils.h"


namespace BPrivate { namespace media {

class BMediaRecorder {
public:
								BMediaRecorder(const char* name,
									media_type type
										= B_MEDIA_UNKNOWN_TYPE);

	virtual						~BMediaRecorder();

			status_t			InitCheck() const;

	typedef void				(*ProcessFunc)(void* cookie,
									bigtime_t timestamp, void* data,
									size_t datasize,
									const media_format& format);

	typedef void				(*NotifyFunc)(void* cookie,
									int32 code, ...);

			status_t			SetHooks(ProcessFunc recordFunc = NULL,
									NotifyFunc notifyFunc = NULL,
									void* cookie = NULL);

			void				SetAcceptedFormat(
									const media_format& format);

	virtual status_t			Start(bool force = false);
	virtual status_t			Stop(bool force = false);

	virtual status_t			Connect(const media_format& format,
									uint32 flags = 0);

	virtual status_t			Connect(const dormant_node_info& dormantInfo,
									const media_format* format = NULL,
									uint32 flags = 0);

	virtual status_t			Connect(const media_node& node,
									const media_output* useOutput = NULL,
									const media_format* format = NULL,
									uint32 flags = 0);

	virtual status_t			Disconnect();

			bool				IsConnected() const;
			bool				IsRunning() const;

			const media_format&	Format() const;

			const media_output&	MediaOutput() const;
			const media_input&	MediaInput() const;

protected:

	virtual	void				BufferReceived(void* buffer,
									size_t size,
									const media_header& header);
private:

		status_t				_Connect(
									const media_format* format,
									uint32 flags,
									const dormant_node_info* dormantNode,
									const media_node* mediaNode,
									const media_output* output);

		status_t				fInitErr;

		bool					fConnected;
		bool					fRunning;

		BTimeSource*			fTimeSource;

		ProcessFunc				fRecordHook;
		NotifyFunc				fNotifyHook;

		media_node				fOutputNode;
		media_output			fOutput;

		BMediaRecorderNode*		fNode;
		media_input				fInput;

		void*					fBufferCookie;

		friend class			BMediaRecorderNode;
};

}

}

using namespace BPrivate::media;

#endif	//	_MEDIA_RECORDER_H
