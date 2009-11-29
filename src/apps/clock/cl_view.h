/*
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 */
#ifndef	_CL_VIEW_H_
#define _CL_VIEW_H_


#include <View.h>


class BBitmap;
class BMessage;


class TOffscreenView : public BView {
	public:
					TOffscreenView(BRect frame, const char *name, short mRadius,
						short hRadius, short offset, long face, bool show); 
		virtual		~TOffscreenView();
		
		void		DrawX();
		void		NextFace();

		short		fHours;
		short		fMinutes;
		short		fSeconds;
		
		short		fOffset;
		short		fHoursRadius;
		short		fMinutesRadius;

		short		fFace;
		bool		fShowSeconds;

	private:
		BBitmap		*fInner;
		BBitmap		*fCenter;
		BBitmap		*fClockFace[9];

		BPoint		fHourPoints[60];
		BPoint		fMinutePoints[60];
};


class TOnscreenView : public BView {
	public:
							TOnscreenView(BRect frame, const char *name,
								short mRadius, short hRadius, short offset); 
							TOnscreenView(BMessage *data);
		virtual				~TOnscreenView();

		static BArchivable	*Instantiate(BMessage *data);
		virtual	status_t	Archive(BMessage *data, bool deep = true) const;
		void				InitObject(BRect frame, short mRadius, short hRadius,
								short offset, long face, bool show);

		virtual void		Pulse();
		virtual	void		Draw(BRect updateRect);
		virtual void		MouseDown(BPoint point);
		virtual	void		MessageReceived(BMessage *msg);

		short				ReturnFace();
		void				UseFace(short face);

		short				ReturnSeconds();
		void				ShowSecs(bool secs);

	private:
		BBitmap				*fOffscreen;
		TOffscreenView		*fOffscreenView;
};

#endif

