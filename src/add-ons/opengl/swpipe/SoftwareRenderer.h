/*
 * Copyright 2006-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 * 		Philippe Houdoin, philippe.houdoin@free.fr
 * 		Artur Wyszynski, harakash@gmail.com
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef SOFTWARERENDERER_H
#define SOFTWARERENDERER_H


#include "GLRenderer.h"
#include "GalliumContext.h"


class SoftwareRenderer : public BGLRenderer {
public:
								SoftwareRenderer(BGLView *view,
									ulong bgl_options,
									BGLDispatcher *dispatcher);
	virtual						~SoftwareRenderer();

	virtual	void				LockGL();
	virtual	void				UnlockGL();

	virtual	void				SwapBuffers(bool vsync = false);
	virtual	void				Draw(BRect updateRect);
	virtual	status_t			CopyPixelsOut(BPoint source, BBitmap *dest);
	virtual	status_t			CopyPixelsIn(BBitmap *source, BPoint dest);
	virtual	void				FrameResized(float width, float height);

	virtual	void				EnableDirectMode(bool enabled);
	virtual	void				DirectConnected(direct_buffer_info *info);

private:

			void				_AllocateBitmap();

			GalliumContext*		fContextObj;
			BBitmap*			fBitmap;
			context_id			fContextID;

			bool				fDirectModeEnabled;
			direct_buffer_info*	fInfo;
			BLocker				fInfoLocker;
			ulong				fOptions;			
			GLuint				fWidth;
			GLuint				fHeight;
			GLuint				fNewWidth;
			GLuint				fNewHeight;
			color_space			fColorSpace;
};

#endif	// SOFTPIPERENDERER_H
