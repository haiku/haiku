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

struct msr_renderbuffer {
	struct gl_renderbuffer Base;
	uint32 Size;
};


static INLINE struct msr_renderbuffer*
msr_renderbuffer(struct gl_renderbuffer* rb)
{
	return (struct msr_renderbuffer*)rb;
}


struct msr_framebuffer {
	struct gl_framebuffer Base;
	uint32 Width;
	uint32 Height;
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
		static	void			_Error(GLcontext* ctx);
		static	const GLubyte*	_GetString(GLcontext* ctx, GLenum name);
		static	void			_Viewport(GLcontext* ctx, GLint x, GLint y,
									GLsizei w, GLsizei h);
		static	void			_UpdateState(GLcontext* ctx, GLuint newState);
		static	void 			_ClearFront(GLcontext* ctx);
		static	GLboolean		_FrontRenderbufferStorage(GLcontext* ctx,
									struct gl_renderbuffer* render,
									GLenum internalFormat,
									GLuint width, GLuint height);
		static	GLboolean		_BackRenderbufferStorage(GLcontext* ctx,
									struct gl_renderbuffer* render,
									GLenum internalFormat,
									GLuint width, GLuint height);
		static void				_Flush(GLcontext *ctx);
		void					_SetSpanFuncs(struct msr_renderbuffer* buffer,
									color_space colorSpace);
		static void				_DeleteBackBuffer(struct gl_renderbuffer* rb);

		void					_AllocateBitmap();
		void					_CopyToDirect();

		BBitmap*				fBitmap;
		bool					fDirectModeEnabled;
		direct_buffer_info*		fInfo;
		BLocker					fInfoLocker;
		ulong					fOptions;

		GLcontext*				fContext;
		GLvisual*				fVisual;
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

