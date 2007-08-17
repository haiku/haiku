/*
 * Copyright 2006-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef OVERLAY_H
#define OVERLAY_H


#include <InterfaceDefs.h>

#include <video_overlay.h>


class HWInterface;
class ServerBitmap;
struct overlay_client_data;


class Overlay {
	public:
		Overlay(HWInterface& interface, ServerBitmap* bitmap,
			overlay_token token);
		~Overlay();

		status_t InitCheck() const;

		status_t Suspend(ServerBitmap* bitmap, bool needTemporary);
		status_t Resume(ServerBitmap* bitmap);

		void SetClientData(overlay_client_data* clientData);
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

		const rgb_color& Color() const
			{ return fColor; }

		void Configure(const BRect& source, const BRect& destination);
		void Hide();

	private:
		void _FreeBuffer();
		status_t _AllocateBuffer(ServerBitmap* bitmap);

		HWInterface&			fHWInterface;
		const overlay_buffer*	fOverlayBuffer;
		overlay_client_data*	fClientData;
		overlay_token			fOverlayToken;
		overlay_view			fView;
		overlay_window			fWindow;
		sem_id					fSemaphore;
		rgb_color				fColor;
};

#endif	// OVERLAY_H
