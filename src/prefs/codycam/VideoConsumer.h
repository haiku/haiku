//	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
//	SMS
/*	VideoConsumer.h	*/

#if !defined(VID_CONSUMER_H)
#define VID_CONSUMER_H

#include <View.h>
#include <Bitmap.h>
#include <Window.h>
#include <MediaNode.h>
#include <TranslationKit.h>
#include <BufferConsumer.h>
#include <TimedEventQueue.h>
#include <MediaEventLooper.h>

typedef struct
{
	port_id					port;
	bigtime_t				rate;
	uint32					imageFormat;
	int32					translator;
	bool					passiveFtp;
	char					fileNameText[64];
	char					serverText[64];
	char					loginText[64];
	char					passwordText[64];	
	char					directoryText[64];
} ftp_msg_info;

#define FTP_INFO			0x60000001

class VideoConsumer : 
	public BMediaEventLooper,
	public BBufferConsumer
{
public:
						VideoConsumer(
							const char * name,
							BView * view,
							BStringView	* statusLine,
							BMediaAddOn *addon,
							const uint32 internal_id);
						~VideoConsumer();
	
/*	BMediaNode */
public:
	
	virtual	BMediaAddOn	*AddOn(long *cookie) const;
	
protected:

	virtual void		Start(bigtime_t performance_time);
	virtual void		Stop(bigtime_t performance_time, bool immediate);
	virtual void		Seek(bigtime_t media_time, bigtime_t performance_time);
	virtual void		TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);

	virtual void		NodeRegistered();
	virtual	status_t 	RequestCompleted(
							const media_request_info & info);
							
	virtual	status_t 	HandleMessage(
							int32 message,
							const void * data,
							size_t size);

	virtual status_t	DeleteHook(BMediaNode * node);

/*  BMediaEventLooper */
protected:
	virtual void		HandleEvent(
							const media_timed_event *event,
							bigtime_t lateness,
							bool realTimeEvent);
/*	BBufferConsumer */
public:
	
	virtual	status_t	AcceptFormat(
							const media_destination &dest,
							media_format * format);
	virtual	status_t	GetNextInput(
							int32 * cookie,
							media_input * out_input);
							
	virtual	void		DisposeInputCookie(
							int32 cookie);
	
protected:

	virtual	void		BufferReceived(
							BBuffer * buffer);
	
private:

	virtual	void		ProducerDataStatus(
							const media_destination &for_whom,
							int32 status,
							bigtime_t at_media_time);									
	virtual	status_t	GetLatencyFor(
							const media_destination &for_whom,
							bigtime_t * out_latency,
							media_node_id * out_id);	
	virtual	status_t	Connected(
							const media_source &producer,
							const media_destination &where,
							const media_format & with_format,
							media_input * out_input);							
	virtual	void		Disconnected(
							const media_source &producer,
							const media_destination &where);							
	virtual	status_t	FormatChanged(
							const media_source & producer,
							const media_destination & consumer, 
							int32 from_change_count,
							const media_format & format);
							
/*	implementation */

public:
			status_t	CreateBuffers(
							const media_format & with_format);
							
			void		DeleteBuffers();
							
	static	status_t	FtpRun(
							void *data);
														
	void				FtpThread(
							void);
							
	void				UpdateFtpStatus(
							char *status);
	
	status_t			LocalSave(
							char *filename,
							BBitmap *bitmap);

	status_t			FtpSave(
							char *filename);

private:

	BStringView	*		mStatusLine;
	uint32				mInternalID;
	BMediaAddOn			*mAddOn;

	thread_id			mFtpThread;
	
	bool					mConnectionActive;
	media_input				mIn;
	media_destination		mDestination;
	bigtime_t				mMyLatency;

	BWindow					*mWindow;
	BView					*mView;
	BBitmap					*mBitmap[3];
	bool					mOurBuffers;
	BBufferGroup			*mBuffers;
	uint32					mBufferMap[3];	
	
	BBitmap					*mFtpBitmap;
	volatile bool			mTimeToFtp;	
	volatile bool			mFtpComplete;	

	bigtime_t				mRate;
	uint32					mImageFormat;
	int32					mTranslator;
	bool					mPassiveFtp;
	char					mFileNameText[64];
	char					mServerText[64];
	char					mLoginText[64];
	char					mPasswordText[64];	
	char					mDirectoryText[64];
};

#endif
