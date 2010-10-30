/*
 * "$Id: curve.c,v 1.55 2010/08/04 00:33:56 rlk Exp $"
 *
 *   Print plug-in driver utility functions for the GIMP.
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <math.h>
#ifdef sun
#include <ieeefp.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __GNUC__
#define inline __inline__
#endif

#undef inline
#define inline

/*
 * We could do more sanity checks here if we want.
 */
#define CHECK_CURVE(curve)			\
do						\
  {						\
    STPI_ASSERT((curve) != NULL, NULL);		\
    STPI_ASSERT((curve)->seq != NULL, NULL);	\
  } while (0)

static const int curve_point_limit = 1048576;

struct stp_curve
{
  stp_curve_type_t curve_type;
  stp_curve_wrap_mode_t wrap_mode;
  int piecewise;
  int recompute_interval;	/* Do we need to recompute the deltas? */
  double gamma;			/* 0.0 means that the curve is not a gamma */
  stp_sequence_t *seq;          /* Sequence (contains the curve data) */
  double *interval;		/* We allocate an extra slot for the
				   wrap-around value. */

};

static const char *const stpi_curve_type_names[] =
  {
    "linear",
    "spline",
  };

static const int stpi_curve_type_count =
(sizeof(stpi_curve_type_names) / sizeof(const char *));

static const char *const stpi_wrap_mode_names[] =
  {
    "nowrap",
    "wrap"
  };

static const int stpi_wrap_mode_count =
(sizeof(stpi_wrap_mode_names) / sizeof(const char *));

/*
 * Get the total number of points in the base sequence class
 */
static size_t
get_real_point_count(const stp_curve_t *curve)
{
  if (curve->piecewise)
    return stp_sequence_get_size(curve->seq) / 2;
  else
    return stp_sequence_get_size(curve->seq);
}

/*
 * Get the number of points used by the curve (that are visible to the
 * user).  This is the real point count, but is decreased by 1 if the
 * curve wraps around.
 */
static size_t
get_point_count(const stp_curve_t *curve)
{
  size_t count = get_real_point_count(curve);
  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    count -= 1;

  return count;
}

static void
invalidate_auxiliary_data(stp_curve_t *curve)
{
  STP_SAFE_FREE(curve->interval);
}

static void
clear_curve_data(stp_curve_t *curve)
{
  if (curve->seq)
    stp_sequence_set_size(curve->seq, 0);
  curve->recompute_interval = 0;
  invalidate_auxiliary_data(curve);
}

static void
compute_linear_deltas(stp_curve_t *curve)
{
  int i;
  size_t delta_count;
  size_t seq_point_count;
  const double *data;

  stp_sequence_get_data(curve->seq, &seq_point_count, &data);
  if (data == NULL)
    return;

  delta_count = get_real_point_count(curve);

  if (delta_count <= 1) /* No intervals can be computed */
    return;
  delta_count--; /* One less than the real point count.  Note size_t
		    is unsigned. */

  curve->interval = stp_malloc(sizeof(double) * delta_count);
  for (i = 0; i < delta_count; i++)
    {
      if (curve->piecewise)
	curve->interval[i] = data[(2 * (i + 1)) + 1] - data[(2 * i) + 1];
      else
	curve->interval[i] = data[i + 1] - data[i];
    }
}

static void
compute_spline_deltas_piecewise(stp_curve_t *curve)
{
  int i;
  int k;
  double *u;
  double *y2;
  const double *data = NULL;
  const stp_curve_point_t *dp;
  size_t point_count;
  size_t real_point_count;
  double sig;
  double p;

  point_count = get_point_count(curve);

  stp_sequence_get_data(curve->seq, &real_point_count, &data);
  dp = (const stp_curve_point_t *)data;
  real_point_count = real_point_count / 2;

  u = stp_malloc(sizeof(double) * real_point_count);
  y2 = stp_malloc(sizeof(double) * real_point_count);

  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    {
      int reps = 3;
      int count = reps * real_point_count;
      double *y2a = stp_malloc(sizeof(double) * count);
      double *ua = stp_malloc(sizeof(double) * count);
      y2a[0] = 0.0;
      ua[0] = 0.0;
      for (i = 1; i < count - 1; i++)
	{
	  int im1 = (i - 1) % point_count;
	  int ia = i % point_count;
	  int ip1 = (i + 1) % point_count;

	  sig = (dp[ia].x - dp[im1].x) / (dp[ip1].x - dp[im1].x);
	  p = sig * y2a[im1] + 2.0;
	  y2a[i] = (sig - 1.0) / p;

	  ua[i] = ((dp[ip1].y - dp[ia].y) / (dp[ip1].x - dp[ia].x)) -
	    ((dp[ia].y - dp[im1].y) / (dp[ia].x - dp[im1].x));
	  ua[i] =
	    (((6.0 * ua[ia]) / (dp[ip1].x - dp[im1].x)) - (sig * ua[im1])) / p;
	}
      y2a[count - 1] = 0.0;
      for (k = count - 2 ; k >= 0; k--)
	y2a[k] = y2a[k] * y2a[k + 1] + ua[k];
      memcpy(u, ua + ((reps / 2) * point_count),
	     sizeof(double) * real_point_count);
      memcpy(y2, y2a + ((reps / 2) * point_count),
	     sizeof(double) * real_point_count);
      stp_free(y2a);
      stp_free(ua);
    }
  else
    {
      int count = real_point_count - 1;

      y2[0] = 0;
      u[0] = 2 * (dp[1].y - dp[0].y);
      for (i = 1; i < count; i++)
	{
	  int im1 = (i - 1);
	  int ip1 = (i + 1);

	  sig = (dp[i].x - dp[im1].x) / (dp[ip1].x - dp[im1].x);
	  p = sig * y2[im1] + 2.0;
	  y2[i] = (sig - 1.0) / p;

	  u[i] = ((dp[ip1].y - dp[i].y) / (dp[ip1].x - dp[i].x)) -
	    ((dp[i].y - dp[im1].y) / (dp[i].x - dp[im1].x));
	  u[i] =
	    (((6.0 * u[i]) / (dp[ip1].x - dp[im1].x)) - (sig * u[im1])) / p;
	  stp_deprintf(STP_DBG_CURVE,
		       "%d sig %f p %f y2 %f u %f x %f %f %f y %f %f %f\n",
		       i, sig, p, y2[i], u[i],
		       dp[im1].x, dp[i].x, dp[ip1].x,
		       dp[im1].y, dp[i].y, dp[ip1].y);
	}
      y2[count] = 0.0;
      u[count] = 0.0;
      for (k = real_point_count - 2; k >= 0; k--)
	y2[k] = y2[k] * y2[k + 1] + u[k];
    }

  curve->interval = y2;
  stp_free(u);
}

static void
compute_spline_deltas_dense(stp_curve_t *curve)
{
  int i;
  int k;
  double *u;
  double *y2;
  const double *y;
  size_t point_count;
  size_t real_point_count;
  double sig;
  double p;

  point_count = get_point_count(curve);

  stp_sequence_get_data(curve->seq, &real_point_count, &y);
  u = stp_malloc(sizeof(double) * real_point_count);
  y2 = stp_malloc(sizeof(double) * real_point_count);

  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    {
      int reps = 3;
      int count = reps * real_point_count;
      double *y2a = stp_malloc(sizeof(double) * count);
      double *ua = stp_malloc(sizeof(double) * count);
      y2a[0] = 0.0;
      ua[0] = 0.0;
      for (i = 1; i < count - 1; i++)
	{
	  int im1 = (i - 1);
	  int ip1 = (i + 1);
	  int im1a = im1 % point_count;
	  int ia = i % point_count;
	  int ip1a = ip1 % point_count;

	  sig = (i - im1) / (ip1 - im1);
	  p = sig * y2a[im1] + 2.0;
	  y2a[i] = (sig - 1.0) / p;

	  ua[i] = y[ip1a] - 2 * y[ia] + y[im1a];
	  ua[i] = 3.0 * ua[i] - sig * ua[im1] / p;
	}
      y2a[count - 1] = 0.0;
      for (k = count - 2 ; k >= 0; k--)
	y2a[k] = y2a[k] * y2a[k + 1] + ua[k];
      memcpy(u, ua + ((reps / 2) * point_count),
	     sizeof(double) * real_point_count);
      memcpy(y2, y2a + ((reps / 2) * point_count),
	     sizeof(double) * real_point_count);
      stp_free(y2a);
      stp_free(ua);
    }
  else
    {
      int count = real_point_count - 1;

      y2[0] = 0;
      u[0] = 2 * (y[1] - y[0]);
      for (i = 1; i < count; i++)
	{
	  int im1 = (i - 1);
	  int ip1 = (i + 1);

	  sig = (i - im1) / (ip1 - im1);
	  p = sig * y2[im1] + 2.0;
	  y2[i] = (sig - 1.0) / p;

	  u[i] = y[ip1] - 2 * y[i] + y[im1];
	  u[i] = 3.0 * u[i] - sig * u[im1] / p;
	}

      u[count] = 2 * (y[count] - y[count - 1]);
      y2[count] = 0.0;

      u[count] = 0.0;
      for (k = real_point_count - 2; k >= 0; k--)
	y2[k] = y2[k] * y2[k + 1] + u[k];
    }

  curve->interval = y2;
  stp_free(u);
}

static void
compute_spline_deltas(stp_curve_t *curve)
{
  if (curve->piecewise)
    compute_spline_deltas_piecewise(curve);
  else
    compute_spline_deltas_dense(curve);
}

/*
 * Recompute the delta values for interpolation.
 * When we actually do support spline curves, this routine will
 * compute the second derivatives for that purpose, too.
 */
static void
compute_intervals(stp_curve_t *curve)
{
  if (curve->interval)
    {
      stp_free(curve->interval);
      curve->interval = NULL;
    }
  if (stp_sequence_get_size(curve->seq) > 0)
    {
      switch (curve->curve_type)
	{
	case STP_CURVE_TYPE_SPLINE:
	  compute_spline_deltas(curve);
	  break;
	case STP_CURVE_TYPE_LINEAR:
	  compute_linear_deltas(curve);
	  break;
	}
    }
  curve->recompute_interval = 0;
}

static int
stpi_curve_set_points(stp_curve_t *curve, size_t points)
{
  if (points < 2)
    return 0;
  if (points > curve_point_limit ||
      (curve->wrap_mode == STP_CURVE_WRAP_AROUND &&
       points > curve_point_limit - 1))
    return 0;
  clear_curve_data(curve);
  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    points++;
  if (curve->piecewise)
    points *= 2;
  if ((stp_sequence_set_size(curve->seq, points)) == 0)
    return 0;
  return 1;
}

/*
 * Create a default curve
 */
static void
stpi_curve_ctor(stp_curve_t *curve, stp_curve_wrap_mode_t wrap_mode)
{
  curve->seq = stp_sequence_create();
  stp_sequence_set_bounds(curve->seq, 0.0, 1.0);
  curve->curve_type = STP_CURVE_TYPE_LINEAR;
  curve->wrap_mode = wrap_mode;
  curve->piecewise = 0;
  stpi_curve_set_points(curve, 2);
  curve->recompute_interval = 1;
  if (wrap_mode == STP_CURVE_WRAP_NONE)
    curve->gamma = 1.0;
  stp_sequence_set_point(curve->seq, 0, 0);
  stp_sequence_set_point(curve->seq, 1, 1);
}

stp_curve_t *
stp_curve_create(stp_curve_wrap_mode_t wrap_mode)
{
  stp_curve_t *ret;
  if (wrap_mode != STP_CURVE_WRAP_NONE && wrap_mode != STP_CURVE_WRAP_AROUND)
    return NULL;
  ret = stp_zalloc(sizeof(stp_curve_t));
  stpi_curve_ctor(ret, wrap_mode);
  return ret;
}

static void
curve_dtor(stp_curve_t *curve)
{
  CHECK_CURVE(curve);
  clear_curve_data(curve);
  if (curve->seq)
    stp_sequence_destroy(curve->seq);
  memset(curve, 0, sizeof(stp_curve_t));
  curve->curve_type = -1;
}

void
stp_curve_destroy(stp_curve_t *curve)
{
  curve_dtor(curve);
  stp_free(curve);
}

void
stp_curve_copy(stp_curve_t *dest, const stp_curve_t *source)
{
  CHECK_CURVE(dest);
  CHECK_CURVE(source);
  curve_dtor(dest);
  dest->curve_type = source->curve_type;
  dest->wrap_mode = source->wrap_mode;
  dest->gamma = source->gamma;
  dest->seq = stp_sequence_create_copy(source->seq);
  dest->piecewise = source->piecewise;
  dest->recompute_interval = 1;
}

stp_curve_t *
stp_curve_create_copy(const stp_curve_t *curve)
{
  stp_curve_t *ret;
  CHECK_CURVE(curve);
  ret = stp_curve_create(curve->wrap_mode);
  stp_curve_copy(ret, curve);
  return ret;
}

void
stp_curve_reverse(stp_curve_t *dest, const stp_curve_t *source)
{
  CHECK_CURVE(dest);
  CHECK_CURVE(source);
  curve_dtor(dest);
  dest->curve_type = source->curve_type;
  dest->wrap_mode = source->wrap_mode;
  dest->gamma = source->gamma;
  if (source->piecewise)
    {
      const double *source_data;
      size_t size;
      double *new_data;
      int i;
      stp_sequence_get_data(source->seq, &size, &source_data);
      new_data = stp_malloc(sizeof(double) * size);
      for (i = 0; i < size; i += 2)
	{
	  int j = size - i - 2;
	  new_data[i] = 1.0 - source_data[j];
	  new_data[i + 1] = source_data[j + 1];
	}
      dest->seq = stp_sequence_create();
      stp_sequence_set_data(dest->seq, size, new_data);
      stp_free(new_data);
    }
  else
      dest->seq = stp_sequence_create_reverse(source->seq);
  dest->piecewise = source->piecewise;
  dest->recompute_interval = 1;
}

stp_curve_t *
stp_curve_create_reverse(const stp_curve_t *curve)
{
  stp_curve_t *ret;
  CHECK_CURVE(curve);
  ret = stp_curve_create(curve->wrap_mode);
  stp_curve_reverse(ret, curve);
  return ret;
}

int
stp_curve_set_bounds(stp_curve_t *curve, double low, double high)
{
  CHECK_CURVE(curve);
  return stp_sequence_set_bounds(curve->seq, low, high);
}

void
stp_curve_get_bounds(const stp_curve_t *curve, double *low, double *high)
{
  CHECK_CURVE(curve);
  stp_sequence_get_bounds(curve->seq, low, high);
}

/*
 * Find the minimum and maximum points on the curve.  This does not
 * attempt to find the minimum and maximum interpolations; with cubic
 * splines these could exceed the boundaries.  That's OK; the interpolation
 * code will clip them to the bounds.
 */
void
stp_curve_get_range(const stp_curve_t *curve, double *low, double *high)
{
  CHECK_CURVE(curve);
  stp_sequence_get_range(curve->seq, low, high);
}

size_t
stp_curve_count_points(const stp_curve_t *curve)
{
  CHECK_CURVE(curve);
  return get_point_count(curve);
}

stp_curve_wrap_mode_t
stp_curve_get_wrap(const stp_curve_t *curve)
{
  CHECK_CURVE(curve);
  return curve->wrap_mode;
}

int
stp_curve_is_piecewise(const stp_curve_t *curve)
{
  CHECK_CURVE(curve);
  return curve->piecewise;
}

int
stp_curve_set_interpolation_type(stp_curve_t *curve, stp_curve_type_t itype)
{
  CHECK_CURVE(curve);
  if (itype < 0 || itype >= stpi_curve_type_count)
    return 0;
  curve->curve_type = itype;
  return 1;
}

stp_curve_type_t
stp_curve_get_interpolation_type(const stp_curve_t *curve)
{
  CHECK_CURVE(curve);
  return curve->curve_type;
}

int
stp_curve_set_gamma(stp_curve_t *curve, double fgamma)
{
  CHECK_CURVE(curve);
  if (curve->wrap_mode || ! isfinite(fgamma) || fgamma == 0.0)
    return 0;
  clear_curve_data(curve);
  curve->gamma = fgamma;
  stp_curve_resample(curve, 2);
  return 1;
}

double
stp_curve_get_gamma(const stp_curve_t *curve)
{
  CHECK_CURVE(curve);
  return curve->gamma;
}

int
stp_curve_set_data(stp_curve_t *curve, size_t count, const double *data)
{
  size_t i;
  size_t real_count = count;
  double low, high;
  CHECK_CURVE(curve);
  if (count < 2)
    return 0;
  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    real_count++;
  if (real_count > curve_point_limit)
    return 0;

  /* Validate the data before we commit to it. */
  stp_sequence_get_bounds(curve->seq, &low, &high);
  for (i = 0; i < count; i++)
    if (! isfinite(data[i]) || data[i] < low || data[i] > high)
      {
	stp_deprintf(STP_DBG_CURVE_ERRORS,
		     "stp_curve_set_data: datum out of bounds: "
		     "%g (require %g <= x <= %g), n = %ld\n",
		     data[i], low, high, (long)i);
	return 0;
      }
  /* Allocate sequence; also accounts for WRAP_MODE */
  stpi_curve_set_points(curve, count);
  curve->gamma = 0.0;
  stp_sequence_set_subrange(curve->seq, 0, count, data);

  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    stp_sequence_set_point(curve->seq, count, data[0]);
  curve->recompute_interval = 1;
  curve->piecewise = 0;

  return 1;
}

int
stp_curve_set_data_points(stp_curve_t *curve, size_t count,
			  const stp_curve_point_t *data)
{
  size_t i;
  size_t real_count = count;
  double low, high;
  double last_x = -1;
  CHECK_CURVE(curve);
  if (count < 2)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "stp_curve_set_data_points: too few points %ld\n", (long)count);
      return 0;
    }
  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    real_count++;
  if (real_count > curve_point_limit)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "stp_curve_set_data_points: too many points %ld\n",
		   (long)real_count);
      return 0;
    }

  /* Validate the data before we commit to it. */
  stp_sequence_get_bounds(curve->seq, &low, &high);
  for (i = 0; i < count; i++)
    {
      if (! isfinite(data[i].y) || data[i].y < low || data[i].y > high)
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "stp_curve_set_data_points: datum out of bounds: "
		       "%g (require %g <= x <= %g), n = %ld\n",
		       data[i].y, low, high, (long)i);
	  return 0;
	}
      if (i == 0 && data[i].x != 0.0)
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "stp_curve_set_data_points: first point must have x=0\n");
	  return 0;
	}
      if (curve->wrap_mode == STP_CURVE_WRAP_NONE && i == count - 1 &&
	  data[i].x != 1.0)
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "stp_curve_set_data_points: last point must have x=1\n");
	  return 0;
	}
      if (curve->wrap_mode == STP_CURVE_WRAP_AROUND &&
	  data[i].x >= 1.0 - .000001)
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "stp_curve_set_data_points: horizontal value must "
		       "not exceed .99999\n");
	  return 0;
	}	  
      if (data[i].x < 0 || data[i].x > 1)
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "stp_curve_set_data_points: horizontal position out of bounds: "
		       "%g, n = %ld\n",
		       data[i].x, (long)i);
	  return 0;
	}
      if (data[i].x - .000001 < last_x)
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "stp_curve_set_data_points: horizontal position must "
		       "exceed previous position by .000001: %g, %g, n = %ld\n",
		       data[i].x, last_x, (long)i);
	  return 0;
	}
      last_x = data[i].x;
    }
  /* Allocate sequence; also accounts for WRAP_MODE */
  curve->piecewise = 1;
  stpi_curve_set_points(curve, count);
  curve->gamma = 0.0;
  stp_sequence_set_subrange(curve->seq, 0, count * 2, (const double *) data);

  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    {
      stp_sequence_set_point(curve->seq, count * 2, data[0].x);
      stp_sequence_set_point(curve->seq, count * 2 + 1, data[0].y);
    }
  curve->recompute_interval = 1;

  return 1;
}


/*
 * Note that we return a pointer to the raw data here.
 * A lot of operations change the data vector, that's why we don't
 * guarantee it across non-const calls.
 */
const double *
stp_curve_get_data(const stp_curve_t *curve, size_t *count)
{
  const double *ret;
  CHECK_CURVE(curve);
  if (curve->piecewise)
    return NULL;
  stp_sequence_get_data(curve->seq, count, &ret);
  *count = get_point_count(curve);
  return ret;
}

const stp_curve_point_t *
stp_curve_get_data_points(const stp_curve_t *curve, size_t *count)
{
  const double *ret;
  CHECK_CURVE(curve);
  if (!curve->piecewise)
    return NULL;
  stp_sequence_get_data(curve->seq, count, &ret);
  *count = get_point_count(curve);
  return (const stp_curve_point_t *) ret;
}

static const double *
stpi_curve_get_data_internal(const stp_curve_t *curve, size_t *count)
{
  const double *ret;
  CHECK_CURVE(curve);
  stp_sequence_get_data(curve->seq, count, &ret);
  *count = get_point_count(curve);
  if (curve->piecewise)
    *count *= 2;
  return ret;
}


/* "Overloaded" functions */

#define DEFINE_DATA_SETTER(t, name)                                        \
int                                                                        \
stp_curve_set_##name##_data(stp_curve_t *curve, size_t count, const t *data) \
{                                                                          \
  double *tmp_data;                                                        \
  size_t i;                                                                \
  int status;                                                              \
  size_t real_count = count;                                               \
                                                                           \
  CHECK_CURVE(curve);                                                      \
  if (count < 2)                                                           \
    return 0;                                                              \
  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)                           \
    real_count++;                                                          \
  if (real_count > curve_point_limit)                                      \
    return 0;                                                              \
  tmp_data = stp_malloc(count * sizeof(double));                           \
  for (i = 0; i < count; i++)                                              \
    tmp_data[i] = (double) data[i];                                        \
  status = stp_curve_set_data(curve, count, tmp_data);                     \
  stp_free(tmp_data);                                                      \
  return status;                                                           \
 }

DEFINE_DATA_SETTER(float, float)
DEFINE_DATA_SETTER(long, long)
DEFINE_DATA_SETTER(unsigned long, ulong)
DEFINE_DATA_SETTER(int, int)
DEFINE_DATA_SETTER(unsigned int, uint)
DEFINE_DATA_SETTER(short, short)
DEFINE_DATA_SETTER(unsigned short, ushort)


#define DEFINE_DATA_ACCESSOR(t, name)					\
const t *								\
stp_curve_get_##name##_data(const stp_curve_t *curve, size_t *count)    \
{									\
  if (curve->piecewise)							\
    return 0;								\
  return stp_sequence_get_##name##_data(curve->seq, count);		\
}

DEFINE_DATA_ACCESSOR(float, float)
DEFINE_DATA_ACCESSOR(long, long)
DEFINE_DATA_ACCESSOR(unsigned long, ulong)
DEFINE_DATA_ACCESSOR(int, int)
DEFINE_DATA_ACCESSOR(unsigned int, uint)
DEFINE_DATA_ACCESSOR(short, short)
DEFINE_DATA_ACCESSOR(unsigned short, ushort)


stp_curve_t *
stp_curve_get_subrange(const stp_curve_t *curve, size_t start, size_t count)
{
  stp_curve_t *retval;
  size_t ncount;
  double blo, bhi;
  const double *data;
  if (start + count > stp_curve_count_points(curve) || count < 2)
    return NULL;
  if (curve->piecewise)
    return NULL;
  retval = stp_curve_create(STP_CURVE_WRAP_NONE);
  stp_curve_get_bounds(curve, &blo, &bhi);
  stp_curve_set_bounds(retval, blo, bhi);
  data = stp_curve_get_data(curve, &ncount);
  if (! stp_curve_set_data(retval, count, data + start))
    {
      stp_curve_destroy(retval);
      return NULL;
    }
  return retval;
}

int
stp_curve_set_subrange(stp_curve_t *curve, const stp_curve_t *range,
		       size_t start)
{
  double blo, bhi;
  double rlo, rhi;
  const double *data;
  size_t count;
  CHECK_CURVE(curve);
  if (start + stp_curve_count_points(range) > stp_curve_count_points(curve))
    return 0;
  if (curve->piecewise)
    return 0;
  stp_sequence_get_bounds(curve->seq, &blo, &bhi);
  stp_sequence_get_range(curve->seq, &rlo, &rhi);
  if (rlo < blo || rhi > bhi)
    return 0;
  stp_sequence_get_data(range->seq, &count, &data);
  curve->recompute_interval = 1;
  curve->gamma = 0.0;
  invalidate_auxiliary_data(curve);
  stp_sequence_set_subrange(curve->seq, start, stp_curve_count_points(range),
			    data);
  return 1;
}


int
stp_curve_set_point(stp_curve_t *curve, size_t where, double data)
{
  CHECK_CURVE(curve);
  if (where >= get_point_count(curve))
    return 0;
  curve->gamma = 0.0;

  if (curve->piecewise)
    return 0;
  if ((stp_sequence_set_point(curve->seq, where, data)) == 0)
    return 0;
  if (where == 0 && curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    if ((stp_sequence_set_point(curve->seq,
				get_point_count(curve), data)) == 0)
      return 0;
  invalidate_auxiliary_data(curve);
  return 1;
}

int
stp_curve_get_point(const stp_curve_t *curve, size_t where, double *data)
{
  CHECK_CURVE(curve);
  if (where >= get_point_count(curve))
    return 0;
  if (curve->piecewise)
    return 0;
  return stp_sequence_get_point(curve->seq, where, data);
}

const stp_sequence_t *
stp_curve_get_sequence(const stp_curve_t *curve)
{
  CHECK_CURVE(curve);
  if (curve->piecewise)
    return NULL;
  return curve->seq;
}

int
stp_curve_rescale(stp_curve_t *curve, double scale,
		  stp_curve_compose_t mode, stp_curve_bounds_t bounds_mode)
{
  size_t real_point_count;
  int i;
  double nblo;
  double nbhi;
  size_t count;

  CHECK_CURVE(curve);

  real_point_count = get_real_point_count(curve);

  stp_sequence_get_bounds(curve->seq, &nblo, &nbhi);
  if (bounds_mode == STP_CURVE_BOUNDS_RESCALE)
    {
      switch (mode)
	{
	case STP_CURVE_COMPOSE_ADD:
	  nblo += scale;
	  nbhi += scale;
	  break;
	case STP_CURVE_COMPOSE_MULTIPLY:
	  if (scale < 0)
	    {
	      double tmp = nblo * scale;
	      nblo = nbhi * scale;
	      nbhi = tmp;
	    }
	  else
	    {
	      nblo *= scale;
	      nbhi *= scale;
	    }
	  break;
	case STP_CURVE_COMPOSE_EXPONENTIATE:
	  if (scale == 0.0)
	    return 0;
	  if (nblo < 0)
	    return 0;
	  nblo = pow(nblo, scale);
	  nbhi = pow(nbhi, scale);
	  break;
	default:
	  return 0;
	}
    }

  if (! isfinite(nbhi) || ! isfinite(nblo))
    return 0;

  count = get_point_count(curve);
  if (count)
    {
      double *tmp;
      size_t scount;
      int stride = 1;
      int offset = 0;
      const double *data;
      if (curve->piecewise)
	{
	  stride = 2;
	  offset = 1;
	}
      stp_sequence_get_data(curve->seq, &scount, &data);
      tmp = stp_malloc(sizeof(double) * scount);
      memcpy(tmp, data, scount * sizeof(double));
      for (i = offset; i < scount; i += stride)
	{
	  switch (mode)
	    {
	    case STP_CURVE_COMPOSE_ADD:
	      tmp[i] = tmp[i] + scale;
	      break;
	    case STP_CURVE_COMPOSE_MULTIPLY:
	      tmp[i] = tmp[i] * scale;
	      break;
	    case STP_CURVE_COMPOSE_EXPONENTIATE:
	      tmp[i] = pow(tmp[i], scale);
	      break;
	    }
	  if (tmp[i] > nbhi || tmp[i] < nblo)
	    {
	      if (bounds_mode == STP_CURVE_BOUNDS_ERROR)
		{
		  stp_free(tmp);
		  return(0);
		}
	      else if (tmp[i] > nbhi)
		tmp[i] = nbhi;
	      else
		tmp[i] = nblo;
	    }
	}
      stp_sequence_set_bounds(curve->seq, nblo, nbhi);
      curve->gamma = 0.0;
      stpi_curve_set_points(curve, count);
      stp_sequence_set_subrange(curve->seq, 0, scount, tmp);
      stp_free(tmp);
      curve->recompute_interval = 1;
      invalidate_auxiliary_data(curve);
    }
  return 1;
}

static int
stpi_curve_check_parameters(stp_curve_t *curve, size_t points)
{
  double blo, bhi;
  if (curve->gamma && curve->wrap_mode)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "curve sets both gamma and wrap_mode\n");
      return 0;
    }
  stp_sequence_get_bounds(curve->seq, &blo, &bhi);
  if (blo > bhi)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "curve low bound is greater than high bound\n");
      return 0;
    }
  return 1;
}

static inline double
interpolate_gamma_internal(const stp_curve_t *curve, double where)
{
  double fgamma = curve->gamma;
  double blo, bhi;
  size_t real_point_count;

  real_point_count = get_real_point_count(curve);;

  if (real_point_count)
    where /= (real_point_count - 1);
  if (fgamma < 0)
    {
      where = 1.0 - where;
      fgamma = -fgamma;
    }
  stp_sequence_get_bounds(curve->seq, &blo, &bhi);
  stp_deprintf(STP_DBG_CURVE,
	       "interpolate_gamma %f %f %f %f %f\n", where, fgamma,
	       blo, bhi, pow(where, fgamma));
  return blo + (bhi - blo) * pow(where, fgamma);
}

static inline double
do_interpolate_spline(double low, double high, double frac,
		      double interval_low, double interval_high,
		      double x_interval)
{
  double a = 1.0 - frac;
  double b = frac;
  double retval = 
    ((a * a * a - a) * interval_low) + ((b * b * b - b) * interval_high);
  retval = retval * x_interval * x_interval / 6;
  retval += (a * low) + (b * high);
  return retval;
}

static inline double
interpolate_point_internal(stp_curve_t *curve, double where)
{
  int integer = where;
  double frac = where - (double) integer;
  double bhi, blo;

  if (frac == 0.0)
    {
      double val;
      if ((stp_sequence_get_point(curve->seq, integer, &val)) == 0)
	return HUGE_VAL; /* Infinity */
      return val;
    }
  if (curve->recompute_interval)
    compute_intervals(curve);
  if (curve->curve_type == STP_CURVE_TYPE_LINEAR)
    {
      double val;
      if ((stp_sequence_get_point(curve->seq, integer, &val)) == 0)
	return HUGE_VAL; /* Infinity */
      return val + frac * curve->interval[integer];
    }
  else
    {
      size_t point_count;
      double ival, ip1val;
      double retval;
      int i = integer;
      int ip1 = integer + 1;

      point_count = get_point_count(curve);

      if (ip1 >= point_count)
	ip1 -= point_count;

      if ((stp_sequence_get_point(curve->seq, i, &ival)) == 0 ||
	  (stp_sequence_get_point(curve->seq, ip1, &ip1val)) == 0)
	return HUGE_VAL; /* Infinity */

      retval = do_interpolate_spline(ival, ip1val, frac, curve->interval[i],
				     curve->interval[ip1], 1.0);

      stp_sequence_get_bounds(curve->seq, &blo, &bhi);
      if (retval > bhi)
	retval = bhi;
      if (retval < blo)
	retval = blo;
      return retval;
    }
}

int
stp_curve_interpolate_value(const stp_curve_t *curve, double where,
			    double *result)
{
  size_t limit;

  CHECK_CURVE(curve);
  if (curve->piecewise)
    return 0;

  limit = get_real_point_count(curve);

  if (where < 0 || where > limit)
    return 0;
  if (curve->gamma)	/* this means a pure gamma curve */
    *result = interpolate_gamma_internal(curve, where);
  else
    *result = interpolate_point_internal((stp_curve_t *) curve, where);
  return 1;
}

int
stp_curve_resample(stp_curve_t *curve, size_t points)
{
  size_t limit = points;
  size_t old;
  size_t i;
  double *new_vec;

  CHECK_CURVE(curve);

  if (points == get_point_count(curve) && curve->seq && !(curve->piecewise))
    return 1;

  if (points < 2)
    return 1;

  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    limit++;
  if (limit > curve_point_limit)
    return 0;
  old = get_real_point_count(curve);
  if (old)
    old--;
  if (!old)
    old = 1;

  new_vec = stp_malloc(sizeof(double) * limit);

  /*
   * Be very careful how we calculate the location along the scale!
   * If we're not careful how we do it, we might get a small roundoff
   * error
   */
  if (curve->piecewise)
    {
      double blo, bhi;
      int curpos = 0;
      stp_sequence_get_bounds(curve->seq, &blo, &bhi);
      if (curve->recompute_interval)
	compute_intervals(curve);
      for (i = 0; i < old; i++)
	{
	  double low;
	  double high;
	  double low_y;
	  double high_y;
	  double x_delta;
	  if (!stp_sequence_get_point(curve->seq, i * 2, &low))
	    {
	      stp_free(new_vec);
	      return 0;
	    }
	  if (i == old - 1)
	    high = 1.0;
	  else if (!stp_sequence_get_point(curve->seq, ((i + 1) * 2), &high))
	    {
	      stp_free(new_vec);
	      return 0;
	    }
	  if (!stp_sequence_get_point(curve->seq, (i * 2) + 1, &low_y))
	    {
	      stp_free(new_vec);
	      return 0;
	    }
	  if (!stp_sequence_get_point(curve->seq, ((i + 1) * 2) + 1, &high_y))
	    {
	      stp_free(new_vec);
	      return 0;
	    }
	  stp_deprintf(STP_DBG_CURVE,
		       "Filling slots at %ld %d: %f %f  %f %f  %ld\n",
		       (long)i,curpos, high, low, high_y, low_y, (long)limit);
	  x_delta = high - low;
	  high *= (limit - 1);
	  low *= (limit - 1);
	  while (curpos <= high)
	    {
	      double frac = (curpos - low) / (high - low);
	      if (curve->curve_type == STP_CURVE_TYPE_LINEAR)
		new_vec[curpos] = low_y + frac * (high_y - low_y);
	      else
		new_vec[curpos] =
		  do_interpolate_spline(low_y, high_y, frac,
					curve->interval[i],
					curve->interval[i + 1],
					x_delta);
	      if (new_vec[curpos] < blo)
		new_vec[curpos] = blo;
	      if (new_vec[curpos] > bhi)
		new_vec[curpos] = bhi;
	      stp_deprintf(STP_DBG_CURVE,
			   "  Filling slot %d %f %f\n",
			   curpos, frac, new_vec[curpos]);
	      curpos++;
	    }
	}
      curve->piecewise = 0;
    }
  else
    {
      for (i = 0; i < limit; i++)
	if (curve->gamma)
	  new_vec[i] =
	    interpolate_gamma_internal(curve, ((double) i * (double) old /
					       (double) (limit - 1)));
	else
	  new_vec[i] =
	    interpolate_point_internal(curve, ((double) i * (double) old /
					       (double) (limit - 1)));
    }
  stpi_curve_set_points(curve, points);
  stp_sequence_set_subrange(curve->seq, 0, limit, new_vec);
  curve->recompute_interval = 1;
  stp_free(new_vec);
  return 1;
}

static unsigned
gcd(unsigned a, unsigned b)
{
  unsigned tmp;
  if (b > a)
    {
      tmp = a;
      a = b;
      b = tmp;
    }
  while (1)
    {
      tmp = a % b;
      if (tmp == 0)
	return b;
      a = b;
      b = tmp;
    }
}

static unsigned
lcm(unsigned a, unsigned b)
{
  if (a == b)
    return a;
  else if (a * b == 0)
    return a > b ? a : b;
  else
    {
      double rval = (double) a / gcd(a, b) * b;
      if (rval > curve_point_limit)
	return curve_point_limit;
      else
	return rval;
    }
}

static int
create_gamma_curve(stp_curve_t **retval, double lo, double hi, double fgamma,
		   int points)
{
  *retval = stp_curve_create(STP_CURVE_WRAP_NONE);
  if (stp_curve_set_bounds(*retval, lo, hi) &&
      stp_curve_set_gamma(*retval, fgamma) &&
      stp_curve_resample(*retval, points))
    return 1;
  stp_curve_destroy(*retval);
  *retval = 0;
  return 0;
}

static int
interpolate_points(stp_curve_t *a, stp_curve_t *b,
		   stp_curve_compose_t mode,
		   int points, double *tmp_data)
{
  double pa, pb;
  int i;
  size_t points_a = stp_curve_count_points(a);
  size_t points_b = stp_curve_count_points(b);
  for (i = 0; i < points; i++)
    {
      if (!stp_curve_interpolate_value
	  (a, (double) i * (points_a - 1) / (points - 1), &pa))
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "interpolate_points: interpolate curve a value failed\n");
	  return 0;
	}
      if (!stp_curve_interpolate_value
	  (b, (double) i * (points_b - 1) / (points - 1), &pb))
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "interpolate_points: interpolate curve b value failed\n");
	  return 0;
	}
      if (mode == STP_CURVE_COMPOSE_ADD)
	pa += pb;
      else
	pa *= pb;
      if (! isfinite(pa))
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "interpolate_points: interpolated point %lu is invalid\n",
		       (unsigned long) i);
	  return 0;
	}
      tmp_data[i] = pa;
    }
  return 1;
}

int
stp_curve_compose(stp_curve_t **retval,
		  stp_curve_t *a, stp_curve_t *b,
		  stp_curve_compose_t mode, int points)
{
  stp_curve_t *ret;
  double *tmp_data;
  double gamma_a = stp_curve_get_gamma(a);
  double gamma_b = stp_curve_get_gamma(b);
  unsigned points_a = stp_curve_count_points(a);
  unsigned points_b = stp_curve_count_points(b);
  double alo, ahi, blo, bhi;

  if (a->piecewise && b->piecewise)
    return 0;
  if (a->piecewise)
    {
      stp_curve_t *a_save = a;
      a = stp_curve_create_copy(a_save);
      stp_curve_resample(a, stp_curve_count_points(b));
    }
  if (b->piecewise)
    {
      stp_curve_t *b_save = b;
      b = stp_curve_create_copy(b_save);
      stp_curve_resample(b, stp_curve_count_points(a));
    }

  if (mode != STP_CURVE_COMPOSE_ADD && mode != STP_CURVE_COMPOSE_MULTIPLY)
    return 0;
  if (stp_curve_get_wrap(a) != stp_curve_get_wrap(b))
    return 0;
  stp_curve_get_bounds(a, &alo, &ahi);
  stp_curve_get_bounds(b, &blo, &bhi);
  if (mode == STP_CURVE_COMPOSE_MULTIPLY && (alo < 0 || blo < 0))
    return 0;

  if (stp_curve_get_wrap(a) == STP_CURVE_WRAP_AROUND)
    {
      points_a++;
      points_b++;
    }
  if (points == -1)
    {
      points = lcm(points_a, points_b);
      if (stp_curve_get_wrap(a) == STP_CURVE_WRAP_AROUND)
	points--;
    }
  if (points < 2 || points > curve_point_limit ||
      ((stp_curve_get_wrap(a) == STP_CURVE_WRAP_AROUND) &&
       points > curve_point_limit - 1))
    return 0;

  if (gamma_a && gamma_b && gamma_a * gamma_b > 0 &&
      mode == STP_CURVE_COMPOSE_MULTIPLY)
    return create_gamma_curve(retval, alo * blo, ahi * bhi, gamma_a + gamma_b,
			      points);
  tmp_data = stp_malloc(sizeof(double) * points);
  if (!interpolate_points(a, b, mode, points, tmp_data))
    {
      stp_free(tmp_data);
      return 0;
    }
  ret = stp_curve_create(stp_curve_get_wrap(a));
  if (mode == STP_CURVE_COMPOSE_ADD)
    {
      stp_curve_rescale(ret, (ahi - alo) + (bhi - blo),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_RESCALE);
      stp_curve_rescale(ret, alo + blo,
			STP_CURVE_COMPOSE_ADD, STP_CURVE_BOUNDS_RESCALE);
    }
  else
    {
      stp_curve_rescale(ret, (ahi - alo) * (bhi - blo),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_RESCALE);
      stp_curve_rescale(ret, alo * blo,
			STP_CURVE_COMPOSE_ADD, STP_CURVE_BOUNDS_RESCALE);
    }
  if (! stp_curve_set_data(ret, points, tmp_data))
    goto bad1;
  *retval = ret;
  stp_free(tmp_data);
  return 1;
 bad1:
  stp_curve_destroy(ret);
  stp_free(tmp_data);
  return 0;
}


stp_curve_t *
stp_curve_create_from_xmltree(stp_mxml_node_t *curve)  /* The curve node */
{
  const char *stmp;                       /* Temporary string */
  stp_mxml_node_t *child;                 /* Child sequence node */
  stp_curve_t *ret = NULL;                /* Curve to return */
  stp_curve_type_t curve_type;            /* Type of curve */
  stp_curve_wrap_mode_t wrap_mode;        /* Curve wrap mode */
  double fgamma;                          /* Gamma value */
  stp_sequence_t *seq = NULL;             /* Sequence data */
  double low, high;                       /* Sequence bounds */
  int piecewise = 0;

  stp_xml_init();
  /* Get curve type */
  stmp = stp_mxmlElementGetAttr(curve, "type");
  if (stmp)
    {
      if (!strcmp(stmp, "linear"))
	  curve_type = STP_CURVE_TYPE_LINEAR;
      else if (!strcmp(stmp, "spline"))
	  curve_type = STP_CURVE_TYPE_SPLINE;
      else
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "stp_curve_create_from_xmltree: %s: \"type\" invalid\n", stmp);
	  goto error;
	}
    }
  else
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "stp_curve_create_from_xmltree: \"type\" missing\n");
      goto error;
    }
  /* Get curve wrap mode */
  stmp = stp_mxmlElementGetAttr(curve, "wrap");
  if (stmp)
    {
      if (!strcmp(stmp, "nowrap"))
	wrap_mode = STP_CURVE_WRAP_NONE;
      else if (!strcmp(stmp, "wrap"))
	{
	  wrap_mode = STP_CURVE_WRAP_AROUND;
	}
      else
	{
	  stp_deprintf(STP_DBG_CURVE_ERRORS,
		       "stp_curve_create_from_xmltree: %s: \"wrap\" invalid\n", stmp);
	  goto error;
	}
    }
  else
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "stp_curve_create_from_xmltree: \"wrap\" missing\n");
      goto error;
    }
  /* Get curve gamma */
  stmp = stp_mxmlElementGetAttr(curve, "gamma");
  if (stmp)
    {
      fgamma = stp_xmlstrtod(stmp);
    }
  else
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "stp_curve_create_from_xmltree: \"gamma\" missing\n");
      goto error;
    }
  /* If gamma is set, wrap_mode must be STP_CURVE_WRAP_NONE */
  if (fgamma && wrap_mode != STP_CURVE_WRAP_NONE)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS, "stp_curve_create_from_xmltree: "
		   "gamma set and \"wrap\" is not STP_CURVE_WRAP_NONE\n");
      goto error;
    }
  stmp = stp_mxmlElementGetAttr(curve, "piecewise");
  if (stmp && strcmp(stmp, "true") == 0)
    piecewise = 1;

  /* Set up the curve */
  ret = stp_curve_create(wrap_mode);
  stp_curve_set_interpolation_type(ret, curve_type);

  child = stp_mxmlFindElement(curve, curve, "sequence", NULL, NULL, STP_MXML_DESCEND);
  if (child)
    seq = stp_sequence_create_from_xmltree(child);

  if (seq == NULL)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "stp_curve_create_from_xmltree: sequence read failed\n");
      goto error;
    }

  /* Set curve bounds */
  stp_sequence_get_bounds(seq, &low, &high);
  stp_curve_set_bounds(ret, low, high);

  if (fgamma)
    stp_curve_set_gamma(ret, fgamma);
  else /* Not a gamma curve, so set points */
    {
      size_t seq_count;
      const double* data;

      stp_sequence_get_data(seq, &seq_count, &data);
      if (piecewise)
	{
	  if ((seq_count % 2) != 0)
	    {
	      stp_deprintf(STP_DBG_CURVE_ERRORS,
			   "stp_curve_create_from_xmltree: invalid data count %ld\n",
			   (long)seq_count);
	      goto error;
	    }
	  if (stp_curve_set_data_points(ret, seq_count / 2,
					(const stp_curve_point_t *) data) == 0)
	    {
	      stp_deprintf(STP_DBG_CURVE_ERRORS,
			   "stp_curve_create_from_xmltree: failed to set curve data points\n");
	      goto error;
	    }
	}
      else
	{
	  if (stp_curve_set_data(ret, seq_count, data) == 0)
	    {
	      stp_deprintf(STP_DBG_CURVE_ERRORS,
			   "stp_curve_create_from_xmltree: failed to set curve data\n");
	      goto error;
	    }
	}
    }

  if (seq)
    {
      stp_sequence_destroy(seq);
      seq = NULL;
    }

    /* Validate curve */
  if (stpi_curve_check_parameters(ret, stp_curve_count_points(ret)) == 0)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "stp_curve_create_from_xmltree: parameter check failed\n");
      goto error;
    }

  stp_xml_exit();

  return ret;

 error:
  stp_deprintf(STP_DBG_CURVE_ERRORS,
	       "stp_curve_create_from_xmltree: error during curve read\n");
  if (ret)
    stp_curve_destroy(ret);
  stp_xml_exit();
  return NULL;
}


stp_mxml_node_t *
stp_xmltree_create_from_curve(const stp_curve_t *curve)  /* The curve */
{
  stp_curve_wrap_mode_t wrapmode;
  stp_curve_type_t interptype;
  double gammaval, low, high;
  stp_sequence_t *seq;

  char *cgamma;

  stp_mxml_node_t *curvenode = NULL;
  stp_mxml_node_t *child = NULL;

  stp_xml_init();

  /* Get curve details */
  wrapmode = stp_curve_get_wrap(curve);
  interptype = stp_curve_get_interpolation_type(curve);
  gammaval = stp_curve_get_gamma(curve);

  if (gammaval && wrapmode != STP_CURVE_WRAP_NONE)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS, "stp_xmltree_create_from_curve: "
		   "curve sets gamma and wrap_mode is not STP_CURVE_WRAP_NONE\n");
      goto error;
    }

  /* Construct the allocated strings required */
  stp_asprintf(&cgamma, "%g", gammaval);

  curvenode = stp_mxmlNewElement(NULL, "curve");
  stp_mxmlElementSetAttr(curvenode, "wrap", stpi_wrap_mode_names[wrapmode]);
  stp_mxmlElementSetAttr(curvenode, "type", stpi_curve_type_names[interptype]);
  stp_mxmlElementSetAttr(curvenode, "gamma", cgamma);
  if (curve->piecewise)
    stp_mxmlElementSetAttr(curvenode, "piecewise", "true");
  else
    stp_mxmlElementSetAttr(curvenode, "piecewise", "false");

  stp_free(cgamma);

  seq = stp_sequence_create();
  stp_curve_get_bounds(curve, &low, &high);
  stp_sequence_set_bounds(seq, low, high);
  if (gammaval != 0) /* A gamma curve does not require sequence data */
    {
      stp_sequence_set_size(seq, 0);
    }
  else
    {
      const double *data;
      size_t count;
      data = stpi_curve_get_data_internal(curve, &count);
      stp_sequence_set_data(seq, count, data);
    }

  child = stp_xmltree_create_from_sequence(seq);

  if (seq)
    {
      stp_sequence_destroy(seq);
      seq = NULL;
    }

  if (child == NULL)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "stp_xmltree_create_from_curve: sequence node is NULL\n");
      goto error;
    }
  stp_mxmlAdd(curvenode, STP_MXML_ADD_AFTER, NULL, child);

  stp_xml_exit();

  return curvenode;

 error:
  stp_deprintf(STP_DBG_CURVE_ERRORS,
	       "stp_xmltree_create_from_curve: error during xmltree creation\n");
  if (curvenode)
    stp_mxmlDelete(curvenode);
  if (child)
    stp_mxmlDelete(child);
  stp_xml_exit();

  return NULL;
}

static stp_mxml_node_t *
xmldoc_create_from_curve(const stp_curve_t *curve)
{
  stp_mxml_node_t *xmldoc;
  stp_mxml_node_t *rootnode;
  stp_mxml_node_t *curvenode;

  /* Get curve details */
  curvenode = stp_xmltree_create_from_curve(curve);
  if (curvenode == NULL)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "xmldoc_create_from_curve: error creating curve node\n");
      return NULL;
    }
  /* Create the XML tree */
  xmldoc = stp_xmldoc_create_generic();
  if (xmldoc == NULL)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "xmldoc_create_from_curve: error creating XML document\n");
      return NULL;
    }
  rootnode = xmldoc->child;
  if (rootnode == NULL)
    {
      stp_mxmlDelete(xmldoc);
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "xmldoc_create_from_curve: error getting XML document root node\n");
      return NULL;
    }

  stp_mxmlAdd(rootnode, STP_MXML_ADD_AFTER, NULL, curvenode);

  return xmldoc;
}

static int
curve_whitespace_callback(stp_mxml_node_t *node, int where)
{
  if (node->type != STP_MXML_ELEMENT)
    return 0;
  if (strcasecmp(node->value.element.name, "gutenprint") == 0)
    {
      switch (where)
	{
	case STP_MXML_WS_AFTER_OPEN:
	case STP_MXML_WS_BEFORE_CLOSE:
	case STP_MXML_WS_AFTER_CLOSE:
	  return '\n';
	case STP_MXML_WS_BEFORE_OPEN:
	default:
	  return 0;
	}
    }
  else if (strcasecmp(node->value.element.name, "curve") == 0)
    {
      switch (where)
	{
	case STP_MXML_WS_AFTER_OPEN:
	  return '\n';
	case STP_MXML_WS_BEFORE_CLOSE:
	case STP_MXML_WS_AFTER_CLOSE:
	case STP_MXML_WS_BEFORE_OPEN:
	default:
	  return 0;
	}
    }
  else if (strcasecmp(node->value.element.name, "sequence") == 0)
    {
      const char *count;
      switch (where)
	{
	case STP_MXML_WS_BEFORE_CLOSE:
	  count = stp_mxmlElementGetAttr(node, "count");
	  if (strcmp(count, "0") == 0)
	    return 0;
	  else
	    return '\n';
	case STP_MXML_WS_AFTER_OPEN:
	case STP_MXML_WS_AFTER_CLOSE:
	  return '\n';
	case STP_MXML_WS_BEFORE_OPEN:
	default:
	  return 0;
	}
    }
  else
    return 0;
}


int
stp_curve_write(FILE *file, const stp_curve_t *curve)  /* The curve */
{
  stp_mxml_node_t *xmldoc = NULL;

  stp_xml_init();

  xmldoc = xmldoc_create_from_curve(curve);
  if (xmldoc == NULL)
    {
      stp_xml_exit();
      return 1;
    }

  stp_mxmlSaveFile(xmldoc, file, curve_whitespace_callback);

  if (xmldoc)
    stp_mxmlDelete(xmldoc);

  stp_xml_exit();

  return 0;
}

char *
stp_curve_write_string(const stp_curve_t *curve)  /* The curve */
{
  stp_mxml_node_t *xmldoc = NULL;
  char *retval;

  stp_xml_init();

  xmldoc = xmldoc_create_from_curve(curve);
  if (xmldoc == NULL)
    {
      stp_xml_exit();
      return NULL;
    }

  retval = stp_mxmlSaveAllocString(xmldoc, curve_whitespace_callback);

  if (xmldoc)
    stp_mxmlDelete(xmldoc);

  stp_xml_exit();

  return retval;
}

static stp_curve_t *
xml_doc_get_curve(stp_mxml_node_t *doc)
{
  stp_mxml_node_t *cur;
  stp_mxml_node_t *xmlcurve;
  stp_curve_t *curve = NULL;

  if (doc == NULL )
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "xml_doc_get_curve: XML file not parsed successfully.\n");
      return NULL;
    }

  cur = doc->child;

  if (cur == NULL)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "xml_doc_get_curve: empty document\n");
      return NULL;
    }

  xmlcurve = stp_xml_get_node(cur, "gutenprint", "curve", NULL);

  if (xmlcurve)
    curve = stp_curve_create_from_xmltree(xmlcurve);

  return curve;
}

stp_curve_t *
stp_curve_create_from_file(const char* file)
{
  stp_curve_t *curve = NULL;
  stp_mxml_node_t *doc;
  FILE *fp = fopen(file, "r");
  if (!fp)
    {
      stp_deprintf(STP_DBG_CURVE_ERRORS,
		   "stp_curve_create_from_file: unable to open %s: %s\n",
		    file, strerror(errno));
      return NULL;
    }
  stp_deprintf(STP_DBG_XML, "stp_curve_create_from_file: reading `%s'...\n",
	       file);

  stp_xml_init();

  doc = stp_mxmlLoadFile(NULL, fp, STP_MXML_NO_CALLBACK);

  curve = xml_doc_get_curve(doc);

  if (doc)
    stp_mxmlDelete(doc);

  stp_xml_exit();
  (void) fclose(fp);
  return curve;

}

stp_curve_t *
stp_curve_create_from_stream(FILE* fp)
{
  stp_curve_t *curve = NULL;
  stp_mxml_node_t *doc;
  stp_deprintf(STP_DBG_XML, "stp_curve_create_from_fp: reading...\n");

  stp_xml_init();

  doc = stp_mxmlLoadFile(NULL, fp, STP_MXML_NO_CALLBACK);

  curve = xml_doc_get_curve(doc);

  if (doc)
    stp_mxmlDelete(doc);

  stp_xml_exit();
  return curve;

}

stp_curve_t *
stp_curve_create_from_string(const char* string)
{
  stp_curve_t *curve = NULL;
  stp_mxml_node_t *doc;
  stp_deprintf(STP_DBG_XML,
	       "stp_curve_create_from_string: reading '%s'...\n", string);
  stp_xml_init();

  doc = stp_mxmlLoadString(NULL, string, STP_MXML_NO_CALLBACK);

  curve = xml_doc_get_curve(doc);

  if (doc)
    stp_mxmlDelete(doc);

  stp_xml_exit();
  return curve;
}
