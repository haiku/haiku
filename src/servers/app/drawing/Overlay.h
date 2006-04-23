/*
 * Copyright 2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef OVERLAY_H
#define OVERLAY_H


#include "RGBColor.h"

#include <video_overlay.h>

//#include <GraphicsDefs.h>
//#include <Rect.h>
//#include <OS.h>


class HWInterface;
struct overlay_client_data;


class Overlay {
	public:
		Overlay(HWInterface& interface);
		~Overlay();

		status_t InitCheck() const;

		void SetOverlayData(const overlay_buffer* overlayBuffer,
			overlay_token token, overlay_client_data* clientData);
		void TakeOverToken(Overlay* other);

		const overlay_buffer* OverlayBuffer() const;
		overlay_client_data* ClientData() const;
		overlay_token OverlayToken() const;

		sem_id Semaphore() const
			{ return fSemaphore; }

		const RGBColor& Color() const
			{ return fColor; }

		void SetVisible(bool visible)
			{ fVisible = visible; }
		bool IsVisible() const
			{ return fVisible; }

		void SetView(const BRect& source, const BRect& destination);
		const BRect& Source() const
			{ return fSource; }
		const BRect& Destination() const
			{ return fDestination; }

		void Show();
		void Hide();

	private:
		HWInterface&			fHWInterface;
		const overlay_buffer*	fOverlayBuffer;
		overlay_client_data*	fClientData;
		overlay_token			fOverlayToken;
		sem_id					fSemaphore;
		RGBColor				fColor;
		BRect					fSource;
		BRect					fDestination;
		bool					fVisible;
};

#endif	// OVERLAY_H
