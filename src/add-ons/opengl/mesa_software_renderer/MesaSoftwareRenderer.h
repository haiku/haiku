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
#include "main/version.h"
#define HAIKU_MESA_VER (MESA_MAJOR * 100) + MESA_MINOR

#include "context.h"

#if HAIKU_MESA_VER >= 712
#include "swrast/s_chan.h"
#endif


#if HAIKU_MESA_VER <= 711
#define gl_context GLcontext
#define gl_config GLvisual
#endif


struct msr_renderbuffer {
	struct gl_renderbuffer base;
	uint32 size;
	bool active;
};


static INLINE struct msr_renderbuffer*
msr_renderbuffer(struct gl_renderbuffer* rb)
{
	return (struct msr_renderbuffer*)rb;
}


struct msr_framebuffer {
	struct gl_framebuffer base;
	uint32 width;
	uint32 height;
};


static INLINE struct msr_framebuffer*
msr_framebuffer(struct gl_framebuffer* rb)
{
	return (struct msr_framebuffer*)rb;
}


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

				GLvoid**		GetRows() { return fRowAddr; }

		virtual	void			EnableDirectMode(bool enabled);
		virtual	void			DirectConnected(direct_buffer_info* info);

private:
		static	void			_Error(gl_context* ctx);
		static	const GLubyte*	_GetString(gl_context* ctx, GLenum name);
		static	void			_Viewport(gl_context* ctx, GLint x, GLint y,
									GLsizei w, GLsizei h);
		static	void			_UpdateState(gl_context* ctx, GLuint newState);
		static	void 			_ClearFront(gl_context* ctx);
		static	GLboolean		_FrontRenderbufferStorage(gl_context* ctx,
									struct gl_renderbuffer* render,
									GLenum internalFormat,
									GLuint width, GLuint height);
		static	GLboolean		_BackRenderbufferStorage(gl_context* ctx,
									struct gl_renderbuffer* render,
									GLenum internalFormat,
									GLuint width, GLuint height);
		static void				_Flush(gl_context *ctx);
		status_t				_SetupRenderBuffer(struct msr_renderbuffer*
									buffer, color_space colorSpace);
		static void				_DeleteBackBuffer(struct gl_renderbuffer* rb);

		void					_AllocateBitmap();
		void					_CopyToDirect();

		BBitmap*				fBitmap;
		bool					fDirectModeEnabled;
		direct_buffer_info*		fInfo;
		BLocker					fInfoLocker;
		ulong					fOptions;

		gl_context*				fContext;
		gl_config*				fVisual;
		struct msr_framebuffer*	fFrameBuffer;
		struct msr_renderbuffer*	fFrontRenderBuffer;
		struct msr_renderbuffer*	fBackRenderBuffer;

		GLchan 					fClearColor[4];	// buffer clear color
		GLuint 					fClearIndex;	// buffer clear color index
		GLuint 					fWidth, fNewWidth;
		GLuint					fHeight, fNewHeight;
		color_space				fColorSpace;

		GLvoid*					fRowAddr[MAX_HEIGHT];
			// address of first pixel in each image row
};

#endif	// MESASOFTWARERENDERER_H

