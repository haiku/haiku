// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#ifndef MEDIA_FILE_INFO_VIEW_H
#define MEDIA_FILE_INFO_VIEW_H


#include <Entry.h>
#include <View.h>


class BMediaFile;
class BString;

class MediaFileInfoView : public BView {
	public:
								MediaFileInfoView(BRect frame,
									uint32 resizingMode);
	virtual						~MediaFileInfoView();

	protected:
	virtual void				Draw(BRect updateRect);
	virtual void				AttachedToWindow();

	public:
			void				Update(BMediaFile* file, entry_ref* ref);
			bigtime_t			Duration() const
									{ return fDuration; }

	private:
			void				_GetFileInfo(BString* audioFormat,
									BString* videoFormat,
									BString* audioDetails,
									BString* videoDetails,
									BString* duration);

			entry_ref			fRef;
			BMediaFile*			fMediaFile;
			bigtime_t			fDuration;
};

#endif // MEDIA_FILE_INFO_VIEW_H
