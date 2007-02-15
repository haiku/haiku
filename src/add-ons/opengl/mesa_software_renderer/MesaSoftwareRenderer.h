/*
 * Copyright 2006-2007, Haiku. All rights reserved.
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

class MesaSoftwareRenderer : public BGLRenderer
{
public:
						MesaSoftwareRenderer(BGLView *view, ulong bgl_options, BGLDispatcher *dispatcher);
	virtual				~MesaSoftwareRenderer();
	
	virtual void		LockGL();
	virtual void 		UnlockGL();

	virtual	void 		SwapBuffers(bool VSync = false);
	virtual	void		Draw(BRect updateRect);
	virtual status_t    CopyPixelsOut(BPoint source, BBitmap *dest);
	virtual status_t    CopyPixelsIn(BBitmap *source, BPoint dest);

private:
	static void	Error(GLcontext *ctx);
	static const GLubyte *	GetString(GLcontext *ctx, GLenum name);
	static void	Viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
	static void	UpdateState(GLcontext *ctx, GLuint new_state);
	static void 	ClearFront(GLcontext *ctx);
	static void 	ClearBack(GLcontext *ctx);
	static void 	ClearIndex(GLcontext *ctx, GLuint index);
	static void 	ClearColor(GLcontext *ctx, const GLfloat color[4]);
	static void 	Clear(GLcontext *ctx, GLbitfield mask);
	static GLboolean RenderbufferStorage(GLcontext *ctx, struct gl_renderbuffer *render, 
			GLenum internalFormat, GLuint width, GLuint height );

	// Buffer functions
   static void 		WriteRGBASpan(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                  GLint x, GLint y,
                                  const void *values,
                                  const GLubyte mask[]);
   static void 		WriteRGBSpan(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                 GLint x, GLint y,
                                 const void *values,
                                 const GLubyte mask[]);
   static void 		WriteMonoRGBASpan(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                      GLint x, GLint y,
                                      const void *values,
                                      const GLubyte mask[]);
   static void 		WriteRGBAPixels(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                    const GLint x[], const GLint y[],
                                    const void *values,
                                    const GLubyte mask[]);
   static void 		WriteMonoRGBAPixels(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                        const GLint x[], const GLint y[],
                                        const void *values,
                                        const GLubyte mask[]);
   static void 		WriteCI32Span(const GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                  GLint x, GLint y,
                                  const GLuint index[], const GLubyte mask[]);
   static void 		WriteCI8Span(const GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                 GLint x, GLint y,
                                 const GLubyte index[], const GLubyte mask[]);
   static void 		WriteMonoCISpan(const GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[]);
   static void 		WriteCI32Pixels(const GLcontext *ctx, struct gl_renderbuffer *rb, 
                                    GLuint n, const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[]);
   static void 		WriteMonoCIPixels(const GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[]);
   static void 		ReadCI32Span(const GLcontext *ctx, struct gl_renderbuffer *rb, 
                                 GLuint n, GLint x, GLint y, GLuint index[]);
   static void 		ReadRGBASpan(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                                 GLint x, GLint y,
                                 void *values);
   static void 		ReadCI32Pixels(const GLcontext *ctx, struct gl_renderbuffer *rb, 
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[]);
   static void 		ReadRGBAPixels(GLcontext *ctx, struct gl_renderbuffer *rb, 
                                   GLuint n, const GLint x[], const GLint y[],
                                   void *values);

	
	BBitmap		*fBitmap;

	GLcontext 	*fContext;
	GLvisual	*fVisual;
	GLframebuffer 	*fFrameBuffer;
	struct gl_renderbuffer *fRenderBuffer;

	GLchan 		fClearColor[4];  // buffer clear color
	GLuint 		fClearIndex;      // buffer clear color index
	GLint		fBottom;	// used for flipping Y coords
	GLuint 		fWidth;
	GLuint		fHeight;
};

#endif	// MESASOFTWARERENDERER_H

