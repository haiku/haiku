/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIDEO_CONSUMER_H
#define VIDEO_CONSUMER_H


#include <Bitmap.h>
#include <BufferConsumer.h>
#include <MediaEventLooper.h>
#include <MediaNode.h>
#include <TimedEventQueue.h>
#include <TranslatorRoster.h>
#include <View.h>
#include <Window.h>


typedef struct {
	port_id		port;
	bigtime_t	rate;
	uint32		imageFormat;
	int32		translator;
	int32		uploadClient;
	bool		passiveFtp;
	char		fileNameText[64];
	char		serverText[64];
	char		loginText[64];
	char		passwordText[64];
	char		directoryText[64];
} ftp_msg_info;

#define FTP_INFO 0x60000001

class BStringView;



class VideoConsumer : public BMediaEventLooper, public BBufferConsumer {
public:
								VideoConsumer(const char* name, BView* view,
									BStringView* statusLine, BMediaAddOn *addon,
									const uint32 internalId);
								~VideoConsumer();

/*	BMediaNode */
public:
	virtual	BMediaAddOn* 		AddOn(int32* cookie) const;

protected:
	virtual void 				Start(bigtime_t performanceTime);
	virtual void 				Stop(bigtime_t performanceTime, bool immediate);
	virtual void 				Seek(bigtime_t mediaTime,
									bigtime_t performanceTime);
	virtual void 				TimeWarp(bigtime_t atRealTime,
									bigtime_t toPerformanceTime);

	virtual void 				NodeRegistered();
	virtual	status_t 			RequestCompleted(
									const media_request_info& info);

	virtual	status_t 			HandleMessage(int32 message, const void* data,
									size_t size);

	virtual status_t 			DeleteHook(BMediaNode* node);

/*  BMediaEventLooper */
protected:
		virtual void 			HandleEvent(const media_timed_event* event,
									bigtime_t lateness, bool realTimeEvent);

/*	BBufferConsumer */
public:
	virtual	status_t 			AcceptFormat(const media_destination& dest,
									media_format* format);

	virtual	status_t 			GetNextInput(int32* cookie,
									media_input* outInput);

	virtual	void 				DisposeInputCookie(int32 cookie);

protected:
	virtual	void 				BufferReceived(BBuffer* buffer);

private:
	virtual	void 				ProducerDataStatus(
									const media_destination &forWhom,
									int32 status, bigtime_t atMediaTime);

	virtual	status_t 			GetLatencyFor(const media_destination& forWhom,
									bigtime_t* outLatency,
									media_node_id* outId);

	virtual	status_t 			Connected(const media_source& producer,
									const media_destination& where,
									const media_format& withFormat,
									media_input* outInput);

	virtual	void 				Disconnected(const media_source& producer,
									const media_destination& where);

	virtual	status_t 			FormatChanged(const media_source& producer,
									const media_destination& consumer,
									int32 fromChangeCount,
									const media_format& format);

/*	implementation */

public:
			status_t 			CreateBuffers(const media_format& withFormat);

			void 				DeleteBuffers();

	static 	status_t 			FtpRun(void* data);

			void 				FtpThread();

			void 				UpdateFtpStatus(const char* status);

			status_t 			LocalSave(char* filename, BBitmap* bitmap);

			status_t 			FtpSave(char* filename);

private:
			BStringView*		fStatusLine;
			int32				fInternalID;
			BMediaAddOn*		fAddOn;

			thread_id			fFtpThread;

			bool				fConnectionActive;
			media_input			fIn;
			media_destination	fDestination;
			bigtime_t			fMyLatency;

			BWindow*			fWindow;
			BView*				fView;
			BBitmap*			fBitmap[3];
			bool				fOurBuffers;
			BBufferGroup*		fBuffers;
			BBuffer*			fBufferMap[3];

			BBitmap*			fFtpBitmap;
			volatile bool		fTimeToFtp;
			volatile bool		fFtpComplete;

			bigtime_t			fRate;
			uint32				fImageFormat;
			int32				fTranslator;
			int32				fUploadClient;
			bool				fPassiveFtp;
			char				fFileNameText[64];
			char				fServerText[64];
			char				fLoginText[64];
			char				fPasswordText[64];
			char				fDirectoryText[64];
};

#endif	// VIDEO_CONSUMER_H
