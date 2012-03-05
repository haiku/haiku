/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Dario Casalinuovo
 */
#ifndef NETWORK_STATUS_VIEW_H
#define NETWORK_STATUS_VIEW_H


#include <Notification.h>
#include <ObjectList.h>
#include <View.h>

class BMessageRunner;


enum {
	kStatusUnknown = 0,
	kStatusNoLink,
	kStatusLinkNoConfig,
	kStatusConnecting,
	kStatusReady,

	kStatusCount
};

class NetworkStatusView : public BView {
	public:
		NetworkStatusView(BRect frame, int32 resizingMode,
			bool inDeskbar = false);
		NetworkStatusView(BMessage* archive);
		virtual	~NetworkStatusView();

		static	NetworkStatusView* Instantiate(BMessage* archive);
		virtual	status_t Archive(BMessage* archive, bool deep = true) const;

		virtual	void	AttachedToWindow();
		virtual	void	DetachedFromWindow();

		virtual	void	MessageReceived(BMessage* message);
		virtual void	FrameResized(float width, float height);
		virtual	void	MouseDown(BPoint where);
		virtual	void	Draw(BRect updateRect);

	private:
		void			_AboutRequested();
		void			_Quit();
		void			_Init();
		void			_UpdateBitmaps();
		void			_ShowConfiguration(BMessage* message);
		bool			_PrepareRequest(struct ifreq& request,
							const char* name);
		int32			_DetermineInterfaceStatus(const char* name);
		void			_Update(bool force = false);
		void			_OpenNetworksPreferences();

		BObjectList<BString> fInterfaces;
		bool			fInDeskbar;
		BBitmap*		fBitmaps[kStatusCount];
		int32			fStatus;
};

#endif	// NETWORK_STATUS_VIEW_H
