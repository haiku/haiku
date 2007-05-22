/***********************************************************************
 *                                                                     *
 * $Id: hpgsbezier.c 270 2006-01-29 21:12:23Z softadm $
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
 * Bezier spline length and scanline cut implementation.               *
 *                                                                     *
 ***********************************************************************/

#include <hpgspaint.h>
#include <math.h>
#include <assert.h>

// #define HPGS_BEZIER_DEBUG

/*! Calculates the derivation of the bezier curve starting
    at point \c i of \c path at curve parameter \c t.
*/
void hpgs_bezier_path_delta(const hpgs_paint_path *path, int i,
			    double t, hpgs_point *p)
{
  double tp,tm;

  tp = t + 0.5;
  tm = 0.5 - t;

  p->x = 3.0* ((path->points[i+1].p.x-path->points[i].p.x)*tm*tm+
	       2.0 * (path->points[i+2].p.x-path->points[i+1].p.x) * tm*tp+
	       (path->points[i+3].p.x-path->points[i+2].p.x) * tp*tp);

  p->y = 3.0 * ((path->points[i+1].p.y-path->points[i].p.y)*tm*tm+
		2.0 * (path->points[i+2].p.y-path->points[i+1].p.y) * tm*tp+
		(path->points[i+3].p.y-path->points[i+2].p.y) * tp*tp);
}

/*! Calculates the derivation of the bezier curve starting
    at point \c i of \c path at curve parameter \c t.
*/
double hpgs_bezier_path_delta_x(const hpgs_paint_path *path, int i, double t)
{
  double tp,tm;

  tp = t + 0.5;
  tm = 0.5 - t;

  return 3.0* ((path->points[i+1].p.x-path->points[i].p.x)*tm*tm+
	       2.0 * (path->points[i+2].p.x-path->points[i+1].p.x) * tm*tp+
	       (path->points[i+3].p.x-path->points[i+2].p.x) * tp*tp);
}

/*! Calculates the derivation of the bezier curve starting
    at point \c i of \c path at curve parameter \c t.
*/
double hpgs_bezier_path_delta_y(const hpgs_paint_path *path, int i, double t)
{
  double tp,tm;

  tp = t + 0.5;
  tm = 0.5 - t;

  return 3.0* ((path->points[i+1].p.y-path->points[i].p.y)*tm*tm+
	       2.0 * (path->points[i+2].p.y-path->points[i+1].p.y) * tm*tp+
	       (path->points[i+3].p.y-path->points[i+2].p.y) * tp*tp);
}

/*! Calculates the tangent to the bezier curve starting
    at point \c i of \c path at curve parameter \c t.

    If \c orientation is \c 1, the tangent is calculated with increasing
    curve parameter, if we meet a singularity (right tangent).

    If \c orientation is \c -1, the tangent is calculated with decreasing
    curve parameter, if we meet a singularity (left tangent).

    If \c orientation is \c 0, the tangent is calculated with decreased
    and increased curve parameter, if we meet a singularity (mid tangent).
*/

void hpgs_bezier_path_tangent(const hpgs_paint_path *path, int i,
                              double t, int orientation,
                              double ytol, hpgs_point *p)
{
  hpgs_bezier_path_delta(path,i,t,p);

  double l = hypot(p->x,p->y);

  if (l < ytol)
    {
      hpgs_point p1;
 
      if (orientation <= 0)
        hpgs_bezier_path_point(path,i,t-1.0e-4,&p1);
      else
        hpgs_bezier_path_point(path,i,t,&p1);

      if (orientation >= 0)
        hpgs_bezier_path_point(path,i,t+1.0e-4,p);
      else
        hpgs_bezier_path_point(path,i,t,p);

      p->x -= p1.x;
      p->y -= p1.y;

      l = hypot(p->x,p->y);
      if (l < ytol*1.0e-4) return;
    }

  p->x /= l;
  p->y /= l;
}

static int quad_roots (double a, double b, double c,
                       double t0, double t1, double *t)
{
  if (fabs(a) < (fabs(b)>fabs(c) ? fabs(b) : fabs(c)) * 1.0e-8 || fabs(a) < 1.0e-16)
    {
      if (fabs(b) < fabs(c) * 1.0e-8)
        return 0;

      t[0] = -c/b;
      return t[0] > t0 && t[0] < t1;
    }

  double p = b/a;
  double q = c/a;

  double det = 0.25*p-q;

  if (det < 0) return 0;

  det = sqrt(det);

  if (p > 0.0)
    t[0] = -0.5*p - det;
  else
    t[0] = -0.5*p + det;

  if (fabs(t[0]) < 1.0e-8)
    return t[0] > t0 && t[0] < t1;

  t[1] = q / t[0];

  if (t[0] <= t0 || t[0] >= t1)
    {
      t[0] = t[1];
      return t[0] > t0 && t[0] < t1;
    }

  if (t[1] <= t0 || t[1] >= t1)
    return 1;

  if (t[0] > t[1])
    {
      double tmp = t[1];
      t[1] = t[0];
      t[0] = tmp;
    }

  return fabs(t[0]-t[1]) < 1.0e-8 ? 1 : 2;
}


/*!
    Calculates the singularities of the bezier curve in the
    parameter interval from \c t0 to \c t1.

    We calculate points, where the cross product of the first derivative
    and the second derivative vanishes. These points include points, where
    the first or second derivative vanishes. Additionally, turning points
    of the curve are also included.

    \c tx is a pointer to an array of at least two double values,
    in which the position of the singularities is returned.
 */
void hpgs_bezier_path_singularities(const hpgs_paint_path *path, int i,
                                    double t0, double t1,
                                    int *nx, double *tx)
{
  // The second order spline p'(t) cross p''(t)
  double k0 =
    (path->points[i+1].p.x-path->points[i].p.x)*(path->points[i+2].p.y-path->points[i+1].p.y) -
    (path->points[i+1].p.y-path->points[i].p.y)*(path->points[i+2].p.x-path->points[i+1].p.x);

  double k1 =
    (path->points[i+1].p.x-path->points[i].p.x)*(path->points[i+3].p.y-path->points[i+2].p.y) -
    (path->points[i+1].p.y-path->points[i].p.y)*(path->points[i+3].p.x-path->points[i+2].p.x);

  double k2 =
    (path->points[i+2].p.x-path->points[i+1].p.x)*(path->points[i+3].p.y-path->points[i+2].p.y) -
    (path->points[i+2].p.y-path->points[i+1].p.y)*(path->points[i+3].p.x-path->points[i+2].p.x);


  // coefficients for the quadratic equations.
  double a = k0 - k1 + k2;

  double b = k2 - k0;

  double c = 0.25 * (k0 + k1 + k2);

  *nx = quad_roots (a,b,c,t0,t1,tx);
}

static void add_quad (const hpgs_point *p1,
                      const hpgs_point *d1,
                      const hpgs_point *p2,
                      const hpgs_point *d2,
                      int *nx, hpgs_point *points)
{
  double det = d1->y * d2->x - d1->x * d2->y;

  if (fabs(det) < 1.0e-8)
    {
      points[*nx].x = 0.5 * (p1->x + p2->x);
      points[*nx].y = 0.5 * (p1->y + p2->y);
      ++*nx;
    }
  else
    {
      double s = (d2->x*(p2->y-p1->y)-
                  d2->y*(p2->x-p1->x)  )/det;

      points[*nx].x = p1->x + s*d1->x;
      points[*nx].y = p1->y + s*d1->y;
      ++*nx;
    }

  points[*nx].x = p2->x;
  points[*nx].y = p2->y;
  ++*nx;
}

/*!
    Approximates the bezier curve segment in the
    parameter interval from \c t0 to \c t1 with quadratic bezier splines.

    The array \c points is used to return the quadratic bezier segments and
    must have a dimension of at least 16 points.

    \c nx returns the number of generated points, which is always an even
    number. The first point is the control point of the first quadratic
    spline, the second point the endpoint of the first quadratic spline
    and so on.
 */
void hpgs_bezier_path_to_quadratic(const hpgs_paint_path *path, int i,
                                   double t0, double t1,
                                   int *nx, hpgs_point *points)
{
  // The second order spline p'(t) cross p''(t)
  double k0 =
    (path->points[i+1].p.x-path->points[i].p.x)*(path->points[i+2].p.y-path->points[i+1].p.y) -
    (path->points[i+1].p.y-path->points[i].p.y)*(path->points[i+2].p.x-path->points[i+1].p.x);

  double k1 =
    (path->points[i+1].p.x-path->points[i].p.x)*(path->points[i+3].p.y-path->points[i+2].p.y) -
    (path->points[i+1].p.y-path->points[i].p.y)*(path->points[i+3].p.x-path->points[i+2].p.x);

  double k2 =
    (path->points[i+2].p.x-path->points[i+1].p.x)*(path->points[i+3].p.y-path->points[i+2].p.y) -
    (path->points[i+2].p.y-path->points[i+1].p.y)*(path->points[i+3].p.x-path->points[i+2].p.x);


  double xmax,xmin,ymin,ymax;

  // coefficients for the quadratic equations.
  double a = k0 - k1 + k2;

  double b = k2 - k0;

  double c = 0.25 * (k0 + k1 + k2);

  // at most three partition points:
  // extrem, roots of the curvature spline
  double tpart[5];

  // get the roots.
  int ipart;
  int npart = quad_roots (a,b,c,t0,t1,tpart+1) + 1;

  // add the extreme point.
  if (fabs(a) > fabs(b))
    {
      double text = -0.5 * b/a;

      if (npart == 3)
        {
          tpart[3] = tpart[2];
          tpart[2] = text;
          npart = 3;
        }
      else if (text > t0 && text < t1)
        {
          if (npart == 2 && text < tpart[1])
            {
              tpart[2] = tpart[1];
              tpart[1] = text;
            }
          else
            tpart[npart] = text;

          ++npart;
        }
    }

  tpart[0] = t0;
  tpart[npart] = t1;

  xmax = xmin = path->points[i].p.x;
  if (path->points[i+1].p.x > xmax) xmax = path->points[i+1].p.x;
  if (path->points[i+2].p.x > xmax) xmax = path->points[i+2].p.x;
  if (path->points[i+3].p.x > xmax) xmax = path->points[i+3].p.x;
  if (path->points[i+1].p.x < xmin) xmin = path->points[i+1].p.x;
  if (path->points[i+2].p.x < xmin) xmin = path->points[i+2].p.x;
  if (path->points[i+3].p.x < xmin) xmin = path->points[i+3].p.x;

  ymax = ymin = path->points[i].p.y;
  if (path->points[i+1].p.y > ymax) ymax = path->points[i+1].p.y;
  if (path->points[i+2].p.y > ymax) ymax = path->points[i+2].p.y;
  if (path->points[i+3].p.y > ymax) ymax = path->points[i+3].p.y;
  if (path->points[i+1].p.y < ymin) ymin = path->points[i+1].p.y;
  if (path->points[i+2].p.y < ymin) ymin = path->points[i+2].p.y;
  if (path->points[i+3].p.y < ymin) ymin = path->points[i+3].p.y;

  double curv0 =
    k0*(0.5-tpart[0])*(0.5-tpart[0]) + 
    k1*(0.5-tpart[0])*(0.5+tpart[0]) + 
    k2*(0.5+tpart[0])*(0.5+tpart[0]);

  *nx = 0;

  for (ipart=0; ipart<npart; ++ipart)
    {
      double curv1 =
        k0*(0.5-tpart[ipart+1])*(0.5-tpart[ipart+1]) + 
        k1*(0.5-tpart[ipart+1])*(0.5+tpart[ipart+1]) + 
        k2*(0.5+tpart[ipart+1])*(0.5+tpart[ipart+1]);

      int n_splines = 1;

      if (fabs(xmax-xmin)<1.0e-8 && fabs(ymax-ymin) < 1.0e-8)
        n_splines = (int)(16.0 * fabs(curv0-curv1)/((xmax-xmin)*(ymax-ymin)));
      
      int j;

      if (n_splines < 1)            n_splines = 1;
      else if (n_splines > 8/npart) n_splines = 8/npart;

      hpgs_point d1;
      hpgs_point p1;

      hpgs_bezier_path_point(path,i,tpart[ipart],&p1);
      hpgs_bezier_path_tangent(path,i,tpart[ipart],1,1.0e-3,&d1);

      for (j=1;j<=n_splines;++j)
        {
          double t = tpart[ipart] + (tpart[ipart+1]-tpart[ipart]) *  j / (double)n_splines;

          hpgs_point d2;
          hpgs_point p2;

          hpgs_bezier_path_point(path,i,t,&p2);
          hpgs_bezier_path_tangent(path,i,t,j<n_splines ? 0 : -1,1.0e-3,&d2);

          add_quad(&p1,&d1,&p2,&d2,nx,points);

          p1 = p2;
          d1 = d2;
       }
      
      curv0 = curv1;
    }
}

/* Internal: Calculates the derivation of the curve length
     of the bezier curve \c b at curve parameter \c t.
*/
static double bezier_dl(const hpgs_paint_path *path, int i, double t)
{
  double tp,tm;

  tp = t + 0.5;
  tm = 0.5 - t;

  return 3.0*hypot((path->points[i+1].p.x-path->points[i].p.x)*tm*tm+
		   2.0 * (path->points[i+2].p.x-path->points[i+1].p.x) * tm*tp+
		   (path->points[i+3].p.x-path->points[i+2].p.x) * tp*tp,

		   (path->points[i+1].p.y-path->points[i].p.y)*tm*tm+
		   2.0 * (path->points[i+2].p.y-path->points[i+1].p.y) * tm*tp+
		   (path->points[i+3].p.y-path->points[i+2].p.y) * tp*tp
		   );
}

/*! Calculates the curve lengths at an equidistant grid
    of curve parmeters by na numerical quadrature.

    You must call this subroutine before using
    \c hpgs_bezier_length_param.
*/
void hpgs_bezier_length_init(hpgs_bezier_length *b,
                             const hpgs_paint_path *path, int i)
{
  int ip;

  b->dl[0] = bezier_dl(path,i,-0.5);

  b->l[0] = 0.0;

  for (ip=1;ip<=16;++ip)
    {
      double dlm0,dlm1;

      // Lobatto quadrature with order 4.
      b->dl[ip] = bezier_dl(path,i,(ip-8.0)*0.0625);

      dlm0 = bezier_dl(path,i,(ip-(8.5+0.22360679774997896964))*0.0625);
      dlm1 = bezier_dl(path,i,(ip-(8.5-0.22360679774997896964))*0.0625);

      b->l[ip] = b->l[ip-1] +
	(b->dl[ip-1] + 5.0 * (dlm0 + dlm1) + b->dl[ip])/(16.0*12.0);
    }
}

/*! Returns the curve parameter at the bezier curve \c b
    corresponding to a curve length \c l.

    You must have called \c hpgs_bezier_length_init before using
    this function.
*/
double hpgs_bezier_length_param(const hpgs_bezier_length *b, double l)
{
  // binary search.

  int i = 0;

  if (l >= b->l[i+8]) i+=8;
  if (l >= b->l[i+4]) i+=4;
  if (l >= b->l[i+2]) i+=2;
  if (l >= b->l[i+1])  ++i;

  double dl = b->l[i+1]-b->l[i];
  double ll = (l-b->l[i])/dl;

  return
    (i - 8.0 + ll*ll*(3.0-2.0*ll)) * 0.0625 +
    (ll*(ll*(ll-2.0)+1.0) / b->dl[i] + ll*ll*(ll-1.0) / b->dl[i+1] ) * dl;
}

/*! Calculates the minimal x component of all four control points of
    the bezier spline. */
double hpgs_bezier_path_xmin (const hpgs_paint_path *path, int i)
{
  double x0 = 
    path->points[i].p.x < path->points[i+1].p.x ?
    path->points[i].p.x : path->points[i+1].p.x;

  double x1 = 
    path->points[i+2].p.x < path->points[i+3].p.x ?
    path->points[i+2].p.x : path->points[i+3].p.x;

  return x0 < x1 ? x0 : x1;
}

/*! Calculates the maximal x component of all four control points of
    the bezier spline. */
double hpgs_bezier_path_xmax (const hpgs_paint_path *path, int i)
{
  double x0 = 
    path->points[i].p.x > path->points[i+1].p.x ?
    path->points[i].p.x : path->points[i+1].p.x;

  double x1 = 
    path->points[i+2].p.x > path->points[i+3].p.x ?
    path->points[i+2].p.x : path->points[i+3].p.x;

  return x0 > x1 ? x0 : x1;
}

/*! Calculates the minimal y component of all four control points of
    the bezier spline. */
double hpgs_bezier_path_ymin (const hpgs_paint_path *path, int i)
{
  double y0 = 
    path->points[i].p.y < path->points[i+1].p.y ?
    path->points[i].p.y : path->points[i+1].p.y;

  double y1 = 
    path->points[i+2].p.y < path->points[i+3].p.y ?
    path->points[i+2].p.y : path->points[i+3].p.y;

  return y0 < y1 ? y0 : y1;
}

/*! Calculates the maximal y component of all four control points of
    the bezier spline. */
double hpgs_bezier_path_ymax (const hpgs_paint_path *path, int i)
{
  double y0 = 
    path->points[i].p.y > path->points[i+1].p.y ?
    path->points[i].p.y : path->points[i+1].p.y;

  double y1 = 
    path->points[i+2].p.y > path->points[i+3].p.y ?
    path->points[i+2].p.y : path->points[i+3].p.y;

  return y0 > y1 ? y0 : y1;
}

/*! Calculates the x component of a point on the bezier spline. */
double hpgs_bezier_path_x (const hpgs_paint_path *path, int i, double t)
{
  double tp,tm;

  tp = t + 0.5;
  tm = 0.5 - t;

  return
    path->points[i].p.x *tm*tm*tm +
    3.0 * tm*tp * (path->points[i+1].p.x * tm + path->points[i+2].p.x * tp) +
    path->points[i+3].p.x * tp*tp*tp;
}

/*! Calculates the y component of a point on the bezier spline. */
double hpgs_bezier_path_y (const hpgs_paint_path *path, int i, double t)
{
  double tp,tm;

  tp = t + 0.5;
  tm = 0.5 - t;

  return
    path->points[i].p.y *tm*tm*tm +
    3.0 * tm*tp * (path->points[i+1].p.y * tm + path->points[i+2].p.y * tp) +
    path->points[i+3].p.y * tp*tp*tp;
}

/*! Calculates the point on the bezier curve starting
    at point \c i of \c path at curve parameter \c t.
*/
void hpgs_bezier_path_point(const hpgs_paint_path *path, int i,
			    double t, hpgs_point *p)
{
  double tp,tm;

  tp = t + 0.5;
  tm = 0.5 - t;

  p->x =
    path->points[i].p.x *tm*tm*tm +
    3.0 * tm*tp * (path->points[i+1].p.x * tm + path->points[i+2].p.x * tp) +
    path->points[i+3].p.x * tp*tp*tp;

  p->y = 
    path->points[i].p.y *tm*tm*tm +
    3.0 * tm*tp * (path->points[i+1].p.y * tm + path->points[i+2].p.y * tp) +
    path->points[i+3].p.y * tp*tp*tp;
}
