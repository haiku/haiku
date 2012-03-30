/*
 * Copyright 2006-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 * 		Philippe Houdoin, philippe.houdoin@free.fr
 *		Artur Wyszynski, harakash@gmail.com
 */
/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 */
#ifndef MESASOFTWARERENDERER_H
#define MESASOFTWARERENDERER_H


#include "GLRenderer.h"

extern "C" {
#include "context.h"
#include "main/version.h"
#include "swrast/s_chan.h"
#include "swrast/s_context.h"
}


class MesaSoftwareRenderer : public BGLRenderer {
public:
							MesaSoftwareRenderer(BGLView* view,
								ulong bgl_options,
								BGLDispatcher* dispatcher);
	virtual					~MesaSoftwareRenderer();

	virtual	void			LockGL();
	virtual	void 			UnlockGL();

	virtual	void 			SwapBuffers(bool VSync = false);
	virtual	void			Draw(BRect updateRect);
	virtual	status_t		CopyPixelsOut(BPoint source, BBitmap* dest);
	virtual	status_t		CopyPixelsIn(BBitmap* source, BPoint dest);
	virtual void			FrameResized(float width, float height);

	virtual	void			EnableDirectMode(bool enabled);
	virtual	void			DirectConnected(direct_buffer_info* info);

private:
	static	void			_Error(gl_context* ctx);
	static	const GLubyte*	_GetString(gl_context* ctx, GLenum name);
	static	void			_Viewport(gl_context* ctx, GLint x, GLint y,
								GLsizei w, GLsizei h);
			void			_CheckResize();
	static	void			_UpdateState(gl_context* ctx, GLuint newState);
	static	void 			_ClearFront(gl_context* ctx);
	static	void			_Flush(gl_context *ctx);

	struct	swrast_renderbuffer* _NewRenderBuffer(bool front);
			status_t		_SetupRenderBuffer(struct gl_renderbuffer* rb,
								color_space colorSpace);

/* Mesa callbacks */
	static	void			_RenderBufferDelete(struct gl_renderbuffer* rb);
	static	GLboolean		_RenderBufferStorage(gl_context* ctx,
								struct gl_renderbuffer* render,
								GLenum internalFormat,
								GLuint width, GLuint height);

			void			_AllocateBitmap();
			void			_CopyToDirect();

			BBitmap*		fBitmap;
			bool			fDirectModeEnabled;
			direct_buffer_info* fInfo;
			BLocker			fInfoLocker;
			ulong			fOptions;

			gl_context*		fContext;
			gl_config*		fVisual;

			struct gl_framebuffer* fFrameBuffer;
			struct swrast_renderbuffer* fFrontRenderBuffer;
			struct swrast_renderbuffer* fBackRenderBuffer;

			GLchan 			fClearColor[4];	// buffer clear color
			GLuint 			fClearIndex;	// buffer clear color index

			GLuint			fWidth;
			GLuint			fHeight;
			GLuint 			fNewWidth;
			GLuint			fNewHeight;
			color_space		fColorSpace;
};

#endif	// MESASOFTWARERENDERER_H

