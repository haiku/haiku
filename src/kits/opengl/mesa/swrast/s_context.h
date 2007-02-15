/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file swrast/s_context.h
 * \brief Software rasterization context and private types.
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

/**
 * \mainpage swrast module
 *
 * This module, software rasterization, contains the software fallback
 * routines for drawing points, lines, triangles, bitmaps and images.
 * All rendering boils down to writing spans (arrays) of pixels with
 * particular colors.  The span-writing routines must be implemented
 * by the device driver.
 */


#ifndef S_CONTEXT_H
#define S_CONTEXT_H

#include "mtypes.h"
#include "swrast.h"


/**
 * \defgroup SpanFlags SPAN_XXX-flags
 * Bitmasks to indicate which sw_span_arrays need to be computed
 * (sw_span::interpMask) or have already been filled in (sw_span::arrayMask)
 */
/*@{*/
#define SPAN_RGBA         0x001
#define SPAN_SPEC         0x002
#define SPAN_INDEX        0x004
#define SPAN_Z            0x008
#define SPAN_W            0x010
#define SPAN_FOG          0x020
#define SPAN_TEXTURE      0x040
#define SPAN_INT_TEXTURE  0x080
#define SPAN_LAMBDA       0x100
#define SPAN_COVERAGE     0x200
#define SPAN_FLAT         0x400  /**< flat shading? */
#define SPAN_XY           0x800
#define SPAN_MASK        0x1000
#define SPAN_VARYING     0x2000
/*@}*/

#if 0
/* alternate arrangement for code below */
struct arrays2 {
   union {
      GLubyte  sz1[MAX_WIDTH][4]; /* primary color */
      GLushort sz2[MAX_WIDTH][4];
      GLfloat  sz4[MAX_WIDTH][4];
   } rgba;
   union {
      GLubyte  sz1[MAX_WIDTH][4]; /* specular color and temp storage */
      GLushort sz2[MAX_WIDTH][4];
      GLfloat  sz4[MAX_WIDTH][4];
   } spec;
};
#endif


/**
 * \sw_span_arrays 
 * \brief Arrays of fragment values.
 *
 * These will either be computed from the x/xStep values above or
 * filled in by glDraw/CopyPixels, etc.
 * These arrays are separated out of sw_span to conserve memory.
 */
typedef struct sw_span_arrays {
   GLenum ChanType; /**< Color channel type, GL_UNSIGNED_BYTE, GL_FLOAT */
   union {
      struct {
         GLubyte rgba[MAX_WIDTH][4]; /**< primary color */
         GLubyte spec[MAX_WIDTH][4]; /**< specular color and temp storage */
      } sz1;
      struct {
         GLushort rgba[MAX_WIDTH][4];
         GLushort spec[MAX_WIDTH][4];
      } sz2;
      struct {
         GLfloat rgba[MAX_WIDTH][4];
         GLfloat spec[MAX_WIDTH][4];
      } sz4;
   } color;
   /** XXX these are temporary fields, pointing into above color arrays */
   GLchan (*rgba)[4];
   GLchan (*spec)[4];

   GLuint  index[MAX_WIDTH];
   GLint   x[MAX_WIDTH];  /**< X/Y used for point/line rendering only */
   GLint   y[MAX_WIDTH];  /**< X/Y used for point/line rendering only */
   GLuint  z[MAX_WIDTH];
   GLfloat fog[MAX_WIDTH];
   GLfloat texcoords[MAX_TEXTURE_COORD_UNITS][MAX_WIDTH][4];
   GLfloat lambda[MAX_TEXTURE_COORD_UNITS][MAX_WIDTH];
   GLfloat coverage[MAX_WIDTH];
   GLfloat varying[MAX_WIDTH][MAX_VARYING_VECTORS][VARYINGS_PER_VECTOR];

   /** This mask indicates which fragments are alive or culled */
   GLubyte mask[MAX_WIDTH];
} SWspanarrays;


/**
 * \SWspan
 * \brief Contains data for either a horizontal line or a set of
 * pixels that are passed through a pipeline of functions before being
 * drawn.
 *
 * The sw_span structure describes the colors, Z, fogcoord, texcoords,
 * etc for either a horizontal run or an array of independent pixels.
 * We can either specify a base/step to indicate interpolated values, or
 * fill in arrays of values.  The interpMask and arrayMask bitfields
 * indicate which are active.
 *
 * With this structure it's easy to hand-off span rasterization to
 * subroutines instead of doing it all inline in the triangle functions
 * like we used to do.
 * It also cleans up the local variable namespace a great deal.
 *
 * It would be interesting to experiment with multiprocessor rasterization
 * with this structure.  The triangle rasterizer could simply emit a
 * stream of these structures which would be consumed by one or more
 * span-processing threads which could run in parallel.
 */
typedef struct sw_span {
   GLint x, y;

   /** Only need to process pixels between start <= i < end */
   /** At this time, start is always zero. */
   GLuint start, end;

   /** This flag indicates that mask[] array is effectively filled with ones */
   GLboolean writeAll;

   /** either GL_POLYGON, GL_LINE, GL_POLYGON, GL_BITMAP */
   GLenum primitive;

   /** 0 = front-facing span, 1 = back-facing span (for two-sided stencil) */
   GLuint facing;

   /**
    * This bitmask (of  \link SpanFlags SPAN_* flags\endlink) indicates
    * which of the x/xStep variables are relevant.
    */
   GLbitfield interpMask;

   /* For horizontal spans, step is the partial derivative wrt X.
    * For lines, step is the delta from one fragment to the next.
    */
#if CHAN_TYPE == GL_FLOAT
   GLfloat red, redStep;
   GLfloat green, greenStep;
   GLfloat blue, blueStep;
   GLfloat alpha, alphaStep;
   GLfloat specRed, specRedStep;
   GLfloat specGreen, specGreenStep;
   GLfloat specBlue, specBlueStep;
#else /* CHAN_TYPE == GL_UNSIGNED_BYTE or GL_UNSIGNED_SHORT */
   GLfixed red, redStep;
   GLfixed green, greenStep;
   GLfixed blue, blueStep;
   GLfixed alpha, alphaStep;
   GLfixed specRed, specRedStep;
   GLfixed specGreen, specGreenStep;
   GLfixed specBlue, specBlueStep;
#endif
   GLfixed index, indexStep;
   GLfixed z, zStep;    /* XXX z should probably be GLuint */
   GLfloat fog, fogStep;
   GLfloat tex[MAX_TEXTURE_COORD_UNITS][4];  /* s, t, r, q */
   GLfloat texStepX[MAX_TEXTURE_COORD_UNITS][4];
   GLfloat texStepY[MAX_TEXTURE_COORD_UNITS][4];
   GLfixed intTex[2], intTexStep[2];  /* s, t only */
   GLfloat var[MAX_VARYING_VECTORS][VARYINGS_PER_VECTOR];
   GLfloat varStepX[MAX_VARYING_VECTORS][VARYINGS_PER_VECTOR];
   GLfloat varStepY[MAX_VARYING_VECTORS][VARYINGS_PER_VECTOR];

   /* partial derivatives wrt X and Y. */
   GLfloat dzdx, dzdy;
   GLfloat w, dwdx, dwdy;
   GLfloat drdx, drdy;
   GLfloat dgdx, dgdy;
   GLfloat dbdx, dbdy;
   GLfloat dadx, dady;
   GLfloat dsrdx, dsrdy;
   GLfloat dsgdx, dsgdy;
   GLfloat dsbdx, dsbdy;
   GLfloat dfogdx, dfogdy;

   /**
    * This bitmask (of \link SpanFlags SPAN_* flags\endlink) indicates
    * which of the fragment arrays in the span_arrays struct are relevant.
    */
   GLbitfield arrayMask;

   /**
    * We store the arrays of fragment values in a separate struct so
    * that we can allocate sw_span structs on the stack without using
    * a lot of memory.  The span_arrays struct is about 400KB while the
    * sw_span struct is only about 512 bytes.
    */
   SWspanarrays *array;
} SWspan;



#define INIT_SPAN(S, PRIMITIVE, END, INTERP_MASK, ARRAY_MASK)	\
do {								\
   (S).primitive = (PRIMITIVE);					\
   (S).interpMask = (INTERP_MASK);				\
   (S).arrayMask = (ARRAY_MASK);				\
   (S).start = 0;						\
   (S).end = (END);						\
   (S).facing = 0;						\
   (S).array = SWRAST_CONTEXT(ctx)->SpanArrays;			\
} while (0)


typedef void (*texture_sample_func)(GLcontext *ctx,
                                    const struct gl_texture_object *tObj,
                                    GLuint n, const GLfloat texcoords[][4],
                                    const GLfloat lambda[], GLchan rgba[][4]);

typedef void (_ASMAPIP blend_func)( GLcontext *ctx, GLuint n,
                                    const GLubyte mask[],
                                    GLvoid *src, const GLvoid *dst,
                                    GLenum chanType);

typedef void (*swrast_point_func)( GLcontext *ctx, const SWvertex *);

typedef void (*swrast_line_func)( GLcontext *ctx,
                                  const SWvertex *, const SWvertex *);

typedef void (*swrast_tri_func)( GLcontext *ctx, const SWvertex *,
                                 const SWvertex *, const SWvertex *);


typedef void (*validate_texture_image_func)(GLcontext *ctx,
                                            struct gl_texture_object *texObj,
                                            GLuint face, GLuint level);


/** \defgroup Bitmasks
 * Bitmasks to indicate which rasterization options are enabled
 * (RasterMask)
 */
/*@{*/
#define ALPHATEST_BIT		0x001	/**< Alpha-test pixels */
#define BLEND_BIT		0x002	/**< Blend pixels */
#define DEPTH_BIT		0x004	/**< Depth-test pixels */
#define FOG_BIT			0x008	/**< Fog pixels */
#define LOGIC_OP_BIT		0x010	/**< Apply logic op in software */
#define CLIP_BIT		0x020	/**< Scissor or window clip pixels */
#define STENCIL_BIT		0x040	/**< Stencil pixels */
#define MASKING_BIT		0x080	/**< Do glColorMask or glIndexMask */
#define MULTI_DRAW_BIT		0x400	/**< Write to more than one color- */
                                        /**< buffer or no buffers. */
#define OCCLUSION_BIT           0x800   /**< GL_HP_occlusion_test enabled */
#define TEXTURE_BIT		0x1000	/**< Texturing really enabled */
#define FRAGPROG_BIT            0x2000  /**< Fragment program enabled */
#define ATIFRAGSHADER_BIT       0x4000  /**< ATI Fragment shader enabled */
#define CLAMPING_BIT            0x8000  /**< Clamp colors to [0,1] */
/*@}*/

#define _SWRAST_NEW_RASTERMASK (_NEW_BUFFERS|	\
			        _NEW_SCISSOR|	\
			        _NEW_COLOR|	\
			        _NEW_DEPTH|	\
			        _NEW_FOG|	\
                                _NEW_PROGRAM|   \
			        _NEW_STENCIL|	\
			        _NEW_TEXTURE|	\
			        _NEW_VIEWPORT|	\
			        _NEW_DEPTH)


/**
 * \struct SWcontext
 * \brief  Per-context state that's private to the software rasterizer module.
 */
typedef struct
{
   /** Driver interface:
    */
   struct swrast_device_driver Driver;

   /** Configuration mechanisms to make software rasterizer match
    * characteristics of the hardware rasterizer (if present):
    */
   GLboolean AllowVertexFog;
   GLboolean AllowPixelFog;

   /** Derived values, invalidated on statechanges, updated from
    * _swrast_validate_derived():
    */
   GLbitfield _RasterMask;
   GLfloat _BackfaceSign;
   GLboolean _PreferPixelFog;    /* Compute fog blend factor per fragment? */
   GLboolean _AnyTextureCombine;
   GLboolean _FogEnabled;
   GLenum _FogMode;  /* either GL_FOG_MODE or fragment program's fog mode */

   /* Accum buffer temporaries.
    */
   GLboolean _IntegerAccumMode;	/**< Storing unscaled integers? */
   GLfloat _IntegerAccumScaler;	/**< Implicit scale factor */

   /* Working values:
    */
   GLuint StippleCounter;    /**< Line stipple counter */
   GLbitfield NewState;
   GLuint StateChanges;
   GLenum Primitive;    /* current primitive being drawn (ala glBegin) */

   void (*InvalidateState)( GLcontext *ctx, GLbitfield new_state );

   /**
    * When the NewState mask intersects these masks, we invalidate the
    * Point/Line/Triangle function pointers below.
    */
   /*@{*/
   GLbitfield InvalidatePointMask;
   GLbitfield InvalidateLineMask;
   GLbitfield InvalidateTriangleMask;
   /*@}*/

   /**
    * Device drivers plug in functions for these callbacks.
    * Will be called when the GL state change mask intersects the above masks.
    */
   /*@{*/
   void (*choose_point)( GLcontext * );
   void (*choose_line)( GLcontext * );
   void (*choose_triangle)( GLcontext * );
   /*@}*/

   /**
    * Current point, line and triangle drawing functions.
    */
   /*@{*/
   swrast_point_func Point;
   swrast_line_func Line;
   swrast_tri_func Triangle;
   /*@}*/

   /**
    * Placeholders for when separate specular (or secondary color) is
    * enabled but texturing is not.
    */
   /*@{*/
   swrast_point_func SpecPoint;
   swrast_line_func SpecLine;
   swrast_tri_func SpecTriangle;
   /*@}*/

   /**
    * Typically, we'll allocate a sw_span structure as a local variable
    * and set its 'array' pointer to point to this object.  The reason is
    * this object is big and causes problems when allocated on the stack
    * on some systems.
    */
   SWspanarrays *SpanArrays;

   /**
    * Used to buffer N GL_POINTS, instead of rendering one by one.
    */
   SWspan PointSpan;

   /** Internal hooks, kept up to date by the same mechanism as above.
    */
   blend_func BlendFunc;
   texture_sample_func TextureSample[MAX_TEXTURE_IMAGE_UNITS];

   /** Buffer for saving the sampled texture colors.
    * Needed for GL_ARB_texture_env_crossbar implementation.
    */
   GLchan *TexelBuffer;

   validate_texture_image_func ValidateTextureImage;

} SWcontext;


extern void
_swrast_validate_derived( GLcontext *ctx );


#define SWRAST_CONTEXT(ctx) ((SWcontext *)ctx->swrast_context)

#define RENDER_START(SWctx, GLctx)			\
   do {							\
      if ((SWctx)->Driver.SpanRenderStart) {		\
         (*(SWctx)->Driver.SpanRenderStart)(GLctx);	\
      }							\
   } while (0)

#define RENDER_FINISH(SWctx, GLctx)			\
   do {							\
      if ((SWctx)->Driver.SpanRenderFinish) {		\
         (*(SWctx)->Driver.SpanRenderFinish)(GLctx);	\
      }							\
   } while (0)



/**
 * Size of an RGBA pixel, in bytes, for given datatype.
 */
#define RGBA_PIXEL_SIZE(TYPE)                                     \
         ((TYPE == GL_UNSIGNED_BYTE) ? 4 * sizeof(GLubyte) :      \
          ((TYPE == GL_UNSIGNED_SHORT) ? 4 * sizeof(GLushort)     \
           : 4 * sizeof(GLfloat)))



/*
 * XXX these macros are just bandages for now in order to make
 * CHAN_BITS==32 compile cleanly.
 * These should probably go elsewhere at some point.
 */
#if CHAN_TYPE == GL_FLOAT
#define ChanToFixed(X)  (X)
#define FixedToChan(X)  (X)
#else
#define ChanToFixed(X)  IntToFixed(X)
#define FixedToChan(X)  FixedToInt(X)
#endif

#endif
