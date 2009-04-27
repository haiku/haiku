/*
 * Copyright 2006-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 * 		Philippe Houdoin, philippe.houdoin@free.fr
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
		static	void			Error(GLcontext* ctx);
		static	const GLubyte*	GetString(GLcontext* ctx, GLenum name);
		static	void			Viewport(GLcontext* ctx, GLint x, GLint y,
									GLsizei w, GLsizei h);
		static	void			UpdateState(GLcontext* ctx, GLuint newState);
		static	void 			ClearFront(GLcontext* ctx);
		static	void 			ClearIndex(GLcontext* ctx, GLuint index);
		static	void 			ClearColor(GLcontext* ctx,
									const GLfloat color[4]);
		static	void 			Clear(GLcontext* ctx, GLbitfield mask);
		static	GLboolean		RenderbufferStorage(GLcontext* ctx,
									struct gl_renderbuffer* render,
									GLenum internalFormat,
									GLuint width, GLuint height);
		static void			Flush(GLcontext *ctx);

		void				_AllocateBitmap();

		BBitmap*				fBitmap;
		bool					fDirectModeEnabled;
		direct_buffer_info*			fInfo;
		BLocker					fInfoLocker;
		ulong					fOptions;

		GLcontext*				fContext;
		GLvisual*				fVisual;
		GLframebuffer*			fFrameBuffer;
		struct gl_renderbuffer*	fRenderBuffer;

		GLchan 					fClearColor[4];	// buffer clear color
		GLuint 					fClearIndex;	// buffer clear color index
		GLuint 					fWidth, fNewWidth;
		GLuint					fHeight, fNewHeight;
		color_space				fColorSpace;

		GLvoid*					fRowAddr[MAX_HEIGHT];
			// address of first pixel in each image row
};

#endif	// MESASOFTWARERENDERER_H

