/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FLURRY_H_
#define _FLURRY_H_


#include <DateTime.h>
#include <GLView.h>
#include <ScreenSaver.h>


struct flurry_info_t;


class FlurryView : public BGLView {
public:
									FlurryView(BRect bounds);
	virtual							~FlurryView();

			status_t				InitCheck() const;

	virtual	void					AttachedToWindow();
	virtual	void					DrawFlurryScreenSaver();
	virtual	void					FrameResized(float width, float height);

private:
			void					_SetupFlurryBaseInfo();

			double					_CurrentTime() const;
			double					_SecondsSinceStart() const;

private:
			float					fWidth;
			float					fHeight;

			double					fStartTime;
			double					fOldFrameTime;
			flurry_info_t*			fFlurryInfo_t;

			BTime					fTime;
			BDateTime				fDateTime;
};


class Flurry : public BScreenSaver {
public:
									Flurry(BMessage* archive, image_id imageId);
	virtual							~Flurry();

	virtual	status_t				InitCheck();

	virtual	status_t				StartSaver(BView* view, bool preview);
	virtual	void					StopSaver();

	virtual	void					DirectDraw(int32 frame);
	virtual	void					DirectConnected(direct_buffer_info* info);

	virtual	void					StartConfig(BView* configView);
	virtual	void					StopConfig();

	virtual	status_t				SaveState(BMessage* into) const;

private:
			FlurryView*				fFlurryView;
};


#endif	// _FLURRY_H_
