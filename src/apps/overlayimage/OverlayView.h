/*
 * Copyright 1999-2010, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * OverlayImage is based on the code presented in this article:
 * http://www.haiku-os.org/documents/dev/replishow_a_replicable_image_viewer
 *
 * Authors:
 *			Seth Flexman
 *			Hartmuth Reh
 *			Humdinger		<humdingerb@gmail.com>
 */

#ifndef OVERLAY_VIEW_H
#define OVERLAY_VIEW_H

#include <stdio.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Dragger.h>
#include <Entry.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <View.h>
#include <Window.h>


class _EXPORT OverlayView;

class OverlayView : public BView {
public:
						OverlayView(BRect frame);
						OverlayView(BMessage *data);
						~OverlayView();
	virtual void 		Draw(BRect);
	virtual void 		MessageReceived(BMessage *msg);
	static 				BArchivable *Instantiate(BMessage *archive);
	virtual status_t 	Archive(BMessage *data, bool deep = true) const;
	void				OverlayAboutRequested();

private:
	BBitmap				*fBitmap;
	bool				fReplicated;
	BTextView			*fText;
};

#endif // OVERLAY_VIEW_H
