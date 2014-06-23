/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DOWNLOAD_PROGRESS_VIEW_H
#define DOWNLOAD_PROGRESS_VIEW_H


#include <GroupView.h>
#include <Path.h>
#include <String.h>

class BEntry;
class BStatusBar;
class BStringView;
class BWebDownload;
class IconView;
class SmallButton;


enum {
	SAVE_SETTINGS = 'svst'
};


class DownloadProgressView : public BGroupView {
public:
								DownloadProgressView(BWebDownload* download);
								DownloadProgressView(const BMessage* archive);

			bool				Init(BMessage* archive = NULL);

			status_t			SaveSettings(BMessage* archive);
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();

	virtual	void				Draw(BRect updateRect);

	virtual	void				MessageReceived(BMessage* message);

			void				ShowContextMenu(BPoint screenWhere);

			BWebDownload*		Download() const;
			const BString&		URL() const;
			bool				IsMissing() const;
			bool				IsFinished() const;

			void				DownloadFinished();
			void				CancelDownload();

	static	void				SpeedVersusEstimatedFinishTogglePulse();

private:
			void				_UpdateStatus(off_t currentSize,
									off_t expectedSize);
			void				_UpdateStatusText();
			void				_StartNodeMonitor(const BEntry& entry);
			void				_StopNodeMonitor();

private:
			IconView*			fIconView;
			BStatusBar*			fStatusBar;
			BStringView*		fInfoView;
			SmallButton*		fTopButton;
			SmallButton*		fBottomButton;
			BWebDownload*		fDownload;
			BString				fURL;
			BPath				fPath;

			off_t				fCurrentSize;
			off_t				fExpectedSize;
			off_t				fLastSpeedReferenceSize;
			off_t				fEstimatedFinishReferenceSize;
			bigtime_t			fLastUpdateTime;
			bigtime_t			fLastSpeedReferenceTime;
			bigtime_t			fProcessStartTime;
			bigtime_t			fLastSpeedUpdateTime;
			bigtime_t			fEstimatedFinishReferenceTime;
	static	const size_t		kBytesPerSecondSlots = 10;
			size_t				fCurrentBytesPerSecondSlot;
			double				fBytesPerSecondSlot[kBytesPerSecondSlots];
			double				fBytesPerSecond;

	static	bigtime_t			sLastEstimatedFinishSpeedToggleTime;
	static	bool				sShowSpeed;
};

#endif // DOWNLOAD_PROGRESS_VIEW_H
