/***********************************************************************
 *                                                                     *
 * $Id: hpgs.h 381 2007-02-20 09:06:38Z softadm $
 *                                                                     *
 * hpgs - HPGl Script, a hpgl/2 interpreter, which uses a Postscript   *
 *        API for rendering a scene and thus renders to a variety of   *
 *        devices and fileformats.                                     *
 *                                                                     *
 * (C) 2004-2006 ev-i Informationstechnologie GmbH  http://www.ev-i.at *
 *                                                                     *
 * Author: Wolfgang Glas                                               *
 *                                                                     *
 *  hpgs is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 *                                                                     *
 * hpgs is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Lesser General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Lesser General Public    *
 * License along with this library; if not, write to the               *
 * Free Software  Foundation, Inc., 59 Temple Place, Suite 330,        *
 * Boston, MA  02111-1307  USA                                         *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 * The public interfaces, which have to be known to all components.    *
 *                                                                     *
 ***********************************************************************/

#ifndef	__HPGS_H
#define	__HPGS_H

#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GNUC__
#ifdef _MSC_VER
#define __inline__ __inline
#else
#define __inline__ inline
#endif
#endif

/* All this is needed in order to set the right attributes for functions.
   This allows us to build fully compliant dlls under Windows and to
   hide functions needed for the implementation of the API in
   ELF modules. Additionally, we make use of the 'format' attribute of
   gcc in order to get the most out of -Wall.
 */
#ifdef HPGS_SHARED
# ifdef WIN32
#  ifdef __GNUC__
#   ifdef HPGS_BUILD_SLIB
#    define HPGS_API __attribute__((dllexport))
#    define HPGS_PRINTF_API(i) __attribute__((dllexport,format(printf,i,i+1)))
#    define HPGS_I18N_API __attribute__((dllexport,format_arg (1)))
#    define HPGS_I18N_N_API __attribute__((dllexport,format_arg (1),format_arg (2)))
#   else
#    define HPGS_API __attribute__((dllimport))
#    define HPGS_PRINTF_API(i) __attribute__((dllimport,format(printf,i,i+1)))
#    define HPGS_I18N_API __attribute__((dllimport,format_arg (1)))
#    define HPGS_I18N_N_API __attribute__((dllimport,format_arg (1),format_arg (2)))
#   endif
#   define HPGS_INTERNAL_PRINTF_API(i) __attribute__((format(printf,i,i+1)))
#  else
#   ifdef HPGS_BUILD_SLIB
#    define HPGS_API __declspec(dllexport)
#   else
#    define HPGS_API __declspec(dllimport)
#   endif
#  endif
# else
#  ifdef __GNUC__
#   define HPGS_PRINTF_API(i) __attribute__((format(printf,i,i+1)))
#   define HPGS_I18N_API __attribute__((format_arg (1)))
#   define HPGS_I18N_N_API __attribute__((format_arg (1),format_arg (2)))
#   define HPGS_INTERNAL_API __attribute__((visibility("hidden")))
#   define HPGS_INTERNAL_PRINTF_API(i) __attribute__((visibility("hidden"),format(printf,i,i+1)))
#  endif
# endif
#endif

#ifndef HPGS_API
#define HPGS_API
#endif
#ifndef HPGS_PRINTF_API
#define HPGS_PRINTF_API(i) HPGS_API
#endif
#ifndef HPGS_I18N_API
#define HPGS_I18N_API HPGS_API
#endif
#ifndef HPGS_I18N_N_API
#define HPGS_I18N_N_API HPGS_API
#endif
#ifndef HPGS_INTERNAL_API
#define HPGS_INTERNAL_API
#endif
#ifndef HPGS_INTERNAL_PRINTF_API
#define HPGS_INTERNAL_PRINTF_API(i) HPGS_INTERNAL_API
#endif

/* These defines are used in order to allow better doxygen integration
   of the printf API functions. */
#define HPGS_PRINTF1_API HPGS_PRINTF_API(1)
#define HPGS_PRINTF2_API HPGS_PRINTF_API(2)
#define HPGS_INTERNAL_PRINTF1_API HPGS_INTERNAL_PRINTF_API(1)
#define HPGS_INTERNAL_PRINTF2_API HPGS_INTERNAL_PRINTF_API(2)

#ifdef WIN32
#define HPGS_SIZE_T_FMT "%lu"
#else
#define HPGS_SIZE_T_FMT "%zu"
#endif

/*! \file hpgs.h

   \brief The public interfaces.

   A header file, which declares the public structures and functions
   provided by the hpgs library. The API declared in this header file
   remains stable across the whole series of release with the same
   major ans minor number.
*/

#define HPGS_STRINGIFYIFY(i) #i
#define HPGS_STRINGIFY(i) HPGS_STRINGIFYIFY(i)

#define HPGS_MAJOR_VERSION 1
#define HPGS_MINOR_VERSION 1
#define HPGS_PATCH_VERSION 0
#define HPGS_EXTRA_VERSION

// a string for displaying the version.
#define HPGS_VERSION HPGS_STRINGIFY(HPGS_MAJOR_VERSION) "." HPGS_STRINGIFY(HPGS_MINOR_VERSION) "." HPGS_STRINGIFY(HPGS_PATCH_VERSION) HPGS_STRINGIFY(HPGS_EXTRA_VERSION)

#define HPGS_ESC '\033'

#define HPGS_MAX_LABEL_SIZE 256

#define hpgs_alloca(sz) alloca(sz)

/*! @addtogroup base
 *  @{
 */
typedef int hpgs_bool;

#define HPGS_TRUE 1
#define HPGS_FALSE 0

HPGS_API int hpgs_array_safe_resize (size_t itemsz, void **pptr, size_t *psz, size_t nsz);

#define HPGS_MIN(a,b) ((a)<(b)?(a):(b))
#define HPGS_MAX(a,b) ((a)>(b)?(a):(b))

#define HPGS_INIT_ARRAY(st,type,pmemb,nmemb,szmemb,insz) \
(st->szmemb=insz,st->nmemb=0,(st->pmemb=(type*)malloc(sizeof(type)*insz))?0:-1)

#define HPGS_GROW_ARRAY_FOR_PUSH(st,type,pmemb,nmemb,szmemb) \
((st->nmemb >= st->szmemb)?hpgs_array_safe_resize(sizeof(type),(void **)(&st->pmemb),&st->szmemb,st->szmemb*2):(0))

#define HPGS_GROW_ARRAY_MIN_SIZE(st,type,pmemb,nmemb,szmemb,msz) \
((st->nmemb>=st->szmemb||st->szmemb<msz)?hpgs_array_safe_resize(sizeof(type),(void **)(&st->pmemb),&st->szmemb,HPGS_MAX(st->szmemb*2,msz)):(0))

#ifdef WIN32
#define HPGS_PATH_SEPARATOR '\\'
#else
#define HPGS_PATH_SEPARATOR '/'
#endif

HPGS_API void hpgs_init (const char *prefix);
HPGS_API const char *hpgs_get_prefix();
HPGS_API void hpgs_cleanup (void);
HPGS_API char *hpgs_share_filename(const char *rel_filename);

HPGS_API char *hpgs_vsprintf_malloc(const char *fmt, va_list ap);
HPGS_PRINTF1_API char *hpgs_sprintf_malloc(const char *fmt, ...);

HPGS_PRINTF1_API int hpgs_set_error(const char *fmt, ...);
HPGS_PRINTF1_API int hpgs_error_ctxt(const char *fmt, ...);
HPGS_API int hpgs_set_verror(const char *fmt, va_list ap);
HPGS_API int hpgs_verror_ctxt(const char *fmt, va_list ap);
HPGS_API const char *hpgs_get_error();
HPGS_API hpgs_bool hpgs_have_error();
HPGS_API void hpgs_clear_error();

HPGS_API int hpgs_next_utf8(const char **p);
HPGS_API int hpgs_utf8_strlen(const char *p, int n);

HPGS_I18N_API const char *hpgs_i18n(const char *msg);
HPGS_I18N_N_API const char *hpgs_i18n_n(const char *msg,
                                        const char *msg_plural,
                                        unsigned long n);

typedef void (*hpgs_logger_func_t) (const char *fmt, va_list ap);
HPGS_API void hpgs_set_logger(hpgs_logger_func_t func);
HPGS_PRINTF1_API void hpgs_log(const char *fmt, ...);
HPGS_API void hpgs_vlog(const char *fmt, va_list ap);

HPGS_API hpgs_bool hpgs_get_arg_value(const char *opt, const char *argv[],
                                      const char **value, int *narg);

/*! \brief A 2D point.

    This structure has a public alias \c hpgs_point and
    represents a point consisting of double precision
    x and y coordinates.
 */
typedef struct hpgs_point_st {
    double x, y;
} hpgs_point;

/*! \brief An application level RGB color.

    This structure has a public alias \c hpgs_color and
    represents an application level RGB color consisting
    of double precision r,g and b color values in the range
    from 0.0 to 1.0.
 */
typedef struct hpgs_color_st {
  double r, g, b;
} hpgs_color;

typedef struct hpgs_palette_color_st hpgs_palette_color;

/*! \brief A screen RGB color as stored in a palette.

    This structure has a public alias \c hpgs_palette_color and
    represents a screen RGB color consisting of single byte
    r,g and b color values in the range 0 to 255.
 */
struct hpgs_palette_color_st
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

typedef void (*hpgs_rop3_func_t) (unsigned char *, unsigned char, unsigned char);

HPGS_API hpgs_rop3_func_t hpgs_rop3_func(int rop3,
                                         hpgs_bool src_transparency,
                                         hpgs_bool pattern_transparency);

typedef unsigned (*hpgs_xrop3_func_t) (unsigned char, unsigned char);

HPGS_API hpgs_xrop3_func_t hpgs_xrop3_func(int rop3,
                                           hpgs_bool src_transparency,
                                           hpgs_bool pattern_transparency);

typedef struct hpgs_paint_color_st hpgs_paint_color;

/*! \brief An image RGB color with an optional palette index.

    This structure has a public alias \c hpgs_paint_color and
    represents an image RGB color with an optional palette index
    consisting of single byte
    r,g and b color values in the range 0 to 255 as well as of
    a single byte palette index.
 
    For calculating a suitable palette index for a given
    \c hpgs_image consider calling \c hpgs_image_define_color.
 */
struct hpgs_paint_color_st
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char index;
};

typedef struct hpgs_bbox_st hpgs_bbox;

/*! \brief A bounding box.

    This structure hold a bounding box used in several contexts.
 */
struct hpgs_bbox_st
{
  double llx;
  double lly;
  double urx;
  double ury;
};

HPGS_API hpgs_bool hpgs_bbox_isequal(const hpgs_bbox *bb1, const hpgs_bbox *bb2);
HPGS_API hpgs_bool hpgs_bbox_isnull (const hpgs_bbox *bb);
HPGS_API hpgs_bool hpgs_bbox_isempty (const hpgs_bbox *bb);
HPGS_API void hpgs_bbox_distance  (hpgs_point *d, const hpgs_bbox *bb1, const hpgs_bbox *bb2);
HPGS_API void hpgs_bbox_null      (hpgs_bbox *bb);
HPGS_API void hpgs_bbox_expand    (hpgs_bbox *bb1, const hpgs_bbox *bb2);
HPGS_API void hpgs_bbox_intersect (hpgs_bbox *bb1, const hpgs_bbox *bb2);
/*! Expands the bounding box \c bb in all directions by the amount of \c border. */
static void hpgs_bbox_addborder (hpgs_bbox *bb, double border);
/*! Expands the bounding box \c bb in order to comprise the smallest bounding box
   containing both \c bb and \cp. */
static void hpgs_bbox_add (hpgs_bbox *bb, const hpgs_point *p);

__inline__ void hpgs_bbox_addborder (hpgs_bbox *bb, double border)
{
  bb->llx -= border;
  bb->urx += border;
  bb->lly -= border;
  bb->ury += border;
}

__inline__ void hpgs_bbox_add (hpgs_bbox *bb, const hpgs_point *p)
{
  if (p->x < bb->llx) bb->llx = p->x;
  if (p->x > bb->urx) bb->urx = p->x;
  if (p->y < bb->lly) bb->lly = p->y;
  if (p->y > bb->ury) bb->ury = p->y;
}

typedef struct hpgs_matrix_st hpgs_matrix;

/*! \brief A transformation matrix for 2D points.

    This structure holds an affine transformation matrix consisting of a
    translation vector and a 2x2 matrix for rotation and or scaling.
 */
struct hpgs_matrix_st
{
  double dx;
  double dy;
  double mxx;
  double mxy;
  double myx;
  double myy;
};

HPGS_API void hpgs_matrix_set_identity(hpgs_matrix *m);

HPGS_API void hpgs_matrix_xform(hpgs_point *res,
                                const hpgs_matrix *m, const hpgs_point *p);
HPGS_API void hpgs_matrix_ixform(hpgs_point *res,
                                 const hpgs_point *p, const hpgs_matrix *m);
HPGS_API void hpgs_matrix_scale(hpgs_point *res,
                                const hpgs_matrix *m, const hpgs_point *p);
HPGS_API void hpgs_matrix_iscale(hpgs_point *res,
                                 const hpgs_point *p, const hpgs_matrix *m);
HPGS_API void hpgs_matrix_concat(hpgs_matrix *res,
                                 const hpgs_matrix *a, const hpgs_matrix *b);
HPGS_API void hpgs_matrix_invert(hpgs_matrix *res, const hpgs_matrix *m);

HPGS_API void hpgs_matrix_xform_bbox(hpgs_bbox *res,
                                     const hpgs_matrix *m, const hpgs_bbox *bb);
HPGS_API void hpgs_matrix_ixform_bbox(hpgs_bbox *res,
                                      const hpgs_bbox *bb, const hpgs_matrix *m);

/*! Defines the supported line cap styles. */
typedef enum {
  hpgs_cap_butt = 0,   //!< Butt line cap.
  hpgs_cap_round = 1,  //!< Round line cap.
  hpgs_cap_square = 2  //!< Square line cap.
} hpgs_line_cap;

/*! Defines the supported line join styles. */
typedef enum {
    hpgs_join_miter = 0, //!< Miter line join.
    hpgs_join_round = 1, //!< Round line join.
    hpgs_join_bevel = 2  //!< Bevel line join.
} hpgs_line_join;

// input stream
typedef struct hpgs_istream_st hpgs_istream;
typedef struct hpgs_istream_vtable_st hpgs_istream_vtable;

typedef int  (*hpgs_istream_getc_func_t)(void *);
typedef int  (*hpgs_istream_ungetc_func_t)(int ,void *);
typedef int  (*hpgs_istream_close_func_t)(void *);
typedef int  (*hpgs_istream_iseof_func_t)(void *);
typedef int  (*hpgs_istream_iserror_func_t)(void *);
typedef int  (*hpgs_istream_seek_func_t)(void *, size_t);
typedef int  (*hpgs_istream_tell_func_t)(void *, size_t*);
typedef size_t (*hpgs_istream_read_func_t)(void *, size_t, size_t, void *);
typedef int (*hpgs_istream_seekend_func_t)(void *, size_t);

/*! \brief A table of virtual function implementing \c hpgs_istream.

    This structure has a public alias \c hpgs_istream_vtable and
    represents a table of functions for a specific implementation
    of \c hpgs_istream.

    The function names are deliberately similar to the corresponding
    ANSI C functions.
 */
struct hpgs_istream_vtable_st
{
  hpgs_istream_getc_func_t    getc_func;
  hpgs_istream_ungetc_func_t  ungetc_func;
  hpgs_istream_close_func_t   close_func;
  hpgs_istream_iseof_func_t   iseof_func;
  hpgs_istream_iserror_func_t iserror_func;
  hpgs_istream_seek_func_t    seek_func;
  hpgs_istream_tell_func_t    tell_func;
  hpgs_istream_read_func_t    read_func;
  hpgs_istream_seekend_func_t seekend_func;
};

/*! \brief A virtual input stream for the HPGL reader.

    This structure has a public alias \c hpgs_istream and
    represents an abstract input stream consisting of
    a stream pointer and a pointer to a table of virtual functions
    implementing the stream.
 */
struct hpgs_istream_st
{
  hpgs_istream_vtable *vtable;

  void *stream;
};

HPGS_API hpgs_istream *hpgs_new_file_istream (const char *fn);
HPGS_API hpgs_istream *hpgs_new_mem_istream (const unsigned char *data,
                                             size_t data_size,
                                             hpgs_bool dup);


/*! The counterpart of ANSI getc for \c hpgs_istream. */
static int  hpgs_getc   (hpgs_istream *_this);
/*! The counterpart of ANSI ungetc for \c hpgs_istream. */
static int  hpgs_ungetc (int c, hpgs_istream *_this);
/*! The counterpart of ANSI fclose for \c hpgs_istream. 
    After calling this function, all allocated resources of the stream are freed. */
static int  hpgs_istream_close  (hpgs_istream *_this);
/*! The counterpart of ANSI feof for \c hpgs_istream. */
static int  hpgs_istream_iseof  (hpgs_istream *_this);
/*! The counterpart of ANSI ferror for \c hpgs_istream. */
static int  hpgs_istream_iserror(hpgs_istream *_this);
/*! The counterpart of ANSI fseek for \c hpgs_istream. */
static int hpgs_istream_seek (hpgs_istream *_this, size_t pos);
/*! The counterpart of ANSI fseek(SEEK_END) for \c hpgs_istream. */
static int hpgs_istream_seekend (hpgs_istream *_this, size_t pos);
/*! The counterpart of ANSI ftell for \c hpgs_istream. */
static int hpgs_istream_tell   (hpgs_istream *_this, size_t *pos);
/*! The counterpart of ANSI fread for \c hpgs_istream. */
static size_t hpgs_istream_read (void *ptr, size_t size, size_t nmemb, hpgs_istream *_this);

__inline__ int hpgs_getc   (hpgs_istream *_this)
{ return _this->vtable->getc_func(_this->stream); }

__inline__ int hpgs_ungetc (int c, hpgs_istream *_this)
{ return _this->vtable->ungetc_func(c,_this->stream); }

__inline__ int hpgs_istream_close (hpgs_istream *_this)
{ int ret = _this->vtable->close_func(_this->stream); free(_this); return ret; }

__inline__ int hpgs_istream_iseof (hpgs_istream *_this)
{ return _this->vtable->iseof_func(_this->stream); }

__inline__ int hpgs_istream_iserror (hpgs_istream *_this)
{ return _this->vtable->iserror_func(_this->stream); }

__inline__ int hpgs_istream_seek (hpgs_istream *_this, size_t pos)
{ return _this->vtable->seek_func(_this->stream,pos); }

__inline__ int hpgs_istream_seekend (hpgs_istream *_this, size_t pos)
{ return _this->vtable->seekend_func(_this->stream,pos); }

__inline__ int hpgs_istream_tell   (hpgs_istream *_this, size_t *pos)
{ return _this->vtable->tell_func(_this->stream,pos); }

__inline__ size_t hpgs_istream_read (void *ptr, size_t size, size_t nmemb, hpgs_istream *_this)
{ return _this->vtable->read_func(ptr,size,nmemb,_this->stream); }

// output stream
typedef struct hpgs_ostream_st hpgs_ostream;
typedef struct hpgs_ostream_vtable_st hpgs_ostream_vtable;

typedef int  (*hpgs_ostream_putc_func_t)(int, void *);
typedef size_t (*hpgs_ostream_write_func_t)(const void *, size_t, size_t, void *);
typedef int  (*hpgs_ostream_vprintf_func_t)(void *, const char *, va_list);
typedef int  (*hpgs_ostream_flush_func_t)(void *);
typedef int  (*hpgs_ostream_close_func_t)(void *);
typedef int  (*hpgs_ostream_iserror_func_t)(void *);
typedef hpgs_istream *(*hpgs_ostream_getbuf_func_t)(void *);
typedef int  (*hpgs_ostream_tell_func_t)(void *, int layer, size_t *);
typedef int  (*hpgs_ostream_seek_func_t)(void *, size_t);

/*! \brief A table of virtual function implementing \c hpgs_istream.

    This structure has a public alias \c hpgs_istream_vtable and
    represents a table of functions for a specific implementation
    of \c hpgs_istream.

    The function names are deliberately similar to the corresponding
    ANSI C functions.
 */
struct hpgs_ostream_vtable_st
{
  hpgs_ostream_putc_func_t    putc_func;
  hpgs_ostream_write_func_t   write_func;
  hpgs_ostream_vprintf_func_t vprintf_func;
  hpgs_ostream_flush_func_t   flush_func;
  hpgs_ostream_close_func_t   close_func;
  hpgs_ostream_iserror_func_t iserror_func;
  hpgs_ostream_getbuf_func_t  getbuf_func;
  hpgs_ostream_tell_func_t    tell_func;
  hpgs_ostream_seek_func_t    seek_func;
};

/*! \brief A virtual output stream for the HPGL reader.

    This structure has a public alias \c hpgs_ostream and
    represents an abstract onput stream consisting of
    a stream pointer and a pointer to a table of virtual functions
    implementing the stream.
 */
struct hpgs_ostream_st
{
  hpgs_ostream_vtable *vtable;

  void *stream;
};

HPGS_API hpgs_ostream *hpgs_new_file_ostream (const char *fn);
HPGS_API hpgs_ostream *hpgs_new_mem_ostream (size_t data_reserve);
HPGS_API hpgs_ostream *hpgs_new_z_ostream (hpgs_ostream *base, int compression, hpgs_bool take_base);

HPGS_API int hpgs_copy_streams (hpgs_ostream *out, hpgs_istream *in);

HPGS_PRINTF2_API int  hpgs_ostream_printf (hpgs_ostream *_this, const char *msg, ...);
HPGS_API int hpgs_ostream_vprintf (hpgs_ostream *_this, const char *msg, va_list ap);

/*! The counterpart of ANSI putc for \c hpgs_ostream. */
static int  hpgs_ostream_putc (int c, hpgs_ostream *_this);
/*! The counterpart of ANSI fwrite for \c hpgs_ostream. */
static size_t hpgs_ostream_write (const void *ptr, size_t size, size_t nmemb, hpgs_ostream *_this);
/*! The counterpart of ANSI fflush for \c hpgs_ostream. */
static int  hpgs_ostream_flush (hpgs_ostream *_this);
/*! The counterpart of ANSI fclose for \c hpgs_ostream. 
    After calling this function, all allocated resources of the stream are freed. */
static int  hpgs_ostream_close (hpgs_ostream *_this);
/*! The counterpart of ANSI ferror for \c hpgs_ostream. */
static int  hpgs_ostream_iserror(hpgs_ostream *_this);
/*! Get the buffer of a \c hpgs_ostream. This function should only be called
    after \c hpgs_ostream_flush. */
static hpgs_istream *hpgs_ostream_getbuf(hpgs_ostream *_this);
/*! The counterpart of ANSI ftell for \c hpgs_ostream. */
static int hpgs_ostream_tell(hpgs_ostream *_this, int layer, size_t *pos);
/*! The counterpart of ANSI fseek for \c hpgs_ostream. */
static int hpgs_ostream_seek (hpgs_ostream *_this, size_t pos);

__inline__ int hpgs_ostream_putc (int c, hpgs_ostream *_this)
{ return _this->vtable->putc_func(c,_this->stream); }

__inline__ size_t hpgs_ostream_write (const void *ptr, size_t size, size_t nmemb, hpgs_ostream *_this)
{ return _this->vtable->write_func(ptr,size,nmemb,_this->stream); }

__inline__ int hpgs_ostream_flush (hpgs_ostream *_this)
{ return _this->vtable->flush_func ?  _this->vtable->flush_func(_this->stream) : 0; }

__inline__ int hpgs_ostream_close (hpgs_ostream *_this)
{ int ret = _this->vtable->close_func(_this->stream); free(_this); return ret; }

__inline__ int hpgs_ostream_iserror (hpgs_ostream *_this)
{ return _this->vtable->iserror_func(_this->stream); }

__inline__ hpgs_istream *hpgs_ostream_getbuf (hpgs_ostream *_this)
{ return _this->vtable->getbuf_func ? _this->vtable->getbuf_func(_this->stream) : 0; }

__inline__ int hpgs_ostream_tell(hpgs_ostream *_this, int layer, size_t *pos)
{ return _this->vtable->tell_func ? _this->vtable->tell_func(_this->stream,layer,pos) : -1; }

__inline__ int hpgs_ostream_seek (hpgs_ostream *_this, size_t pos)
{ return _this->vtable->seek_func ? _this->vtable->seek_func(_this->stream,pos) : -1; }

HPGS_API int hpgs_parse_papersize(const char *str, double *pt_width, double *pt_height);
HPGS_API int hpgs_parse_length(const char *str, double *pt_length);

/*! @} */ /* end of group base */

/*! @addtogroup device
 *  @{
 */
typedef struct hpgs_device_st hpgs_device;
typedef struct hpgs_plotsize_device_st hpgs_plotsize_device;
typedef struct hpgs_eps_device_st hpgs_eps_device;
typedef struct hpgs_gs_device_st hpgs_gs_device;
typedef struct hpgs_device_vtable_st hpgs_device_vtable;
typedef struct hpgs_image_st hpgs_image;
typedef struct hpgs_image_vtable_st hpgs_image_vtable;
typedef struct hpgs_png_image_st hpgs_png_image;
typedef struct hpgs_paint_device_st hpgs_paint_device;
typedef struct hpgs_gstate_st     hpgs_gstate;
typedef struct hpgs_font_st       hpgs_font;

/*! \brief The vector graphics state.

    This structure has a public alias \c hpgs_gstate and holds
    the line attributes of a graphics state.
 */
struct hpgs_gstate_st
{
  hpgs_line_cap    line_cap;
  hpgs_line_join   line_join;
  hpgs_color       color;
  double           miterlimit;
  double           linewidth;

  int rop3;
  hpgs_bool src_transparency;
  hpgs_bool pattern_transparency;

  hpgs_color pattern_color;

  int     n_dashes;
  float  *dash_lengths;
  double  dash_offset;
};

HPGS_API hpgs_gstate *hpgs_new_gstate(void);
HPGS_API void hpgs_gstate_destroy(hpgs_gstate *gstate);
HPGS_API int  hpgs_gstate_setdash(hpgs_gstate *gstate,
                                  const float *, unsigned, double);


#define HPGS_DEVICE_CAP_RASTER        (1<<0) //!< This device is a raster device.
#define HPGS_DEVICE_CAP_ANTIALIAS     (1<<1) //!< This device supports anitaliasing.
#define HPGS_DEVICE_CAP_VECTOR        (1<<2) //!< This device is a true vector device.
#define HPGS_DEVICE_CAP_MULTIPAGE     (1<<3) //!< This device supports multiple pages.
#define HPGS_DEVICE_CAP_PAGECOLLATION (1<<4) //!< This device is able to write multiple pages to a single file.
#define HPGS_DEVICE_CAP_MULTISIZE     (1<<5) //!< This device is able to cope with distinct sizes per page.
#define HPGS_DEVICE_CAP_DRAWIMAGE     (1<<6) //!< The device may draw an image.
#define HPGS_DEVICE_CAP_NULLIMAGE     (1<<7) //!< The device accepts a null image in drawimage.
#define HPGS_DEVICE_CAP_PLOTSIZE      (1<<8) //!< This device is a plotsize device.
#define HPGS_DEVICE_CAP_ROP3          (1<<9) //!< This device supports rop3 operations.

/*! \brief A table of virtual function implementing \c hpgs_device.

    This structure has a public alias \c hpgs_device_vtable and
    represents a table of functions for a specific implementation
    of \c hpgs_device.

    The function names are deliberately similar to the corresponding
    functions of the PostScript programming language.
 */
struct hpgs_device_vtable_st {
  const char * rtti;
  int (*moveto)      (hpgs_device *_this, const hpgs_point *p);
  int (*lineto)      (hpgs_device *_this, const hpgs_point *p);
  int (*curveto)     (hpgs_device *_this, const hpgs_point *p1,
		      const hpgs_point *p2, const hpgs_point *p3 );
  int (*newpath)     (hpgs_device *_this);
  int (*closepath)   (hpgs_device *_this);
  int (*stroke)      (hpgs_device *_this);
  int (*fill)        (hpgs_device *_this, hpgs_bool winding);
  int (*clip)        (hpgs_device *_this, hpgs_bool winding);
  int (*clipsave)    (hpgs_device *_this);
  int (*cliprestore) (hpgs_device *_this);
  int (*setrgbcolor) (hpgs_device *_this, const hpgs_color *rgb);
  int (*setdash)     (hpgs_device *_this, const float *, unsigned, double);
  int (*setlinewidth)(hpgs_device *_this, double);
  int (*setlinecap)  (hpgs_device *_this, hpgs_line_cap);
  int (*setlinejoin) (hpgs_device *_this, hpgs_line_join);
  int (*setmiterlimit) (hpgs_device *_this, double l);
  int (*setrop3)     (hpgs_device *_this, int rop,
                      hpgs_bool src_transparency, hpgs_bool pattern_transparency);
  int (*setpatcol)   (hpgs_device *_this, const hpgs_color *rgb);
  int (*drawimage)   (hpgs_device *_this, const hpgs_image *img,
                      const hpgs_point *ll, const hpgs_point *lr,
                      const hpgs_point *ur);
  int (*setplotsize) (hpgs_device *_this, const hpgs_bbox *bb);
  int (*getplotsize) (hpgs_device *_this, int i, hpgs_bbox *bb);
  int (*showpage)    (hpgs_device *_this, int i);
  int (*finish)      (hpgs_device *_this);
  int (*capabilities)(hpgs_device *_this);
  void (*destroy)    (hpgs_device *_this);
};

/*! \brief A virtual vector graphics device for the HPGL reader.

    This structure has a public alias \c hpgs_device and
    represents an abstract vector drawing device consisting of
    a pointer to a table of virtual functions 
    implementing the device.

 */
struct hpgs_device_st {
  hpgs_device_vtable *vtable;
};

#define HPGS_BASE_CLASS(d) (&(d->inherited))

HPGS_API hpgs_plotsize_device *hpgs_new_plotsize_device(hpgs_bool ignore_ps,
                                                        hpgs_bool do_linewidth);

HPGS_API hpgs_eps_device *hpgs_new_eps_device(const char *filename,
                                              const hpgs_bbox *bb,
                                              hpgs_bool do_rop3  );

HPGS_API hpgs_eps_device *hpgs_new_ps_device(const char *filename,
                                             const hpgs_bbox *bb,
                                             hpgs_bool do_rop3);

typedef int (*hpgs_reader_asset_func_t)(void *, hpgs_device *,
                                        const hpgs_matrix *,
                                        const hpgs_matrix *,
                                        const hpgs_bbox *, int);

HPGS_API int hpgs_new_plugin_device( hpgs_device **device,
                                     void **page_asset_ctxt,
                                     hpgs_reader_asset_func_t *page_asset_func,
                                     void **frame_asset_ctxt,
                                     hpgs_reader_asset_func_t *frame_asset_func,
                                     const char *dev_name,
                                     const char *filename,
                                     const hpgs_bbox *bb,
                                     double xres, double yres,
                                     hpgs_bool do_rop3,
                                     int argc, const char *argv[]);

HPGS_API const char *hpgs_device_rtti(hpgs_device *_this);

/*! PostScript moveto on the device. */
static int hpgs_moveto      (hpgs_device *_this, const hpgs_point *p);
/*! PostScript lineto on the device. */
static int hpgs_lineto      (hpgs_device *_this, const hpgs_point *p);
/*! PostScript curveto on the device. */
static int hpgs_curveto     (hpgs_device *_this, const hpgs_point *p1,
				 const hpgs_point *p2, const hpgs_point *p3 );
/*! PostScript closepath on the device. */
static int hpgs_closepath   (hpgs_device *_this);
/*! PostScript newpath on the device. */
static int hpgs_newpath     (hpgs_device *_this);
/*! PostScript stroke on the device. */
static int hpgs_stroke      (hpgs_device *_this);
/*! PostScript fill/eofill on the device.
    If \c winding is \c HPGS_TRUE we issue \c fill, otherwise \c eofill. */
static int hpgs_fill        (hpgs_device *_this, hpgs_bool winding);
/*! PostScript clip/eoclip on the device.
    If \c winding is \c HPGS_TRUE we issue \c clip, otherwise \c eoclip. */
static int hpgs_clip        (hpgs_device *_this, hpgs_bool winding);
/*! Save the clip state onto the clip stack. Unlike PostScripts \c gsave
    the line attributes and colors of the graphics state are not saved.  */
static int hpgs_clipsave    (hpgs_device *_this);
/*! Restores the last clip state from the clip stack. */
static int hpgs_cliprestore (hpgs_device *_this);
/*! PostScript setrgbcolor on the device. */
static int hpgs_setrgbcolor (hpgs_device *_this, const hpgs_color *rgb);
/*! PostScript setdash on the device. */
static int hpgs_setdash     (hpgs_device *_this, const float *d,
				 unsigned nd, double s);
/*! PostScript setlinewidth on the device. */
static int hpgs_setlinewidth(hpgs_device *_this, double w);
/*! PostScript setlinecap on the device. */
static int hpgs_setlinecap  (hpgs_device *_this, hpgs_line_cap c);
/*! PostScript setlinejoin on the device. */
static int hpgs_setlinejoin (hpgs_device *_this, hpgs_line_join j);
/*! PostScript setmiterlimit on the device. */
static int hpgs_setmiterlimit (hpgs_device *_this, double l);
/*! Get the device capabilities. */
static int hpgs_device_capabilities (hpgs_device *_this);

HPGS_API int hpgs_setrop3 (hpgs_device *_this, int rop,
                           hpgs_bool src_transparency, hpgs_bool pattern_transparency);
HPGS_API int hpgs_setpatcol (hpgs_device *_this, const hpgs_color *rgb);

HPGS_API int hpgs_drawimage(hpgs_device *_this, const hpgs_image *img,
                            const hpgs_point *ll, const hpgs_point *lr,
                            const hpgs_point *ur);

HPGS_API int hpgs_setplotsize (hpgs_device *_this, const hpgs_bbox *bb);
HPGS_API int hpgs_getplotsize (hpgs_device *_this, int i, hpgs_bbox *bb);
HPGS_API int hpgs_showpage    (hpgs_device *_this, int i);
HPGS_API int hpgs_device_finish   (hpgs_device *_this);
HPGS_API void hpgs_device_destroy (hpgs_device *_this);

__inline__ int hpgs_moveto      (hpgs_device *_this, const hpgs_point *p)
{ return _this->vtable->moveto(_this,p); }

__inline__ int hpgs_lineto      (hpgs_device *_this, const hpgs_point *p)
{ return _this->vtable->lineto(_this,p); }

__inline__ int hpgs_curveto     (hpgs_device *_this, const hpgs_point *p1,
			     const hpgs_point *p2, const hpgs_point *p3 )
{ return _this->vtable->curveto(_this,p1,p2,p3); }

__inline__ int hpgs_closepath   (hpgs_device *_this)
{ return _this->vtable->closepath(_this); }

__inline__ int hpgs_newpath   (hpgs_device *_this)
{ return _this->vtable->newpath(_this); }

__inline__ int hpgs_stroke      (hpgs_device *_this)
{ return _this->vtable->stroke(_this); }

__inline__ int hpgs_fill        (hpgs_device *_this, hpgs_bool winding)
{ return _this->vtable->fill(_this,winding); }

__inline__ int hpgs_clip        (hpgs_device *_this, hpgs_bool winding)
{ return _this->vtable->clip(_this,winding); }

__inline__ int hpgs_clipsave    (hpgs_device *_this)
{ return _this->vtable->clipsave(_this); }

__inline__ int hpgs_cliprestore    (hpgs_device *_this)
{ return _this->vtable->cliprestore(_this); }

__inline__ int hpgs_setrgbcolor (hpgs_device *_this, const hpgs_color *rgb)
{ return _this->vtable->setrgbcolor ? _this->vtable->setrgbcolor(_this,rgb) : 0; }

__inline__ int hpgs_setdash     (hpgs_device *_this, const float *d,
                                 unsigned nd, double s)
{ return _this->vtable->setdash ? _this->vtable->setdash(_this,d,nd,s) : 0; }

__inline__ int hpgs_setlinewidth(hpgs_device *_this, double w)
{ return _this->vtable->setlinewidth ? _this->vtable->setlinewidth(_this,w) : 0; }

__inline__ int hpgs_setlinecap  (hpgs_device *_this, hpgs_line_cap c)
{ return _this->vtable->setlinecap ? _this->vtable->setlinecap(_this,c) : 0; }

__inline__ int hpgs_setlinejoin (hpgs_device *_this, hpgs_line_join j)
{ return _this->vtable->setlinejoin ? _this->vtable->setlinejoin(_this,j) : 0; }

__inline__ int hpgs_setmiterlimit (hpgs_device *_this, double l)
{ return _this->vtable->setmiterlimit ? _this->vtable->setmiterlimit(_this,l) : 0; }

__inline__ int hpgs_device_capabilities (hpgs_device *_this)
{ return _this->vtable->capabilities(_this); }

/*! @} */ /* end of group device */

/*! @addtogroup reader
 *  @{
 */
typedef struct hpgs_reader_st hpgs_reader;

HPGS_API hpgs_reader *hpgs_new_reader(hpgs_istream *in, hpgs_device *dev,
                                      hpgs_bool multipage, int v);

HPGS_API void hpgs_reader_set_lw_factor(hpgs_reader *reader, double lw_factor);

HPGS_API void hpgs_reader_set_fixed_page(hpgs_reader *reader,
                                         hpgs_bbox *bbox,
                                         double page_width,
                                         double page_height,
                                         double border,
                                         double angle       );

HPGS_API void hpgs_reader_set_dynamic_page(hpgs_reader *reader,
                                           hpgs_bbox *bbox,
                                           double max_page_width,
                                           double max_page_height,
                                           double border,
                                           double angle       );

HPGS_API void hpgs_reader_set_page_asset_func(hpgs_reader *reader,
                                              void * ctxt,
                                              hpgs_reader_asset_func_t func);

HPGS_API void hpgs_reader_set_frame_asset_func(hpgs_reader *reader,
                                               void * ctxt,
                                               hpgs_reader_asset_func_t func);

HPGS_API void hpgs_reader_interrupt(hpgs_reader *reader);
HPGS_API int hpgs_reader_get_current_pen(hpgs_reader *reader);

HPGS_API int  hpgs_reader_imbue(hpgs_reader *reader, hpgs_device *dev);
HPGS_API int  hpgs_reader_attach(hpgs_reader *reader, hpgs_istream *in);
HPGS_API int  hpgs_reader_stamp(hpgs_reader *reader,
                                const hpgs_bbox *bb,
                                const char *stamp, const char *encoding,
                                double stamp_size);

HPGS_API int  hpgs_device_stamp(hpgs_device *dev,
                                const hpgs_bbox *bb,
                                const char *stamp, const char *encoding,
                                double stamp_size);

HPGS_API int  hpgs_reader_set_png_dump(hpgs_reader *reader, const char *filename);
HPGS_API int  hpgs_read(hpgs_reader *reader, hpgs_bool finish);
HPGS_API void hpgs_destroy_reader(hpgs_reader *reader);

/*! @} */ /* end of group reader */

/*! @addtogroup font
 *  @{
 */

HPGS_API hpgs_font *hpgs_find_font(const char *name);
HPGS_API void hpgs_destroy_font(hpgs_font *font);
HPGS_API double hpgs_font_get_ascent(hpgs_font *font);
HPGS_API double hpgs_font_get_descent(hpgs_font *font);
HPGS_API double hpgs_font_get_line_gap(hpgs_font *font);
HPGS_API double hpgs_font_get_cap_height(hpgs_font *font);
HPGS_API unsigned hpgs_font_get_glyph_count(hpgs_font *font);
HPGS_API unsigned hpgs_font_get_glyph_id(hpgs_font *font, int uc);
HPGS_API const char *hpgs_font_get_glyph_name(hpgs_font *font, unsigned gid);
HPGS_API int hpgs_font_get_glyph_bbox(hpgs_font *font, hpgs_bbox *bb, unsigned gid);
HPGS_API int hpgs_font_get_glyph_metrics(hpgs_font *font, hpgs_point *m, unsigned gid);
HPGS_API int hpgs_font_get_kern_metrics(hpgs_font *font, hpgs_point *m, unsigned gid_l, unsigned gid_r);
HPGS_API int hpgs_font_get_utf8_metrics(hpgs_font *font, hpgs_point *m, const char *str, int strlen);

typedef int (*hpgs_moveto_func_t) (void *ctxt, const hpgs_point *p);
typedef int (*hpgs_lineto_func_t) (void *ctxt, const hpgs_point *p);
typedef int (*hpgs_curveto_func_t) (void *ctxt, const hpgs_point *p1, const hpgs_point *p2, const hpgs_point *p3);
typedef int (*hpgs_fill_func_t) (void *ctxt, hpgs_bool winding);

HPGS_API int hpgs_font_decompose_glyph(hpgs_font *font,
                                       void *ctxt,
                                       hpgs_moveto_func_t moveto_func,
                                       hpgs_lineto_func_t lineto_func,
                                       hpgs_curveto_func_t curveto_func,
                                       const hpgs_matrix *m,
                                       unsigned gid);

HPGS_API int hpgs_font_draw_glyph(hpgs_font *font,
                                  hpgs_device *device,
                                  const hpgs_matrix *m,
                                  unsigned gid);

HPGS_API int hpgs_font_decompose_utf8(hpgs_font *font,
                                      void *ctxt,
                                      hpgs_moveto_func_t moveto_func,
                                      hpgs_lineto_func_t lineto_func,
                                      hpgs_curveto_func_t curveto_func,
                                      hpgs_fill_func_t fill_func,
                                      const hpgs_matrix *m,
                                      const char *str, int strlen);

HPGS_API int hpgs_font_draw_utf8(hpgs_font *font,
                                 hpgs_device *device,
                                 const hpgs_matrix *m,
                                 const char *str, int strlen);

/*! @} */ /* end of group font */

/*! @addtogroup image
 *  @{
 */

/*! \brief A table of virtual function implementing \c hpgs_image.

    This structure has a public alias \c hpgs_image_vtable and
    represents a table of functions for a specific implementation
    of \c hpgs_image.
 */
struct hpgs_image_vtable_st {
  int (*get_pixel)   (const hpgs_image *_this,
		      int x, int y, hpgs_paint_color *c, double *alpha);
  int (*put_pixel)   (hpgs_image *_this,
		      int x, int y, const hpgs_paint_color *c, double alpha);
  int (*put_chunk)   (hpgs_image *_this,
		      int x1, int x2, int y, const hpgs_paint_color *c);
  int (*rop3_pixel)  (hpgs_image *_this,
		      int x, int y, const hpgs_paint_color *c, double alpha);
  int (*rop3_chunk)  (hpgs_image *_this,
		      int x1, int x2, int y, const hpgs_paint_color *c);
  int (*setrop3)     (hpgs_image *_this, hpgs_rop3_func_t rop3);
  int (*setpatcol)   (hpgs_image *_this, const hpgs_palette_color *p);
  int (*resize)      (hpgs_image *_this, int w, int h);
  int (*set_resolution)(hpgs_image *pim, double x_dpi, double y_dpi);
  int (*clear)       (hpgs_image *_this);
  int (*write)       (hpgs_image *_this, const char *filename);
  int (*get_data)    (hpgs_image *_this, unsigned char **data, int *stride, int *depth);
  void (*destroy)    (hpgs_image *_this);
};

/*! \brief An abstract pixel image.

    This structure has a public alias \c hpgs_image and
    represents a rectangular pixel container, which tradiationally
    is called an image.

    Nevertheless, implementations for drawing to a window system
    may be provided in the future.
 */
struct hpgs_image_st {
  hpgs_image_vtable *vtable; //!< The virtual table.
  int  width;                //!< The number of pixel columns.
  int  height;               //!< The number of pixel rows.
  
  hpgs_palette_color *palette; //!< The palette for indexed images.
  unsigned *palette_idx;       //!< a compacted rgb color index.
  int palette_ncolors;         //!< The number of colors in the palette.
};

HPGS_API int hpgs_image_define_color_func (hpgs_image *image, hpgs_paint_color *c);

static int hpgs_image_define_color (hpgs_image *image, hpgs_paint_color *c);

/*! Enter the supplied rgb value to the palette of an indexed image.
    Set the index member of \c c to the palette index of the
    RGB triplet.

    If the palette is exhausted, -1 is returned.
 */
__inline__ int hpgs_image_define_color (hpgs_image *image, hpgs_paint_color *c)
{
  if (image->palette) return hpgs_image_define_color_func (image,c); else return 0;
}

HPGS_API int hpgs_image_set_palette (hpgs_image *image,
                                     hpgs_palette_color *p, int np);

HPGS_API hpgs_png_image *hpgs_new_png_image(int width, int height,
                                            int depth, hpgs_bool palette,
                                            hpgs_bool do_rop3);

HPGS_API int hpgs_png_image_set_compression(hpgs_png_image *pim, int compression);

HPGS_API int hpgs_image_set_resolution(hpgs_image *pim, double x_dpi, double y_dpi);

HPGS_API int hpgs_image_get_data(hpgs_image *_this,
                                 unsigned char **data, int *stride,
                                 int *depth);

/*! Retrieves the color and alpha value of the pixel 
    in column \c x and row \c y.  */
static int hpgs_image_get_pixel (const hpgs_image *_this,
				 int x, int y,
				 hpgs_paint_color *c, double *alpha);
/*! Sets the color and alpha value of the pixel 
    in column \c x and row \c y.  */
static int hpgs_image_put_pixel (hpgs_image *_this,
				 int x, int y,
				 const hpgs_paint_color *c, double alpha);
/*! Sets the color of all pixels 
    in the columns \c x1 up to \c x2 in row \c y.
    The alpha value is set to 1. */
static int hpgs_image_put_chunk (hpgs_image *_this,
				 int x1, int x2, int y, const hpgs_paint_color *c);

/*! Sets the color and alpha value of the pixel 
    in column \c x and row \c y. This function applies a ROP3
    operation, if supported by the image.
 */
static int hpgs_image_rop3_pixel (hpgs_image *_this,
                                  int x, int y,
                                  const hpgs_paint_color *c, double alpha);
/*! Sets the color of all pixels 
    in the columns \c x1 up to \c x2 in row \c y.
    The alpha value is set to 1. This function applies a ROP3
    operation, if supported by the image.
 */
static int hpgs_image_rop3_chunk (hpgs_image *_this,
                                  int x1, int x2, int y, const hpgs_paint_color *c);

HPGS_API int hpgs_image_resize (hpgs_image *_this, int w, int h);
HPGS_API int hpgs_image_clear (hpgs_image *_this);
HPGS_API int hpgs_image_write (hpgs_image *_this, const char *filename);
HPGS_API int hpgs_image_setrop3 (hpgs_image *_this, hpgs_rop3_func_t rop3);
HPGS_API int hpgs_image_setpatcol (hpgs_image *_this, const hpgs_palette_color *p);

HPGS_API void hpgs_image_destroy         (hpgs_image *_this);

__inline__ int hpgs_image_get_pixel (const hpgs_image *_this,
				     int x, int y,
				     hpgs_paint_color *c, double *alpha)
{ return _this->vtable->get_pixel(_this,x,y,c,alpha); }

__inline__ int hpgs_image_put_pixel (hpgs_image *_this,
				     int x, int y,
				     const hpgs_paint_color *c, double alpha)
{ return _this->vtable->put_pixel(_this,x,y,c,alpha); }

__inline__ int hpgs_image_put_chunk (hpgs_image *_this,
				     int x1, int x2, int y, const hpgs_paint_color *c)
{ return _this->vtable->put_chunk(_this,x1,x2,y,c); }

__inline__ int hpgs_image_rop3_pixel (hpgs_image *_this,
                                      int x, int y,
                                      const hpgs_paint_color *c, double alpha)
{ return _this->vtable->rop3_pixel(_this,x,y,c,alpha); }

__inline__ int hpgs_image_rop3_chunk (hpgs_image *_this,
                                      int x1, int x2, int y, const hpgs_paint_color *c)
{ return _this->vtable->rop3_chunk(_this,x1,x2,y,c); }

HPGS_API int hpgs_image_rop3_clip(hpgs_device *device,
                                  hpgs_palette_color *data,
                                  const hpgs_image *img,
                                  const hpgs_point *ll, const hpgs_point *lr,
                                  const hpgs_point *ur,
                                  const hpgs_palette_color *p,
                                  hpgs_xrop3_func_t xrop3);

/*! @} */ /* end of group image */

/*! @addtogroup path
 *  @{
 */

typedef struct hpgs_path_point_st hpgs_path_point;
typedef struct hpgs_paint_path_st hpgs_paint_path;

#define HPGS_POINT_ROLE_MASK     0xF //!< A mask for retrieving the role of a point in the path
#define HPGS_POINT_UNDEFINED     0x0 //!< point is undefined
#define HPGS_POINT_LINE          0x1 //!< start of line
#define HPGS_POINT_BEZIER        0x2 //!< start of bezier curve
#define HPGS_POINT_CONTROL       0x3 //!< bezier control point
#define HPGS_POINT_FILL_LINE     0x4 //!< line fill only (implicit closepath)
#define HPGS_POINT_DOT           0x8 //!< indicates a moveto/lineto with the same coordinates at the start of a subpath.

#define HPGS_POINT_SUBPATH_START   0x10 //!< a subpath starts at this point
#define HPGS_POINT_SUBPATH_END     0x20 //!< a subpath ends at this point
#define HPGS_POINT_SUBPATH_CLOSE   0x40 //!< indicates an explicit closepath at the end of a subpath

/*! \brief A point in a stored path.

    This structure has a public alias \c hpgs_path_point and
    represents a point of a polygonal path.
 */
struct hpgs_path_point_st
{
  hpgs_point p; /*!< Coordinates of this path point. */
  int flags;    /*!< Flags for this path point. */
};

/*! \brief A stored vector path.

    This structure has a public alias \c hpgs_paint_path and
    represents a polygonal path, which is stores the path topology
    together with cached informations about bezier splines.
 */
struct hpgs_paint_path_st
{
  /*@{ */
  /*! A stack of path points.
      \c last_start indicates the index of the last start of a subpolygon.
      This is information is needed during path construction.
   */
  hpgs_path_point *points;
  int              n_points;
  int              last_start;
  int              points_malloc_size;
  /*@} */

  hpgs_bbox bb; /*! The bounding box of this path. */
};

HPGS_API hpgs_paint_path *hpgs_new_paint_path(void);
HPGS_API void             hpgs_paint_path_destroy(hpgs_paint_path *_this);
HPGS_API void             hpgs_paint_path_truncate(hpgs_paint_path *_this);

HPGS_API int              hpgs_paint_path_moveto(hpgs_paint_path *_this,
                                                 const hpgs_point *p  );

HPGS_API int              hpgs_paint_path_lineto(hpgs_paint_path *_this,
                                                 const hpgs_point *p  );

HPGS_API int              hpgs_paint_path_curveto(hpgs_paint_path *_this,
                                                  const hpgs_point *p1,
                                                  const hpgs_point *p2,
                                                  const hpgs_point *p3  );

HPGS_API int              hpgs_paint_path_closepath(hpgs_paint_path *_this);

HPGS_API int              hpgs_paint_path_buldgeto(hpgs_paint_path *_this,
                                                   const hpgs_point *p,
                                                   double buldge);

HPGS_API hpgs_paint_path *hpgs_paint_path_stroke_path(const hpgs_paint_path *_this,
                                                               const hpgs_gstate *gstate);

HPGS_API hpgs_paint_path *hpgs_paint_path_decompose_dashes(const hpgs_paint_path *_this,
                                                           const hpgs_gstate *gstate);

HPGS_API double hpgs_bezier_path_x (const hpgs_paint_path *path, int i, double t);
HPGS_API double hpgs_bezier_path_y (const hpgs_paint_path *path, int i, double t);
HPGS_API double hpgs_bezier_path_delta_x (const hpgs_paint_path *path, int i, double t);
HPGS_API double hpgs_bezier_path_delta_y (const hpgs_paint_path *path, int i, double t);

HPGS_API double hpgs_bezier_path_xmin (const hpgs_paint_path *path, int i);
HPGS_API double hpgs_bezier_path_xmax (const hpgs_paint_path *path, int i);
HPGS_API double hpgs_bezier_path_ymin (const hpgs_paint_path *path, int i);
HPGS_API double hpgs_bezier_path_ymax (const hpgs_paint_path *path, int i);

HPGS_API void hpgs_bezier_path_point(const hpgs_paint_path *path, int i,
                                     double t, hpgs_point *p);

HPGS_API void hpgs_bezier_path_delta(const hpgs_paint_path *path, int i,
                                     double t, hpgs_point *p);

HPGS_API void hpgs_bezier_path_tangent(const hpgs_paint_path *path, int i,
                                       double t, int orientation,
                                       double ytol,
                                       hpgs_point *p);

HPGS_API void hpgs_bezier_path_singularities(const hpgs_paint_path *path, int i,
                                             double t0, double t1,
                                             int *nx, double *tx);

HPGS_API void hpgs_bezier_path_to_quadratic(const hpgs_paint_path *path, int i,
                                            double t0, double t1,
                                            int *nx, hpgs_point *points);

/*! @} */ /* end of group path */

/*! @addtogroup paint_device
 *  @{
 */
HPGS_API hpgs_paint_device *hpgs_new_paint_device(hpgs_image *image,
                                                  const char *filename,
                                                  const hpgs_bbox *bb,
                                                  hpgs_bool antialias);

HPGS_API void hpgs_paint_device_set_image_interpolation (hpgs_paint_device *pdv, int i);
HPGS_API void hpgs_paint_device_set_thin_alpha (hpgs_paint_device *pdv, double a);

/*! @} */ /* end of group paint_device */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif /* ! __HPGS_H */
