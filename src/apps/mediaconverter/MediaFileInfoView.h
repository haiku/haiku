// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#ifndef MEDIA_FILE_INFO_VIEW_H
#define MEDIA_FILE_INFO_VIEW_H


#include <Entry.h>
#include <View.h>

#include "MediaFileInfo.h"


class BMediaFile;
class BString;

class MediaFileInfoView : public BView {
	public:
								MediaFileInfoView();
	virtual						~MediaFileInfoView();

	virtual BSize				MinSize();
	virtual BSize				MaxSize();
	virtual BSize				PreferredSize();
	virtual	BAlignment			LayoutAlignment();
	virtual	void				InvalidateLayout(bool children = false);

	virtual	void				SetFont(const BFont* font,
									uint32 mask = B_FONT_ALL);

	protected:
	virtual void				Draw(BRect updateRect);
	virtual void				AttachedToWindow();

	public:
			void				Update(BMediaFile* file, entry_ref* ref);
			bigtime_t			Duration() const
									{ return fInfo.useconds; }

	private:
			void				_ValidateMinMax();
			float				_MaxLineWidth(BString* strings,
									int32 stringCount, const BFont& font);
			float				_LineHeight();
			void				_SetFontFace(uint16 face);

			bool				fMinMaxValid;
			BSize				fMinSize;
			float				fMaxLabelWidth;
			float				fNoFileLabelWidth;
			float				fLineHeight;
			entry_ref			fRef;
			BMediaFile*			fMediaFile;
			MediaFileInfo		fInfo;
};

#endif // MEDIA_FILE_INFO_VIEW_H
