//	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
//	SMS
//	VideoConsumer.cpp


#include "FileUploadClient.h"
#include "FtpClient.h"
#include "SftpClient.h"
#include "VideoConsumer.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <Application.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <Catalog.h>
#include <Locale.h>
#include <MediaRoster.h>
#include <NodeInfo.h>
#include <scheduler.h>
#include <StringView.h>
#include <TimeSource.h>
#include <View.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "VideoConsumer.cpp"

#define M1 ((double)1000000.0)
#define JITTER		20000

#define	FUNCTION	printf
#define ERROR		printf
#define PROGRESS	printf
#define LOOP		printf


static status_t SetFileType(BFile* file,  int32 translator, uint32 type);

const media_raw_video_format vid_format = {29.97, 1, 0, 239,
	B_VIDEO_TOP_LEFT_RIGHT, 1, 1, {B_RGB16, 320, 240, 320 * 4, 0, 0}};


VideoConsumer::VideoConsumer(const char* name, BView* view,
	BStringView* statusLine,
	BMediaAddOn* addon, const uint32 internalId)
	: BMediaNode(name),
	BMediaEventLooper(),
	BBufferConsumer(B_MEDIA_RAW_VIDEO),
	fStatusLine(statusLine),
	fInternalID(internalId),
	fAddOn(addon),
	fConnectionActive(false),
	fMyLatency(20000),
	fWindow(NULL),
	fView(view),
	fOurBuffers(false),
	fBuffers(NULL),
	fTimeToFtp(false),
	fFtpComplete(true),
	fRate(1000000),
	fImageFormat(0),
	fTranslator(0),
	fUploadClient(0),
	fPassiveFtp(true)
{
	FUNCTION("VideoConsumer::VideoConsumer\n");

	AddNodeKind(B_PHYSICAL_OUTPUT);
	SetEventLatency(0);
	fWindow = fView->Window();

	for (uint32 j = 0; j < 3; j++) {
		fBitmap[j] = NULL;
		fBufferMap[j] = 0;
	}

	strcpy(fFileNameText, "");
	strcpy(fServerText, "");
	strcpy(fLoginText, "");
	strcpy(fPasswordText, "");
	strcpy(fDirectoryText, "");

	SetPriority(B_DISPLAY_PRIORITY);
}


VideoConsumer::~VideoConsumer()
{
	FUNCTION("VideoConsumer::~VideoConsumer\n");

	Quit();

	if (fWindow) {
		printf(B_TRANSLATE("Locking the window\n"));
		if (fWindow->Lock()) {
			printf(B_TRANSLATE("Closing the window\n"));
			fWindow->Close();
			fWindow = 0;
		}
	}

	// clean up ftp thread
	// wait up to 30 seconds if ftp is in progress
	int32 count = 0;
	while (!fFtpComplete && count < 30) {
		snooze(1000000);
		count++;
	}

	if (count == 30)
		kill_thread(fFtpThread);

	DeleteBuffers();

}

/********************************
	From BMediaNode
********************************/


BMediaAddOn*
VideoConsumer::AddOn(long* cookie) const
{
	FUNCTION("VideoConsumer::AddOn\n");
	// do the right thing if we're ever used with an add-on
	*cookie = fInternalID;
	return fAddOn;
}


// This implementation is required to get around a bug in
// the ppc compiler.

void
VideoConsumer::Start(bigtime_t performanceTime)
{
	BMediaEventLooper::Start(performanceTime);
}


void
VideoConsumer::Stop(bigtime_t performanceTime, bool immediate)
{
	BMediaEventLooper::Stop(performanceTime, immediate);
}


void
VideoConsumer::Seek(bigtime_t mediaTime, bigtime_t performanceTime)
{
	BMediaEventLooper::Seek(mediaTime, performanceTime);
}


void
VideoConsumer::TimeWarp(bigtime_t atRealTime, bigtime_t toPerformanceTime)
{
	BMediaEventLooper::TimeWarp(atRealTime, toPerformanceTime);
}


status_t
VideoConsumer::DeleteHook(BMediaNode* node)
{
	return BMediaEventLooper::DeleteHook(node);
}


void
VideoConsumer::NodeRegistered()
{
	FUNCTION("VideoConsumer::NodeRegistered\n");
	fIn.destination.port = ControlPort();
	fIn.destination.id = 0;
	fIn.source = media_source::null;
	fIn.format.type = B_MEDIA_RAW_VIDEO;
	fIn.format.u.raw_video = vid_format;

	Run();
}


status_t
VideoConsumer::RequestCompleted(const media_request_info& info)
{
	FUNCTION("VideoConsumer::RequestCompleted\n");
	switch (info.what) {
		case media_request_info::B_SET_OUTPUT_BUFFERS_FOR:
			if (info.status != B_OK)
					ERROR("VideoConsumer::RequestCompleted: Not using our buffers!\n");
			break;

		default:
			ERROR("VideoConsumer::RequestCompleted: Invalid argument\n");
			break;
	}
	return B_OK;
}


status_t
VideoConsumer::HandleMessage(int32 message, const void* data, size_t size)
{
	//FUNCTION("VideoConsumer::HandleMessage\n");
	ftp_msg_info* info = (ftp_msg_info*)data;
	status_t status = B_OK;

	switch (message) {
		case FTP_INFO:
			PROGRESS("VideoConsumer::HandleMessage - FTP_INFO message\n");
			fRate = info->rate;
			fImageFormat = info->imageFormat;
			fTranslator = info->translator;
			fPassiveFtp = info->passiveFtp;
			fUploadClient = info->uploadClient;
			strcpy(fFileNameText, info->fileNameText);
			strcpy(fServerText, info->serverText);
			strcpy(fLoginText, info->loginText);
			strcpy(fPasswordText, info->passwordText);
			strcpy(fDirectoryText, info->directoryText);
			// remove old user events
			EventQueue()->FlushEvents(TimeSource()->Now(), BTimedEventQueue::B_ALWAYS,
				true, BTimedEventQueue::B_USER_EVENT);
			if (fRate != B_INFINITE_TIMEOUT) {
				// if rate is not "Never," push an event
				// to restart captures 5 seconds from now
				media_timed_event event(TimeSource()->Now() + 5000000,
					BTimedEventQueue::B_USER_EVENT);
				EventQueue()->AddEvent(event);
			}
			break;
	}

	return status;
}


void
VideoConsumer::BufferReceived(BBuffer* buffer)
{
	LOOP("VideoConsumer::Buffer #%ld received, start_time %Ld\n", buffer->ID(), buffer->Header()->start_time);

	if (RunState() == B_STOPPED) {
		buffer->Recycle();
		return;
	}

	media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
		buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
	EventQueue()->AddEvent(event);
}


void
VideoConsumer::ProducerDataStatus(const media_destination& forWhom, int32 status,
	bigtime_t atMediaTime)
{
	FUNCTION("VideoConsumer::ProducerDataStatus\n");

	if (forWhom != fIn.destination)
		return;
}


status_t
VideoConsumer::CreateBuffers(const media_format& withFormat)
{
	FUNCTION("VideoConsumer::CreateBuffers\n");

	DeleteBuffers();
		// delete any old buffers

	status_t status = B_OK;

	// create a buffer group
	uint32 xSize = withFormat.u.raw_video.display.line_width;
	uint32 ySize = withFormat.u.raw_video.display.line_count;
	color_space colorspace = withFormat.u.raw_video.display.format;
	PROGRESS("VideoConsumer::CreateBuffers - Colorspace = %d\n", colorspace);

	fBuffers = new BBufferGroup();
	status = fBuffers->InitCheck();
	if (status != B_OK) {
		ERROR("VideoConsumer::CreateBuffers - ERROR CREATING BUFFER GROUP\n");
		return status;
	}
	// and attach the  bitmaps to the buffer group
	for (uint32 j = 0; j < 3; j++) {
		fBitmap[j] = new BBitmap(BRect(0, 0, (xSize - 1), (ySize - 1)), colorspace,
			false, true);
		if (fBitmap[j]->IsValid()) {
			buffer_clone_info info;
			if ((info.area = area_for(fBitmap[j]->Bits())) == B_ERROR)
				ERROR("VideoConsumer::CreateBuffers - ERROR IN AREA_FOR\n");
			info.offset = 0;
			info.size = (size_t)fBitmap[j]->BitsLength();
			info.flags = j;
			info.buffer = 0;

			if ((status = fBuffers->AddBuffer(info)) != B_OK) {
				ERROR("VideoConsumer::CreateBuffers - ERROR ADDING BUFFER TO GROUP\n");
				return status;
			}
			else
				PROGRESS("VideoConsumer::CreateBuffers - SUCCESSFUL ADD BUFFER TO GROUP\n");
		} else {
			ERROR("VideoConsumer::CreateBuffers - ERROR CREATING VIDEO RING BUFFER: %08lx\n",
				status);
			return B_ERROR;
		}
	}

	BBuffer** buffList = new BBuffer * [3];
	for (int j = 0; j < 3; j++)
		buffList[j] = 0;

	if ((status = fBuffers->GetBufferList(3, buffList)) == B_OK)
		for (int j = 0; j < 3; j++)
			if (buffList[j] != NULL) {
				fBufferMap[j] = (uint32)buffList[j];
				PROGRESS(" j = %d buffer = %08lx\n", j, fBufferMap[j]);
			} else {
				ERROR("VideoConsumer::CreateBuffers ERROR MAPPING RING BUFFER\n");
				return B_ERROR;
			}
	else
		ERROR("VideoConsumer::CreateBuffers ERROR IN GET BUFFER LIST\n");

	fFtpBitmap = new BBitmap(BRect(0, 0, xSize - 1, ySize - 1), B_RGB32, false, false);

	FUNCTION("VideoConsumer::CreateBuffers - EXIT\n");
	return status;
}


void
VideoConsumer::DeleteBuffers()
{
	FUNCTION("VideoConsumer::DeleteBuffers\n");

	if (fBuffers) {
		delete fBuffers;
		fBuffers = NULL;

		for (uint32 j = 0; j < 3; j++)
			if (fBitmap[j]->IsValid()) {
				delete fBitmap[j];
				fBitmap[j] = NULL;
			}
	}
	FUNCTION("VideoConsumer::DeleteBuffers - EXIT\n");
}


status_t
VideoConsumer::Connected(const media_source& producer, const media_destination& where,
	const media_format& withFormat, media_input* outInput)
{
	FUNCTION("VideoConsumer::Connected\n");

	fIn.source = producer;
	fIn.format = withFormat;
	fIn.node = Node();
	sprintf(fIn.name, "Video Consumer");
	*outInput = fIn;

	uint32 userData = 0;
	int32 changeTag = 1;
	if (CreateBuffers(withFormat) == B_OK)
		BBufferConsumer::SetOutputBuffersFor(producer, fDestination,
			fBuffers, (void *)&userData, &changeTag, true);
	else {
		ERROR("VideoConsumer::Connected - COULDN'T CREATE BUFFERS\n");
		return B_ERROR;
	}

	fConnectionActive = true;

	FUNCTION("VideoConsumer::Connected - EXIT\n");
	return B_OK;
}


void
VideoConsumer::Disconnected(const media_source& producer, const media_destination& where)
{
	FUNCTION("VideoConsumer::Disconnected\n");

	if (where == fIn.destination && producer == fIn.source) {
		// disconnect the connection
		fIn.source = media_source::null;
		delete fFtpBitmap;
		fConnectionActive = false;
	}

}


status_t
VideoConsumer::AcceptFormat(const media_destination& dest, media_format* format)
{
	FUNCTION("VideoConsumer::AcceptFormat\n");

	if (dest != fIn.destination) {
		ERROR("VideoConsumer::AcceptFormat - BAD DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	if (format->type == B_MEDIA_NO_TYPE)
		format->type = B_MEDIA_RAW_VIDEO;

	if (format->type != B_MEDIA_RAW_VIDEO) {
		ERROR("VideoConsumer::AcceptFormat - BAD FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.format != B_RGB32
		&& format->u.raw_video.display.format != B_RGB16
		&& format->u.raw_video.display.format != B_RGB15
		&& format->u.raw_video.display.format != B_GRAY8
		&&
		format->u.raw_video.display.format != media_raw_video_format::wildcard.display.format) {
		ERROR("AcceptFormat - not a format we know about!\n");
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.format == media_raw_video_format::wildcard.display.format) {
		format->u.raw_video.display.format = B_RGB16;
	}

	char formatString[256];
	string_for_format(*format, formatString, 256);
	FUNCTION("VideoConsumer::AcceptFormat: %s\n", formatString);

	return B_OK;
}


status_t
VideoConsumer::GetNextInput(int32* cookie, media_input* outInput)
{
	FUNCTION("VideoConsumer::GetNextInput\n");

	// custom build a destination for this connection
	// put connection number in id

	if (*cookie < 1) {
		fIn.node = Node();
		fIn.destination.id = *cookie;
		sprintf(fIn.name, "Video Consumer");
		*outInput = fIn;
		(*cookie)++;
		return B_OK;
	} else {
		ERROR("VideoConsumer::GetNextInput - - BAD INDEX\n");
		return B_MEDIA_BAD_DESTINATION;
	}
}


void
VideoConsumer::DisposeInputCookie(int32 /*cookie*/)
{
}


status_t
VideoConsumer::GetLatencyFor(const media_destination& forWhom, bigtime_t* outLatency,
	media_node_id* out_timesource)
{
	FUNCTION("VideoConsumer::GetLatencyFor\n");

	if (forWhom != fIn.destination)
		return B_MEDIA_BAD_DESTINATION;

	*outLatency = fMyLatency;
	*out_timesource = TimeSource()->ID();
	return B_OK;
}


status_t
VideoConsumer::FormatChanged(const media_source& producer, const media_destination& consumer,
	int32 fromChangeCount, const media_format& format)
{
	FUNCTION("VideoConsumer::FormatChanged\n");

	if (consumer != fIn.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (producer != fIn.source)
		return B_MEDIA_BAD_SOURCE;

	fIn.format = format;

	return CreateBuffers(format);
}


void
VideoConsumer::HandleEvent(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	LOOP("VideoConsumer::HandleEvent\n");

	BBuffer* buffer;

	switch (event->type) {
		case BTimedEventQueue::B_START:
			PROGRESS("VideoConsumer::HandleEvent - START\n");
			break;

		case BTimedEventQueue::B_STOP:
			PROGRESS("VideoConsumer::HandleEvent - STOP\n");
			EventQueue()->FlushEvents(event->event_time, BTimedEventQueue::B_ALWAYS,
				true, BTimedEventQueue::B_HANDLE_BUFFER);
			break;

		case BTimedEventQueue::B_USER_EVENT:
			PROGRESS("VideoConsumer::HandleEvent - USER EVENT\n");
			if (RunState() == B_STARTED) {
				fTimeToFtp = true;
				PROGRESS("Pushing user event for %.4f, time now %.4f\n",
					(event->event_time + fRate) / M1, event->event_time/M1);
				media_timed_event newEvent(event->event_time + fRate,
					BTimedEventQueue::B_USER_EVENT);
				EventQueue()->AddEvent(newEvent);
			}
			break;

		case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			LOOP("VideoConsumer::HandleEvent - HANDLE BUFFER\n");
			buffer = (BBuffer *)event->pointer;
			if (RunState() == B_STARTED && fConnectionActive) {
				// see if this is one of our buffers
				uint32 index = 0;
				fOurBuffers = true;
				while (index < 3)
					if ((uint32)buffer == fBufferMap[index])
						break;
					else
						index++;

				if (index == 3) {
					// no, buffers belong to consumer
					fOurBuffers = false;
					index = 0;
				}

				if (fFtpComplete && fTimeToFtp) {
					PROGRESS("VidConsumer::HandleEvent - SPAWNING FTP THREAD\n");
					fTimeToFtp = false;
					fFtpComplete = false;
					memcpy(fFtpBitmap->Bits(), buffer->Data(), fFtpBitmap->BitsLength());
					fFtpThread = spawn_thread(FtpRun, "Video Window Ftp", B_NORMAL_PRIORITY, this);
					resume_thread(fFtpThread);
				}

				if ((RunMode() == B_OFFLINE)
					|| ((TimeSource()->Now() > (buffer->Header()->start_time - JITTER))
						&& (TimeSource()->Now() < (buffer->Header()->start_time + JITTER)))) {
					if (!fOurBuffers)
						// not our buffers, so we need to copy
						memcpy(fBitmap[index]->Bits(), buffer->Data(), fBitmap[index]->BitsLength());

					if (fWindow->Lock()) {
						uint32 flags;
						if ((fBitmap[index]->ColorSpace() == B_GRAY8) &&
							!bitmaps_support_space(fBitmap[index]->ColorSpace(), &flags)) {
							// handle mapping of GRAY8 until app server knows how
							uint32* start = (uint32*)fBitmap[index]->Bits();
							int32 size = fBitmap[index]->BitsLength();
							uint32* end = start + size / 4;
							for (uint32* p = start; p < end; p++)
								*p = (*p >> 3) & 0x1f1f1f1f;
						}

						fView->DrawBitmap(fBitmap[index], fView->Bounds());
						fWindow->Unlock();
					}
				}
				else
					PROGRESS("VidConsumer::HandleEvent - DROPPED FRAME\n");
				buffer->Recycle();
			}
			else
				buffer->Recycle();
			break;
		}

		default:
			ERROR("VideoConsumer::HandleEvent - BAD EVENT\n");
			break;
	}
}


status_t
VideoConsumer::FtpRun(void* data)
{
	FUNCTION("VideoConsumer::FtpRun\n");

	((VideoConsumer *)data)->FtpThread();

	return 0;
}


void
VideoConsumer::FtpThread()
{
	char fullPath[B_PATH_NAME_LENGTH];
	FUNCTION("VideoConsumer::FtpThread\n");
	if (fUploadClient == 2) {
		// 64 + 64 = 128 max
		snprintf(fullPath, B_PATH_NAME_LENGTH, "%s/%s", fDirectoryText, fFileNameText);
		LocalSave(fullPath, fFtpBitmap);
	} else if (LocalSave(fFileNameText, fFtpBitmap) == B_OK)
		FtpSave(fFileNameText);

#if 0
	// save a small version, too
	BBitmap* b = new BBitmap(BRect(0,0,159,119), B_RGB32, true, false);
	BView* v = new BView(BRect(0,0,159,119), "SmallView 1", 0, B_WILL_DRAW);
	b->AddChild(v);

	b->Lock();
	v->DrawBitmap(fFtpBitmap, v->Frame());
	v->Sync();
	b->Unlock();

	if (LocalSave("small.jpg", b) == B_OK)
		FtpSave("small.jpg");

	delete b;
#endif

	fFtpComplete = true;
}


void
VideoConsumer::UpdateFtpStatus(const char* status)
{
	printf("FTP STATUS: %s\n",status);
	if (fView->Window()->Lock()) {
		fStatusLine->SetText(status);
		fView->Window()->Unlock();
	}
}


status_t
VideoConsumer::LocalSave(char* filename, BBitmap* bitmap)
{
	BFile* output;

	UpdateFtpStatus(B_TRANSLATE("Capturing Image" B_UTF8_ELLIPSIS));

	/* save a local copy of the image in the requested format */
	output = new BFile();
	if (output->SetTo(filename, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE) == B_NO_ERROR) {
		BBitmapStream input(bitmap);
		status_t err = BTranslatorRoster::Default()->Translate(&input, NULL, NULL,
        	output, fImageFormat);
        if (err == B_OK) {
        	err = SetFileType(output, fTranslator, fImageFormat);
        	if (err != B_OK)
        		UpdateFtpStatus(B_TRANSLATE("Error setting type of output file"));
        }
        else
        	UpdateFtpStatus(B_TRANSLATE("Error writing output file"));

		input.DetachBitmap(&bitmap);
		output->Unset();
		delete output;
		return B_OK;
    } else {
		UpdateFtpStatus(B_TRANSLATE("Error creating output file"));
    	return B_ERROR;
    }
}


status_t
VideoConsumer::FtpSave(char* filename)
{
	FileUploadClient *ftp;

	//XXX: make that cleaner
	switch (fUploadClient) {
		case 0:
			ftp = new FtpClient;
			break;
		case 1:
			ftp = new SftpClient;
			break;
		case 2:
			return B_OK;
		default:
			fprintf(stderr, B_TRANSLATE("invalid upload client %ld\n"),
				fUploadClient);
			return EINVAL;
	}

	ftp->SetPassive(fPassiveFtp);
		// ftp the local file to our web site

	UpdateFtpStatus(B_TRANSLATE("Logging in" B_UTF8_ELLIPSIS));
	if (ftp->Connect((string)fServerText, (string)fLoginText,
		(string)fPasswordText)) {
		// connect to server
		UpdateFtpStatus(B_TRANSLATE("Connected" B_UTF8_ELLIPSIS));

		if (ftp->ChangeDir((string)fDirectoryText)) {
			// cd to the desired directory
			UpdateFtpStatus(B_TRANSLATE("Transmitting" B_UTF8_ELLIPSIS));

			if (ftp->PutFile((string)filename, (string)"temp")) {
				// send the file to the server

				ftp->Chmod((string)"temp", (string)"644");
				// make it world readable

				UpdateFtpStatus(B_TRANSLATE("Renaming" B_UTF8_ELLIPSIS));

				if (ftp->MoveFile((string)"temp", (string)filename)) {
					// change to the desired name
					uint32 time = real_time_clock();
					char s[80];
					strcpy(s, B_TRANSLATE("Last Capture: "));
					strcat(s, ctime((const long*)&time));
					s[strlen(s) - 1] = 0;
					UpdateFtpStatus(s);
					delete ftp;
					return B_OK;
				}
				else
					UpdateFtpStatus(B_TRANSLATE("Rename failed"));
			}
			else
				UpdateFtpStatus(B_TRANSLATE("File transmission failed"));
		}
		else
			UpdateFtpStatus(B_TRANSLATE("Couldn't find requested directory on "
				"server"));
	}
	else
		UpdateFtpStatus(B_TRANSLATE("Server login failed"));

	delete ftp;
	return B_ERROR;
}


status_t
SetFileType(BFile* file, int32 translator, uint32 type)
{
	translation_format* formats;
	int32 count;

	status_t err = BTranslatorRoster::Default()->GetOutputFormats(translator,
		(const translation_format **)&formats, &count);
	if (err < B_OK)
		return err;

	const char* mime = NULL;
	for (int ix = 0; ix < count; ix++) {
		if (formats[ix].type == type) {
			mime = formats[ix].MIME;
			break;
		}
	}

	if (mime == NULL) {
		/* this should not happen, but being defensive might be prudent */
		return B_ERROR;
	}

	/* use BNodeInfo to set the file type */
	BNodeInfo ninfo(file);
	return ninfo.SetType(mime);
}
