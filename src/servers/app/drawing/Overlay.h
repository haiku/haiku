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


class HWInterface;
struct overlay_client_data;


class Overlay {
	public:
		Overlay(HWInterface& interface);
		~Overlay();

		status_t InitCheck() const;

		void SetOverlayData(const overlay_buffer* overlayBuffer,
			overlay_token token, overlay_client_data* clientData);
		void SetFlags(uint32 flags);
		void TakeOverToken(Overlay* other);

		const overlay_buffer* OverlayBuffer() const;
		overlay_client_data* ClientData() const;
		overlay_token OverlayToken() const;

		void SetColorSpace(uint32 colorSpace);

		const overlay_window* OverlayWindow() const
			{ return &fWindow; }
		const overlay_view* OverlayView() const
			{ return &fView; }

		sem_id Semaphore() const
			{ return fSemaphore; }

		const RGBColor& Color() const
			{ return fColor; }

		bool IsVisible() const
			{ return fVisible; }

		void SetView(const BRect& source, const BRect& destination);

		void Show();
		void Hide();

	private:
		HWInterface&			fHWInterface;
		const overlay_buffer*	fOverlayBuffer;
		overlay_client_data*	fClientData;
		overlay_token			fOverlayToken;
		overlay_view			fView;
		overlay_window			fWindow;
		sem_id					fSemaphore;
		RGBColor				fColor;
		bool					fVisible;
};

#endif	// OVERLAY_H
