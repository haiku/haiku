/*
 * "$Id: channel.h,v 1.6 2005/07/03 22:13:36 rlk Exp $"
 *
 *   libgimpprint header.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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
 * Revision History:
 *
 *   See ChangeLog
 */

/**
 * @file gutenprint/channel.h
 * @brief Channel functions.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_CHANNEL_H
#define GUTENPRINT_CHANNEL_H

#ifdef __cplusplus
extern "C" {
#endif


extern void stp_channel_reset(stp_vars_t *v);
extern void stp_channel_reset_channel(stp_vars_t *v, int channel);

extern void stp_channel_add(stp_vars_t *v, unsigned channel,
			    unsigned subchannel, double value);
extern double stp_channel_get_value(stp_vars_t *v, unsigned channel,
				    unsigned subchannel);

extern void stp_channel_set_density_adjustment(stp_vars_t *v,
					       int color, int subchannel,
					       double adjustment);
extern double stp_channel_get_density_adjustment(stp_vars_t *v,
						 int color, int subchannel);
extern void stp_channel_set_ink_limit(stp_vars_t *v, double limit);
extern double stp_channel_get_ink_limit(stp_vars_t *v);
extern void stp_channel_set_cutoff_adjustment(stp_vars_t *v,
					      int color, int subchannel,
					      double adjustment);
extern double stp_channel_get_cutoff_adjustment(stp_vars_t *v,
						int color, int subchannel);
extern void stp_channel_set_black_channel(stp_vars_t *v, int channel);
extern int stp_channel_get_black_channel(stp_vars_t *v);
extern void stp_channel_set_gloss_channel(stp_vars_t *v, int channel);
extern int stp_channel_get_gloss_channel(stp_vars_t *v);
extern void stp_channel_set_gloss_limit(stp_vars_t *v, double limit);
extern double stp_channel_get_gloss_limit(stp_vars_t *v);
extern void stp_channel_set_curve(stp_vars_t *v, int channel,
				  const stp_curve_t *curve);
extern const stp_curve_t *stp_channel_get_curve(stp_vars_t *v, int channel);
extern void stp_channel_set_gcr_curve(stp_vars_t *v, const stp_curve_t *curve);
extern const stp_curve_t *stp_channel_get_gcr_curve(stp_vars_t *v);

extern void stp_channel_initialize(stp_vars_t *v, stp_image_t *image,
				   int input_channel_count);

extern void stp_channel_convert(const stp_vars_t *v, unsigned *zero_mask);

extern unsigned short * stp_channel_get_input(const stp_vars_t *v);

extern unsigned short * stp_channel_get_output(const stp_vars_t *v);

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_CHANNEL_H */
