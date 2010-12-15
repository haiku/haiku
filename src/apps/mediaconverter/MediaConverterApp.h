// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#ifndef MEDIA_CONVERTER_APP_H
#define MEDIA_CONVERTER_APP_H


#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <MediaDefs.h>
#include <MediaFormats.h>


class BMediaFile;
class MediaConverterWindow;

class MediaConverterApp : public BApplication {
	public:
								MediaConverterApp();
	virtual						~MediaConverterApp();

	protected:

	virtual void				MessageReceived(BMessage* message);
	virtual void				ReadyToRun();
	virtual void				RefsReceived(BMessage* message);
	
	public:
			bool				IsConverting() const;
			void				StartConverting();

			void				SetStatusMessage(const char* message);

	private:
			BEntry				_CreateOutputFile(BDirectory directory,
									entry_ref* ref,
									media_file_format* outputFormat);

	static	int32				_RunConvertEntry(void* castToMediaConverterApp);
			void				_RunConvert();
			status_t			_ConvertFile(BMediaFile* inFile,
									BMediaFile* outFile,
									media_codec_info* audioCodec,
									media_codec_info* videoCodec,
									int32 audioQuality, int32 videoQuality,
									bigtime_t StartTime, bigtime_t EndTime);
	
			MediaConverterWindow* fWin;
			thread_id			fConvertThreadID;
			bool				fConverting;
	volatile bool				fCancel;
};

#endif // MEDIA_CONVERTER_APP_H
