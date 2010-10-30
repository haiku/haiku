/*
 * "$Id: gutenprintui.h,v 1.2 2005/01/28 03:02:18 rlk Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu). and Steve Miller (smiller@rni.net
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifndef GUTENPRINTUI_H
#define GUTENPRINTUI_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __GNUC__
#define inline __inline__
#endif

#include <gtk/gtk.h>

#include <gutenprint/gutenprint.h>

#include <gutenprintui2/curve.h>
#include <gutenprintui2/gammacurve.h>
#include <gutenprintui2/typebuiltins.h>

/*
 * All Gimp-specific code is in this file.
 */

typedef enum
{
  ORIENT_AUTO = -1,
  ORIENT_PORTRAIT = 0,
  ORIENT_LANDSCAPE = 1,
  ORIENT_UPSIDEDOWN = 2,
  ORIENT_SEASCAPE = 3
} orient_t;

/*
 * If this is changed, command_options[] in panel.c must be appropriately
 * updated.
 */
typedef enum
{
  COMMAND_TYPE_DEFAULT,
  COMMAND_TYPE_CUSTOM,
  COMMAND_TYPE_FILE
} command_t;

typedef struct		/**** Printer List ****/
{
  char		*name;		/* Name of printer */
  command_t	command_type;
  char		*queue_name;
  char		*extra_printer_options;
  char		*custom_command;
  char		*current_standard_command;
  char		*output_filename;
  float		scaling;      /* Scaling, percent of printable area */
  orient_t	orientation;
  int		unit;	  /* Units for preview area 0=Inch 1=Metric */
  int		auto_size_roll_feed_paper;
  int		invalid_mask;
  stp_vars_t	*v;
} stpui_plist_t;

typedef struct stpui_image
{
  stp_image_t im;
  void (*transpose)(struct stpui_image *image);
  void (*hflip)(struct stpui_image *image);
  void (*vflip)(struct stpui_image *image);
  void (*rotate_ccw)(struct stpui_image *image);
  void (*rotate_cw)(struct stpui_image *image);
  void (*rotate_180)(struct stpui_image *image);
  void (*crop)(struct stpui_image *image, int left, int top,
	       int right, int bottom);
} stpui_image_t;  

/*
 * Function prototypes
 */
extern void stpui_plist_set_name(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_name_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_name(const stpui_plist_t *p);

extern void stpui_plist_set_queue_name(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_queue_name_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_queue_name(const stpui_plist_t *p);

extern void stpui_plist_set_output_filename(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_output_filename_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_output_filename(const stpui_plist_t *p);

extern void stpui_plist_set_extra_printer_options(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_extra_printer_options_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_extra_printer_options(const stpui_plist_t *p);

extern void stpui_plist_set_custom_command(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_custom_command_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_custom_command(const stpui_plist_t *p);

extern void stpui_plist_set_copy_count(stpui_plist_t *p, gint count);
extern int stpui_plist_get_copy_count(const stpui_plist_t *p);

extern void stpui_plist_set_current_standard_command(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_current_standard_command_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_current_standard_command(const stpui_plist_t *p);

extern void stpui_plist_set_command_type(stpui_plist_t *p, command_t val);
extern command_t stpui_plist_get_command_type(const stpui_plist_t *p);

extern void stpui_set_global_parameter(const char *param, const char *value);
extern const char *stpui_get_global_parameter(const char *param);

extern void stpui_plist_copy(stpui_plist_t *vd, const stpui_plist_t *vs);
extern int stpui_plist_add(const stpui_plist_t *key, int add_only);
extern void stpui_printer_initialize(stpui_plist_t *printer);
extern const stpui_plist_t *stpui_get_current_printer(void);

extern char *stpui_build_standard_print_command(const stpui_plist_t *plist,
						const stp_printer_t *printer);

extern void stpui_set_printrc_file(const char *name);
extern const char * stpui_get_printrc_file(void);
extern void stpui_printrc_load (void);
extern void stpui_get_system_printers (void);
extern void stpui_printrc_save (void);
extern void stpui_set_image_filename(const char *);
extern const char *stpui_get_image_filename(void);
extern void stpui_set_errfunc(stp_outfunc_t wfunc);
extern stp_outfunc_t stpui_get_errfunc(void);
extern void stpui_set_errdata(void *errdata);
extern void *stpui_get_errdata(void);

extern gint stpui_do_print_dialog (void);

extern gint stpui_compute_orientation(void);
extern void stpui_set_image_dimensions(gint width, gint height);
extern void stpui_set_image_resolution(gdouble xres, gdouble yres);
extern void stpui_set_image_type(const char *image_type);
extern void stpui_set_image_raw_channels(gint channels);
extern void stpui_set_image_channel_depth(gint bit_depth);

typedef guchar *(*get_thumbnail_func_t)(void *data, gint *width, gint *height,
					gint *bpp, gint page);
extern void stpui_set_thumbnail_func(get_thumbnail_func_t);
extern get_thumbnail_func_t stpui_get_thumbnail_func(void);
extern void stpui_set_thumbnail_data(void *);
extern void *stpui_get_thumbnail_data(void);

extern int stpui_print(const stpui_plist_t *printer, stpui_image_t *im);


#ifdef __cplusplus
  }
#endif

#endif  /* GUTENPRINTUI_H */
