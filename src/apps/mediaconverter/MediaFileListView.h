// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#ifndef MEDIA_FILE_LIST_VIEW_H
#define MEDIA_FILE_LIST_VIEW_H


#include <Entry.h>
#include <ListView.h>


class BMediaFile;
struct entry_ref;


class MediaFileListItem : public BStringItem {
public:
				MediaFileListItem(BMediaFile* file, const entry_ref& ref);
	virtual		~MediaFileListItem();

	entry_ref	fRef;
	BMediaFile*	fMediaFile;
};


class MediaFileListView : public BListView {
public:
								MediaFileListView();
	virtual						~MediaFileListView();

protected:
	virtual void				KeyDown(const char *bytes, int32 numBytes);
	virtual void				SelectionChanged();

public:
			bool				AddMediaItem(BMediaFile* file,
									const entry_ref& ref);

			void				SetEnabled(bool enabled);
			bool				IsEnabled() const;

private:
			bool				fEnabled;
};

#endif //MEDIACONVERTER_H
