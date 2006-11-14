/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
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
	static void	GetBufferSize(GLframebuffer * framebuffer, GLuint *width,
                             GLuint *height);
	static void 	SetBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                             GLenum mode);
	static void 	ClearFront(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                          GLint width, GLint height);
	static void 	ClearBack(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                         GLint width, GLint height);
	static void 	ClearIndex(GLcontext *ctx, GLuint index);
	static void 	ClearColor(GLcontext *ctx, const GLfloat color[4]);
	static void 	Clear(GLcontext *ctx, GLbitfield mask,
                               GLboolean all, GLint x, GLint y,
                               GLint width, GLint height);

	// Front-buffer functions
   static void 		WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);
   static void 		WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);
   static void 		WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                      GLint x, GLint y,
                                      const GLchan color[4],
                                      const GLubyte mask[]);
   static void 		WriteRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    CONST GLubyte rgba[][4],
                                    const GLubyte mask[]);
   static void 		WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                        const GLint x[], const GLint y[],
                                        const GLchan color[4],
                                        const GLubyte mask[]);
   static void 		WriteCI32SpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  const GLuint index[], const GLubyte mask[]);
   static void 		WriteCI8SpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLubyte index[], const GLubyte mask[]);
   static void 		WriteMonoCISpanFront(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[]);
   static void 		WriteCI32PixelsFront(const GLcontext *ctx,
                                    GLuint n, const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[]);
   static void 		WriteMonoCIPixelsFront(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[]);
   static void 		ReadCI32SpanFront(const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[]);
   static void 		ReadRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 GLubyte rgba[][4]);
   static void 		ReadCI32PixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[]);
   static void 		ReadRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[]);

   // Back buffer functions
   static void 		WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);
   static void 		WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);
   static void 		WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[]);
   static void 		WriteRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[]);
   static void 		WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[]);
   static void 		WriteCI32SpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[]);
   static void 		WriteCI8SpanBack(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[]);
   static void 		WriteMonoCISpanBack(const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y, GLuint colorIndex,
                                   const GLubyte mask[]);
   static void 		WriteCI32PixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[]);
   static void 		WriteMonoCIPixelsBack(const GLcontext *ctx,
                                     GLuint n, const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[]);
   static void 		ReadCI32SpanBack(const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[]);
   static void 		ReadRGBASpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                GLubyte rgba[][4]);
   static void 		ReadCI32PixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLuint indx[], const GLubyte mask[]);
   static void 		ReadRGBAPixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[]);
	
	BBitmap		*fBitmap;

	GLcontext 	*fContext;
	GLvisual	*fVisual;
	GLframebuffer 	*fFrameBuffer;

	GLchan 		fClearColor[4];  // buffer clear color
	GLuint 		fClearIndex;      // buffer clear color index
	GLint		fBottom;	// used for flipping Y coords
	GLuint 		fWidth;
	GLuint		fHeight;
};

#endif	// MESASOFTWARERENDERER_H

