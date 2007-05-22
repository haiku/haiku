/***********************************************************************
 *                                                                     *
 * $Id: hpgsscanline.c 374 2007-01-24 15:57:52Z softadm $
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
 * The implementation of the API for scanline handling.                *
 *                                                                     *
 ***********************************************************************/

#include<hpgspaint.h>
#include<math.h>
#include<string.h>
#if defined ( __MINGW32__ ) || defined ( _MSC_VER )
#include<malloc.h>
#else
#include<alloca.h>
#endif

//#define HPGS_DEBUG_CUT
//#define HPGS_DEBUG_THIN_CUT
//#define HPGS_DEBUG_ALPHA_CUT
//#define HPGS_DEBUG_EMIT_ALPHA
//#define HPGS_DEBUG_EMIT_0
//#define HPGS_DEBUG_CLIP

static int hpgs_paint_scanline_add_point(hpgs_paint_scanline *s, double x, int o);
static int hpgs_paint_scanline_add_slope(hpgs_paint_scanline *s, double x1, double x2, int o1, int o2);

/*! \defgroup scanline scanline handling.

  This module contains the documentation of the internals
  of the vector graphics renderer \c hpgs_paint_device.

  The dcomumentation of \c hpgs_paint_clipper_st explains the
  concepts used throughout the implementation of the objects
  in this module.
*/

/*!
   Initializes the given scanline structure for the given initial size \c msize
   of intersection points. The minimal value of \c msize is 4.

    Return value:
    \li 0 Success.
    \li -1 The system is out of memory.   
*/
static int hpgs_paint_scanline_init(hpgs_paint_scanline *s, int msize)
{
  s->points_malloc_size = msize < 4 ? 4 : msize;
  s->n_points = 0;
  s->points = (hpgs_scanline_point *)malloc(sizeof(hpgs_scanline_point)*s->points_malloc_size);

  return s->points ? 0 : -1;
}

/*!
   Frees the vector of of intersection points of the given scanline.
*/
static void hpgs_paint_scanline_cleanup(hpgs_paint_scanline *s)
{
  if (s->points)
    free(s->points);
}

/* Internal:
   Adds an edge of a path to the given scanline.

   \c order 1 is the intersection order
   (1 upwards, 0 horizontal, -1 downwards)
   of the path segment before the given edge point.

   \c order2 is the intersection order
   of the path segment after the given edge point.

   If the two orders are of a different sign ((1,-1) or (-1,1)),
   the edge is inserted twice with an opposite sign in order to draw
   line segments of width 0.

   Return value:
   \li 0 Success.
   \li -1 The system is out of memory.   
*/
static int add_edge(hpgs_paint_scanline *s,
		    hpgs_path_point *point,
		    int order1, int order2)
{
#ifdef HPGS_DEBUG_CUT
  hpgs_log("add_edge: x,y,o1,o2 = %lg,%lg,%d,%d.\n",
	   point->p.x,point->p.y,order1,order2);
#endif
  if (order1+order2 > 0)
    {
      if (hpgs_paint_scanline_add_point(s,point->p.x,1))
	return -1;
    }
  else if (order1+order2 < 0)
    {
      if (hpgs_paint_scanline_add_point(s,point->p.x,-1))
	return -1;
    }
  else
    if (order1 > 0)
      {
	if (hpgs_paint_scanline_add_point(s,point->p.x,1) ||
	    hpgs_paint_scanline_add_point(s,point->p.x,-1)   )
	  return -1;
      }
    else if (order1 < 0) 
      {
	if (hpgs_paint_scanline_add_point(s,point->p.x,-1) ||
	    hpgs_paint_scanline_add_point(s,point->p.x,1)   )
	  return -1;
      }

  return 0;
}

/* Internal:
   Sorts the intersection points of the the given scanline
   using merge sort. Merge sort is used in order to preserve
   the order of two equal x positions with opposite sign.
   (see add_egde above)

   As a side effect, this approach is ways faster than qsort().

   Internal routine. Driver routine below.
*/
static void scanline_msort(hpgs_scanline_point *a, int n,
			   hpgs_scanline_point *res,
			   hpgs_scanline_point *tmp )
{
  int n1 = n/2;
  int n2 = n - n1;
  hpgs_scanline_point *b=a+n1;
  hpgs_scanline_point *te=tmp+n1;
  hpgs_scanline_point *be=a+n;

  // sort first half of a into tmp.
  switch (n1)
    {
    case 1:
      *tmp = *a;
    case 0:
      break;
    default:
      scanline_msort(a,n1,tmp,te);
    }

  // sort second half of a (aka b) in place.
  if (n2 > 1)
    scanline_msort(b,n2,b,te);

  // now merge the results.
  while (tmp < te && b < be)
    {
      // stable sort characteristics: prefer first half.
      if (tmp->x <= b->x)
	*res = *tmp++;
      else
	*res = *b++;

      ++res;
    }

  // copy rest of first sorted half
  while (tmp < te)
    *res++ = *tmp++;
  
  // copy rest of second half, if not co-located
  if (res!=b)
    while (b < be)
      *res++ = *b++;
}

/* Internal:
   Sorts the intersection points of the the given scanline
   using merge sort. Internal subroutine and docs above.
*/
static void scanline_sort (hpgs_scanline_point *a, int n)
{
  hpgs_scanline_point *tmp = hpgs_alloca(sizeof(hpgs_scanline_point) * n);

  scanline_msort(a,n,a,tmp);
}

/* Internal:
   Cuts the bezier segment with the scanlines of the clipper.

   Returns the scanline number of the endpoint hit or -1,
   if endpoint is not an exact hit.

   Upon memory error return -2;
*/

static int bezier_clipper_cut (hpgs_paint_clipper *c,
                               hpgs_paint_path *path, int i,
                               double t0, double t1,
                               double y0,
                               double y1,
                               double y2,
                               double y3 )
{
  double ymin0 = (y0 < y1) ? y0 : y1;
  double ymin1 = (y2 < y3) ? y2 : y3;
  double ymin = ymin0<ymin1 ? ymin0 : ymin1;

  double ymax0 = (y0 > y1) ? y0 : y1;
  double ymax1 = (y2 > y3) ? y2 : y3;
  double ymax = ymax0>ymax1 ? ymax0 : ymax1;

  double dscan0 = (c->y0 - ymax)/c->yfac;
  double dscan1 = (c->y0 - ymin)/c->yfac;

  double rdscan0 = ceil(dscan0);
  double rdscan1 = floor(dscan1);

  int iscan0 = (int)rdscan0;
  int iscan1 = (int)rdscan1;

  if (iscan0 > iscan1 || iscan1 < 0 || iscan0 >= c->n_scanlines)
    {
      return -1;
    }
  else if (t1-t0 < 1.0e-8 && iscan0 == iscan1)
    {
      int o0;

      if (y1 > y0) o0 = 1;
      else if (y1 < y0) o0 = -1;
      else if (y2 > y0) o0 = 1;
      else if (y2 < y0) o0 = -1;
      else if (y3 > y0) o0 = 1;
      else if (y3 < y0) o0 = -1;
      else o0 = 0;

      int o1;

      if (y2 > y3) o1 = -1;
      else if (y2 < y3) o1 = 1;
      else if (y1 > y3) o1 = -1;
      else if (y1 < y3) o1 = 1;
      else if (y0 > y3) o1 = -1;
      else if (y0 < y3) o1 = 1;
      else o1 = 0;

      // horizontal cut.
      if (o0*o1 == 0) 
        {
          return -1;
        }

      // filter out startpoint hit.
      if (t0 == -0.5 &&
          ((y0 == ymin && dscan1 == rdscan1) ||
           (y0 == ymax && dscan0 == rdscan0)   )) return -1;

      // filter out endpoint hit.
      if (t1 == 0.5 &&
          ((y3 == ymin && dscan1 == rdscan1) ||
           (y3 == ymax && dscan0 == rdscan0)   )) return iscan0;

      if (iscan0 < c->iscan0) c->iscan0 = iscan0;
      if (iscan0 > c->iscan1) c->iscan1 = iscan0;

      if (o0*o1 > 0)
        {
          if (hpgs_paint_scanline_add_point(c->scanlines+iscan0,
                                            hpgs_bezier_path_x(path,i,0.5*(t0+t1)),
                                            o0))
            return -2;

          return -1;
        }
      else
        {
          if (hpgs_paint_scanline_add_point(c->scanlines+iscan0,
                                            hpgs_bezier_path_x(path,i,t0),
                                            o0))
            return -2;

          if (hpgs_paint_scanline_add_point(c->scanlines+iscan0,
                                            hpgs_bezier_path_x(path,i,t1),
                                            o1))
            return -2;

          return -1;
        }
    }
  else if (t1-t0 < 1.0e-15)
    {
      return -1;
    }
  else
    {
      // partition the spline.
      double tmid = 0.5*(t0+t1);

      // midpoint.
      double ymid,y1l,y2l,y1u,y2u;
      
      y1l = 0.5 * (y0 + y1);

      y2l = 0.25 * (y0 + y2) + 0.5 * y1;

      ymid =
        (y0 + y3) * 0.125 + 0.375 * (y1 + y2);

      y1u = 0.25 * (y1 + y3) + 0.5 * y2;

      y2u = 0.5 * (y2 + y3);

      if (bezier_clipper_cut (c,path,i,t0,tmid,y0,y1l,y2l,ymid) < -1)
        return -2;

      return bezier_clipper_cut (c,path,i,tmid,t1,ymid,y1u,y2u,y3);
    }
}

/*!
   Cuts te given \c path with the scanlines of the given
   clipper \c c. If you emit the clipper afterwards, the interior
   of the given path is drawn to the image.

   If you want to stroke the path use either \c hpgs_paint_clipper_thin_cut
   for thin lines or contruct the outline of a thick line using
   the line style of a graphics state with \c hpgs_paint_path_stroke_path.

   Return value:
   \li 0 Success.
   \li -1 The system is out of memory.   
*/
static int hpgs_paint_clipper_cut_0(hpgs_paint_clipper *c,
                                    hpgs_paint_path *path)
{
  int i;
  int start_order=0,last_order=0;
  hpgs_path_point *deferred_edge_cut = 0;
  int deferred_edge_cut_is = -1;
  int iscan;

  // catch path' which lie outside of the area of interest.
  if (path->bb.urx < c->bb.llx) return 0;
  if (path->bb.llx > c->bb.urx) return 0;
  if (path->bb.ury < c->bb.lly) return 0;
  if (path->bb.lly > c->bb.ury) return 0;

  for (i=0;i<path->n_points;++i)
    {
      hpgs_path_point *point = &path->points[i];
      hpgs_path_point *next_point;
      hpgs_path_point *next_edge_cut=0;
      int next_edge_cut_is=-1;

      int order1 = 0;
      int order2 = 0;
      int iscan0,iscan1;

      switch (point->flags & HPGS_POINT_ROLE_MASK)
	{
	case HPGS_POINT_LINE:
	case HPGS_POINT_FILL_LINE:
          {
            next_point = &path->points[i+1];

            double dscan0 = (c->y0 - point->p.y)/c->yfac;
            double dscan1 = (c->y0 - next_point->p.y)/c->yfac;
 
#ifdef HPGS_DEBUG_CUT
            hpgs_log("cut_line: x1,y1,x2,y2 = %lg,%lg,%lg,%lg.\n",
                     point->p.x,point->p.y,next_point->p.x,next_point->p.y);
#endif


            if (dscan0 < dscan1)
              {
                double rdscan0 = ceil(dscan0);
                double rdscan1 = floor(dscan1);

                iscan0 = (int)rdscan0;
                iscan1 = (int)rdscan1;

                // startpoint hit
                if (rdscan0 == dscan0)
                  ++iscan0;

                // endpoint hit
                if (rdscan1 == dscan1)
                  {
                    if (iscan1 >= 0 && iscan1 < c->n_scanlines)
                      {
                        next_edge_cut = next_point;
                        next_edge_cut_is = iscan1;
                      }
                    --iscan1;
                  }
              }
            else
              {
                double rdscan0 = ceil(dscan1);
                double rdscan1 = floor(dscan0);

                iscan0 = (int)rdscan0;
                iscan1 = (int)rdscan1;

                // startpoint hit
                if (rdscan1 == dscan1)
                  --iscan1;

                // endpoint hit
                if (rdscan0 == dscan0)
                  {
                    if (iscan0 >= 0 && iscan0 < c->n_scanlines)
                      {
                        next_edge_cut = next_point;
                        next_edge_cut_is = iscan0;
                      }
                    ++iscan0;
                  }
              }

            if (iscan0 < 0) iscan0 = 0;
            if (iscan1 >= c->n_scanlines) iscan1 = c->n_scanlines-1;

            if (iscan0 < c->iscan0) c->iscan0 = iscan0;
            if (iscan1 > c->iscan1) c->iscan1 = iscan1;
            
            if (next_point->p.y > point->p.y)
              order1 = 1;
            else if (next_point->p.y < point->p.y)
              order1 = -1;
            else
              order1 = 0;
            
            order2 = order1;
            
            if (point->p.x != next_point->p.x)
              for (iscan=iscan0;iscan<=iscan1;++iscan)
                {
                  double y = c->y0 - iscan * c->yfac;
                  double x =
                    (point->p.x * (next_point->p.y - y) +
                     next_point->p.x * (y - point->p.y)  ) /
                    (next_point->p.y - point->p.y);
                  
                  if (hpgs_paint_scanline_add_point(c->scanlines+iscan,x,order1))
                    return -1;
                }
            else
              for (iscan=iscan0;iscan<=iscan1;++iscan)
                {
                  if (hpgs_paint_scanline_add_point(c->scanlines+iscan,
                                                    point->p.x,order1))
                    return -1;
                }
          }
          break;
        case HPGS_POINT_BEZIER:
	  next_point = &path->points[i+3];

#ifdef HPGS_DEBUG_CUT
	  hpgs_log("cut_bezier: x1,y1,x4,y4 = %lg,%lg,%lg,%lg.\n",
		   point->p.x,point->p.y,next_point->p.x,next_point->p.y);
#endif

	  if (path->points[i+1].p.y > point->p.y)
	    order1 = 1;
	  else if (path->points[i+1].p.y < point->p.y)
	    order1 = -1;
	  else if (path->points[i+2].p.y > point->p.y)
	    order1 = 1;
	  else if (path->points[i+2].p.y < point->p.y)
	    order1 = -1;
	  else if (next_point->p.y > point->p.y)
	    order1 = 1;
	  else if (next_point->p.y < point->p.y)
	    order1 = -1;
	  else
	    order1 = 0;
	  
	  if (path->points[i+2].p.y < next_point->p.y)
	    order2 = 1;
	  else if (path->points[i+2].p.y > next_point->p.y)
	    order2 = -1;
	  else if (path->points[i+1].p.y < next_point->p.y)
	    order2 = 1;
	  else if (path->points[i+1].p.y > next_point->p.y)
	    order2 = -1;
	  else if (point->p.y < next_point->p.y)
	    order2 = 1;
	  else if (point->p.y > next_point->p.y)
	    order2 = -1;
            else
	    order2 = 0;
	  
          iscan1 = bezier_clipper_cut (c,path,i,-0.5,0.5,
                                       point->p.y,
                                       path->points[i+1].p.y,
                                       path->points[i+2].p.y,
                                       next_point->p.y );

          if (iscan1 < -1) return -1;

          if (iscan1 >= 0 && iscan1 < c->n_scanlines)
            {
              next_edge_cut = next_point;
              next_edge_cut_is = iscan1;
            }

	  // ignore control points.
	  i+=2;
	  break;
	}

      if (point->flags & HPGS_POINT_SUBPATH_END)
	{
	  if (deferred_edge_cut)
	    if (add_edge(c->scanlines+deferred_edge_cut_is,
			 deferred_edge_cut,last_order,start_order))
	      return -1;
	}
      else
	{
	  if (deferred_edge_cut)
	    if (add_edge(c->scanlines+deferred_edge_cut_is,
			 deferred_edge_cut,last_order,order1))
	      return -1;
	}

      if (point->flags & HPGS_POINT_SUBPATH_START)
	start_order = order1;

      last_order = order2;
      deferred_edge_cut = next_edge_cut;
      deferred_edge_cut_is = next_edge_cut_is;
    }

  if (deferred_edge_cut)
    if (add_edge(c->scanlines+deferred_edge_cut_is,
		 deferred_edge_cut,last_order,start_order))
      return -1;

  // search for minimal figures, which did not hit any scanline
  // (horizontal fills with a height below 1 pixel...).
  if (c->iscan0 > c->iscan1 && path->n_points)
    {
      iscan = (int)floor((c->y0 - 0.5 * (path->bb.lly + path->bb.ury))/c->yfac + 0.5);
      
#ifdef HPGS_DEBUG_CUT
      hpgs_log("cut_minimal: llx,lly,urx,ury,iscan,n = %lg,%lg,%lg,%lg.\n",
               path->bb.llx,path->bb.lly,path->bb.urx,path->bb.ury);
#endif

      if (iscan < 0 || iscan >= c->n_scanlines) return 0;

      c->iscan0 = c->iscan1 = iscan;

      if (hpgs_paint_scanline_add_point(c->scanlines+iscan,path->bb.llx,1) ||
          hpgs_paint_scanline_add_point(c->scanlines+iscan,path->bb.urx,-1)   )
        return -1;

      return 0;
    }

  for (iscan = c->iscan0;iscan<=c->iscan1;++iscan)
    {
      scanline_sort(c->scanlines[iscan].points,c->scanlines[iscan].n_points);
#ifdef HPGS_DEBUG_CUT
      {
	int j;

	hpgs_log("iscan = %d, np = %d ****************************************\n",
		 iscan,c->scanlines[iscan].n_points);

	if (c->scanlines[iscan].n_points & 1)
	  hpgs_log("np odd !!!!!!!!!!!!!!!\n");

	for (j=0;j<c->scanlines[iscan].n_points;++j)
	  hpgs_log("%lg(%d) ",
		   c->scanlines[iscan].points[j].x,
		   c->scanlines[iscan].points[j].order);

	hpgs_log("\n");
      }
#endif
    }

  return 0;
}

/* Internal:
   Cuts the bezier segment with the scanlines of the clipper
   as for thin lines.

   Returns 0 upon success or -1 on memory error.
*/

typedef struct hpgs_bezier_clipper_thin_cut_data_st hpgs_bezier_clipper_thin_cut_data;

struct hpgs_bezier_clipper_thin_cut_data_st
{
  hpgs_paint_clipper *c;
  hpgs_paint_path *path;
  int i;

  double t0;
  double t1;

  double t_last;
  double x_last;
  int iscan_last;
};

static int hpgs_bezier_clipper_thin_cut_data_push(hpgs_bezier_clipper_thin_cut_data *d,
                                                  double t, double x, int iscan, int o)
{
  if (d->iscan_last >= 0 && d->iscan_last < d->c->n_scanlines)
    {
      if (d->iscan_last < d->c->iscan0)
        d->c->iscan0 = d->iscan_last;

      if (d->iscan_last > d->c->iscan1)
        d->c->iscan1 = d->iscan_last;

      int order = x > d->x_last ? 1 : -1;

      if (hpgs_paint_scanline_add_point(d->c->scanlines+d->iscan_last,d->x_last,order))
        return -1;

      if (hpgs_paint_scanline_add_point(d->c->scanlines+d->iscan_last,x,-order))
        return -1;
    }

  if (o > 0)
    d->iscan_last = iscan;
  else
    d->iscan_last = iscan+1;

  d->x_last = x;
  d->t_last = t;

  return 0;
}

static int bezier_clipper_thin_cut (hpgs_bezier_clipper_thin_cut_data *d,
                                    double t0, double t1,
                                    double y0,
                                    double y1,
                                    double y2,
                                    double y3 )
{
  double ymin0 = (y0 < y1) ? y0 : y1;
  double ymin1 = (y2 < y3) ? y2 : y3;
  double ymin = ymin0<ymin1 ? ymin0 : ymin1;

  double ymax0 = (y0 > y1) ? y0 : y1;
  double ymax1 = (y2 > y3) ? y2 : y3;
  double ymax = ymax0>ymax1 ? ymax0 : ymax1;

  double dscan0 = (d->c->y0 - ymax)/d->c->yfac - 0.5;
  double dscan1 = (d->c->y0 - ymin)/d->c->yfac - 0.5;

  double rdscan0 = ceil(dscan0);
  double rdscan1 = floor(dscan1);

  int iscan0 = (int)rdscan0;
  int iscan1 = (int)rdscan1;

  if (iscan0 > iscan1 || iscan1 < -1 || iscan0 >= d->c->n_scanlines)
    {
      return 0;
    }
  else if (t1-t0 < 1.0e-8 && iscan0 == iscan1)
    {
      int o0;

      if (y1 > y0) o0 = 1;
      else if (y1 < y0) o0 = -1;
      else if (y2 > y0) o0 = 1;
      else if (y2 < y0) o0 = -1;
      else if (y3 > y0) o0 = 1;
      else if (y3 < y0) o0 = -1;
      else o0 = 0;

      int o1;

      if (y2 > y3) o1 = -1;
      else if (y2 < y3) o1 = 1;
      else if (y1 > y3) o1 = -1;
      else if (y1 < y3) o1 = 1;
      else if (y0 > y3) o1 = -1;
      else if (y0 < y3) o1 = 1;
      else o1 = 0;

      // horizontal cut.
      if (o0*o1 == 0) 
        {
          return 0;
        }

      // filter out startpoint hit.
      if (t0 == d->t0 &&
          ((y0 == ymin && dscan1 == rdscan1) ||
           (y0 == ymax && dscan0 == rdscan0)   )) return 0;

      // filter out endpoint hit.
      if (t1 == d->t1 &&
          ((y3 == ymin && dscan1 == rdscan1) ||
           (y3 == ymax && dscan0 == rdscan0)   )) return 0;

      if (iscan0 < d->c->iscan0)
        {
          if (iscan0 < 0)
            d->c->iscan0 = 0;
          else
            d->c->iscan0 = iscan0;
        }

      if (iscan0 >= d->c->iscan1)
        {
          if (iscan0+1 >= d->c->n_scanlines)
            d->c->iscan1 = d->c->n_scanlines-1;
          else
            d->c->iscan1 = iscan0+1;
        }

      if (o0*o1 > 0)
        {
          double t = 0.5*(t0+t1);
          double x = hpgs_bezier_path_x(d->path,d->i,t);

          return hpgs_bezier_clipper_thin_cut_data_push(d,t,x,iscan0,o0);
        }
      else
        {
          double x0 = hpgs_bezier_path_x(d->path,d->i,t0);
          double x1 = hpgs_bezier_path_x(d->path,d->i,t1);

          if (hpgs_bezier_clipper_thin_cut_data_push(d,t0,x0,iscan0,o0))
            return -1;

          return hpgs_bezier_clipper_thin_cut_data_push(d,t1,x1,iscan0,o1);
        }
    }
  else if (t1-t0 < 1.0e-15)
    {
      return 0;
    }
  else
    {
      // partition the spline.
      double tmid = 0.5*(t0+t1);

      // midpoint.
      double ymid,y1l,y2l,y1u,y2u;
      
      y1l = 0.5 * (y0 + y1);

      y2l = 0.25 * (y0 + y2) + 0.5 * y1;

      ymid =
        (y0 + y3) * 0.125 + 0.375 * (y1 + y2);

      y1u = 0.25 * (y1 + y3) + 0.5 * y2;

      y2u = 0.5 * (y2 + y3);

      if (bezier_clipper_thin_cut (d,t0,tmid,y0,y1l,y2l,ymid))
        return -1;

      if (bezier_clipper_thin_cut (d,tmid,t1,ymid,y1u,y2u,y3))
        return -1;
          
      return 0;
    }
}

static int thin_push_dot(hpgs_paint_clipper *c, const hpgs_point *p)
{
  int iscan = floor((c->y0 - p->y)/c->yfac + 0.5);

  if (iscan < 0 || iscan >= c->n_scanlines) return 0;
  
#ifdef HPGS_DEBUG_CUT
	  hpgs_log("push_dot: x,y,iscan = %lg,%lg,%d.\n",
		   p->x,p->y,iscan);
#endif

  if (hpgs_paint_scanline_add_point(c->scanlines+iscan,p->x,1) ||
      hpgs_paint_scanline_add_point(c->scanlines+iscan,p->x,-1)   )
    return -1;
  
  if (iscan < c->iscan0) c->iscan0 = iscan;
  if (iscan > c->iscan1) c->iscan1 = iscan;
  return 0;
}

/* Internal:
   Cut a part of a path with a linewidth below the width of a pixel with
   the scanlines of the given clipper \c c.
  
   The subpath starts at the path segment starting at point \c i0 at the
   curve parameter \c t0 (t=-0.5 start of segment, t=0.5 end of segment).

   The scanline ends at the path segment starting at point \c i1 at the
   curve parameter \c t1.

   The actual intersections are undertaken at an y value in the middle between
   the two y values of adjacent scanlines. The actual intersection points with
   this intermediate scanline is inserted twice, once in the adjacent scanline 
   above and below the intermediate scanline.  
*/
static int thin_cut_segment(hpgs_paint_clipper *c,
			    hpgs_paint_path *path,
			    int i0, double t0,
			    int i1, double t1 )
{
  int i;
  int iscan;

  for (i=i0;i<=i1;++i)
    {
      hpgs_path_point *point = &path->points[i];
      hpgs_path_point *next_point;
      double x0,x1,y0,y1;
      int iscan0,iscan1,order;

      double tt0 = (i == i0) ? t0 : -0.5;
      double tt1 = (i == i1) ? t1 :  0.5;

      switch (point->flags & HPGS_POINT_ROLE_MASK)
	{
        case HPGS_POINT_DOT:
          if (thin_push_dot(c,&path->points[i].p)) return -1;
          break;

	case HPGS_POINT_LINE:
	  next_point = &path->points[i+1];

	  if (tt0 > -0.5)
	    {
	      x0 = (0.5 - tt0) * point->p.x + (0.5 + tt0) * next_point->p.x;
	      y0 = (0.5 - tt0) * point->p.y + (0.5 + tt0) * next_point->p.y;
	    }
	  else
	    {
	      x0 = point->p.x;
	      y0 = point->p.y;
	    }

	  if (tt1 < 0.5)
	    {
	      x1 = (0.5 - tt1) * point->p.x + (0.5 + tt1) * next_point->p.x;
	      y1 = (0.5 - tt1) * point->p.y + (0.5 + tt1) * next_point->p.y;
	    }
	  else
	    {
	      x1 =  next_point->p.x;
	      y1 =  next_point->p.y;
	    }

	  order = x1 < x0 ? -1 : 1;

	  iscan0 =
	    (int)floor((c->y0 - y0)/c->yfac + 0.5);
	  iscan1 =
	    (int)floor((c->y0 - y1)/c->yfac  + 0.5);

	  if (iscan0 >= 0 && iscan0 < c->n_scanlines &&
	      hpgs_paint_scanline_add_point(c->scanlines+iscan0,x0,order))
	    return -1;

	  if (iscan1 >= 0 && iscan1 < c->n_scanlines &&
	      hpgs_paint_scanline_add_point(c->scanlines+iscan1,x1,-order))
	    return -1;

	  if (iscan0 > iscan1)
	    { int tmp = iscan0; iscan0 = iscan1; iscan1 = tmp; order = -order; }

	  if (iscan0 < 0)
	    {
	      iscan0 = -1;
	      c->iscan0 = 0;
	    }
	  else
	    if (iscan0 < c->iscan0)
	      c->iscan0 = iscan0;

	  if (iscan1 >= c->n_scanlines)
	    {
	      iscan1 = c->n_scanlines;
	      c->iscan1 = c->n_scanlines-1;
	    }
	  else
	    if (iscan1 > c->iscan1)
	      c->iscan1 = iscan1;

	  if (point->p.x != next_point->p.x)
	    for (iscan=iscan0;iscan<iscan1;++iscan)
	      {
		double y = c->y0 - (iscan + 0.5) * c->yfac;
		
		double x =
		  (point->p.x * (next_point->p.y - y) +
		   next_point->p.x * (y - point->p.y)  ) /
		  (next_point->p.y - point->p.y);
		
		if (iscan >= 0 &&
		    hpgs_paint_scanline_add_point(c->scanlines+iscan,x,-order))
		  return -1;
		
		if (iscan < c->n_scanlines-1 &&
		    hpgs_paint_scanline_add_point(c->scanlines+iscan+1,x,order))
		  return -1;
	      }
	  else
	    for (iscan=iscan0;iscan<iscan1;++iscan)
	      {
		if (iscan >= 0 &&
		    hpgs_paint_scanline_add_point(c->scanlines+iscan,
						  point->p.x,-order))
		  return -1;
		
		if (iscan < c->n_scanlines-1 &&
		    hpgs_paint_scanline_add_point(c->scanlines+iscan+1,
						  point->p.x,order))
		  return -1;
	      }

	  break;
	case HPGS_POINT_BEZIER:
	  next_point = &path->points[i+3];

          {
            double yc0,yc1;

            // start point.
            if (tt0 > -0.5)
              {
                x0 = hpgs_bezier_path_x(path,i,tt0);
                y0 = hpgs_bezier_path_y(path,i,tt0);
              }
            else
              {
                x0 = point->p.x;
                y0 = point->p.y;
	    }

	  
            // end point
            if (tt1 < 0.5)
              {
                x1 = hpgs_bezier_path_x(path,i,tt1);
                y1 = hpgs_bezier_path_y(path,i,tt1);
              }
            else
              {
                x1 =  next_point->p.x;
                y1 =  next_point->p.y;
              }

            // control points
            if (tt0 > -0.5 || tt1 < 0.5)
              {
                yc0 = y0 + (tt1-tt0) * hpgs_bezier_path_delta_y(path,i,tt0)/3.0;
                yc1 = y1 - (tt1-tt0) * hpgs_bezier_path_delta_y(path,i,tt1)/3.0;
              }
            else
              {
                yc0 = path->points[i+1].p.y;
                yc1 = path->points[i+2].p.y;
              }

            hpgs_bezier_clipper_thin_cut_data d =
              { c,path,i,tt0,tt1,tt0,x0,
                (int)floor((c->y0 - y0)/c->yfac + 0.5) };

            if (bezier_clipper_thin_cut (&d,tt0,tt1,y0,yc0,yc1,y1))
              return -1;

            // add endpoint with appropriate order.
            if (d.iscan_last >= 0 && d.iscan_last < c->n_scanlines)
              {
                int order = x1 > d.x_last ? 1 : -1;

                if (d.iscan_last < c->iscan0)
                  c->iscan0 = d.iscan_last;

                if (d.iscan_last > c->iscan1)
                  c->iscan1 = d.iscan_last;

                if (hpgs_paint_scanline_add_point(c->scanlines+d.iscan_last,d.x_last,order))
                  return -1;

                if (hpgs_paint_scanline_add_point(c->scanlines+d.iscan_last,x1,-order))
                  return -1;
              }
          }
          // ignore control points.
          i+=2;
	  break;
	}
    }

  return 0;
}

/* Internal:
   Cut path which is dashed according to the given \c gstate and has a linewidth
   below the width of a pixel with the scanlines of the given clipper \c c.
*/
static int thin_cut_dashed(hpgs_paint_clipper *c,
			   hpgs_paint_path *path,
			   const hpgs_gstate *gstate)
{
  int i0=0,i1=-1,i;
  int idash = 0;
  double t0=-0.5;
  double t1=0.5;

  double l=gstate->dash_offset;
  int out_state = 0;
  double ltot = 0.0;
  double next_l = 0.0;
  double lseg;

  hpgs_bezier_length bl;

  for (idash=0;idash<gstate->n_dashes;++idash)
    ltot += gstate->dash_lengths[idash];

  for (i=0;i<path->n_points;++i)
    {
      int role = path->points[i].flags & HPGS_POINT_ROLE_MASK;
      int end_line = 0;

      if (role == HPGS_POINT_BEZIER || role == HPGS_POINT_LINE)
	i1 = i;

      if (path->points[i].flags & HPGS_POINT_SUBPATH_START)
	{
          if (role == HPGS_POINT_DOT)
            {
              if (thin_push_dot(c,&path->points[i].p)) return -1;
              i1 = -1;
              continue;
            }
          else
            {
              i0 = i1;
              t0 = -0.5;

              l=fmod(gstate->dash_offset,ltot);
              if (l<0.0) l+= ltot;

              next_l = 0.0;
              for (idash=0;idash<gstate->n_dashes;++idash)
                {
                  next_l += gstate->dash_lengths[idash];
                  if (next_l > l)
                    break;
                }

              out_state = (idash+1) & 1;
            }
	}

      switch (role)
	{
	case HPGS_POINT_LINE:
	  lseg = hypot(path->points[i+1].p.x - path->points[i].p.x,
		       path->points[i+1].p.y - path->points[i].p.y );

	  end_line =
	    (path->points[i+1].flags & HPGS_POINT_SUBPATH_END) ||
	    i >= path->n_points-2;

	  break;
	case HPGS_POINT_BEZIER:
	  hpgs_bezier_length_init(&bl,path,i);
	  
	  lseg = bl.l[HPGS_BEZIER_NSEGS];

	  end_line =
	    (path->points[i+3].flags & HPGS_POINT_SUBPATH_END) ||
	    i >= path->n_points-4;
	  break;

	default:
	  lseg = 0.0;
	}

      if  (role == HPGS_POINT_BEZIER || role == HPGS_POINT_LINE)
	{
	  while (l+lseg > next_l)
	    {
	      if (role == HPGS_POINT_BEZIER)
		t1 =  hpgs_bezier_length_param(&bl,next_l - l);
	      else
		t1 = (next_l - l) / lseg - 0.5;

	      idash = (idash+1) % gstate->n_dashes;
	      next_l += gstate->dash_lengths[idash];

	      if (out_state)
		{
		  if (thin_cut_segment(c,path,i0,t0,i1,t1))
		    return -1;
		}

	      i0 = i1;
	      t0 = t1;

	      out_state = (idash+1) & 1; 
	    }

	  l+=lseg;
	}

      if (end_line && out_state && (i0<i1 || t0 < 0.5) )
	{
	  if (thin_cut_segment(c,path,i0,t0,i1,0.5))
	    return -1;
	}
    }
  return 0;
}     

/*!
   Cuts the border of the given \c path with the scanlines of the given
   clipper \c c. The linewidth of the given \c gstate is ignored and an
   optimized algorithm is used in order to retrieve the cut of the border
   of the path as if the linewidth has been exactly one pixel of the
   underlying image.

   Return value:
   \li 0 Success.
   \li -1 The system is out of memory.   
*/
int hpgs_paint_clipper_thin_cut(hpgs_paint_clipper *c,
				hpgs_paint_path *path,
				const hpgs_gstate *gstate)
{
  int iscan;

  if (path->n_points < 1) return 0;

  // catch path' which lie outside of the area of interest.
  if (path->bb.urx < c->bb.llx) return 0;
  if (path->bb.llx > c->bb.urx) return 0;
  if (path->bb.ury < c->bb.lly) return 0;
  if (path->bb.lly > c->bb.ury) return 0;

  if (gstate->n_dashes == 0)
    {
      if (thin_cut_segment (c,path,
			    0,-0.5,path->n_points-1,0.5))
	return -1;
    }
  else
    {
      if (thin_cut_dashed(c,path,gstate))
	return -1;
    }

  for (iscan = c->iscan0;iscan<=c->iscan1;++iscan)
    {
      scanline_sort(c->scanlines[iscan].points,c->scanlines[iscan].n_points);

#ifdef HPGS_DEBUG_THIN_CUT
      {
	int j;

	hpgs_log("iscan = %d, np = %d ****************************************\n",
		 iscan,c->scanlines[iscan].n_points);

	if (c->scanlines[iscan].n_points & 1)
	  hpgs_log("np odd !!!!!!!!!!!!!!!\n");

	for (j=0;j<c->scanlines[iscan].n_points;++j)
	  hpgs_log("%lg(%d) ",
		   c->scanlines[iscan].points[j].x,
		   c->scanlines[iscan].points[j].order);

	hpgs_log("\n");
      }
#endif
    }

  return 0;
}

/* Internal:
   Cuts the bezier segment with the scanlines of the clipper
   and creates alpha slopes on the generated scanline intersection points.

   Returns 0 upon success or -1 on memory error.
*/

typedef struct hpgs_bezier_clipper_alpha_cut_data_st hpgs_bezier_clipper_alpha_cut_data;

struct hpgs_bezier_clipper_alpha_cut_data_st
{
  hpgs_paint_clipper *c;
  hpgs_paint_path *path;
  int i;

  double t_last;
  double x_last;
  int o_last;
  int iscan_last;
};

static int hpgs_bezier_clipper_alpha_cut_data_push(hpgs_bezier_clipper_alpha_cut_data *d,
                                                   double t, double x,
                                                   int iscan, int order,
                                                   int o)
{
  if (d->iscan_last >= 0 && d->iscan_last < d->c->n_scanlines)
    {
      if (d->iscan_last < d->c->iscan0)
        d->c->iscan0 = d->iscan_last;

      if (d->iscan_last > d->c->iscan1)
        d->c->iscan1 = d->iscan_last;

      if (hpgs_paint_scanline_add_slope(d->c->scanlines+d->iscan_last,d->x_last,x,
                                        d->o_last,o<=0 ? order : 0))
        return -1;
    }

  if (o < 0)
    {
      d->iscan_last = iscan+1;
      d->o_last = 0;
    }
  else
    {
      d->iscan_last = iscan;
      d->o_last = order;
    }

  d->x_last = x;
  d->t_last = t;

  return 0;
}

static int bezier_clipper_alpha_cut_isolate_extrema (hpgs_bezier_clipper_alpha_cut_data *d,
                                                     double t0, double t1,
                                                     double y0,
                                                     double y1,
                                                     double y2,
                                                     double y3, hpgs_bool do_max )
{
  // Ya olde golden cut...
  double c0 = .38196601125010515180;
  double c1 = .61803398874989484820;

  double tt0 = -0.5;
  double tt1 = -0.5 * c1 + 0.5 * c0;
  double tt2 = -0.5 * c0 + 0.5 * c1;
  double tt3 = 0.5;

  double tp = tt1 + 0.5;
  double tm = 0.5 - tt1;

  double yy1 = y0 *tm*tm*tm + 3.0 * tm*tp * (y1 * tm + y2 * tp) + y3 * tp*tp*tp;

  tp = tt2 + 0.5;
  tm = 0.5 - tt2;

  double yy2 = y0 *tm*tm*tm + 3.0 * tm*tp * (y1 * tm + y2 * tp) + y3 * tp*tp*tp;


  while ((tt2-tt1)*(t1-t0) > 1.0e-8)
    if ((do_max && yy1 > yy2) || (!do_max && yy1 < yy2))
      {
        tt3 = tt2;
        tt2 = tt1;
        yy2 = yy1;
        tt1 = tt3 * c0 + tt0 * c1;
        
        tp = tt1 + 0.5;
        tm = 0.5 - tt1;
        yy1 = y0 *tm*tm*tm + 3.0 * tm*tp * (y1 * tm + y2 * tp) + y3 * tp*tp*tp;
      }
    else
      {
        tt0 = tt1;
        tt1 = tt2;
        yy1 = yy2;
        tt2 = tt3 * c1 + tt0 * c0;
        
        tp = tt2 + 0.5;
        tm = 0.5 - tt2;
        yy2 = y0 *tm*tm*tm + 3.0 * tm*tp * (y1 * tm + y2 * tp) + y3 * tp*tp*tp;
      }
  
  double tt = 0.5 * (tt1+tt2);
  double t = t0 * (0.5-tt) + t1 * (0.5+tt);
  double x = hpgs_bezier_path_x(d->path,d->i,t);

  double dscan = (d->c->y0 - 0.5 * (yy1+yy2))/d->c->yfac + 0.5;
  double rdscan = floor(dscan);
  int iscan = (int)rdscan;
  int order = (int)((dscan-rdscan)*256.0+0.5);

  return hpgs_bezier_clipper_alpha_cut_data_push(d,t,x,iscan,order,0);
}

static int bezier_clipper_alpha_cut (hpgs_bezier_clipper_alpha_cut_data *d,
                                    double t0, double t1,
                                    double y0,
                                    double y1,
                                    double y2,
                                    double y3 )
{
  double ymin0 = (y0 < y1) ? y0 : y1;
  double ymin1 = (y2 < y3) ? y2 : y3;
  double ymin = ymin0<ymin1 ? ymin0 : ymin1;

  double ymax0 = (y0 > y1) ? y0 : y1;
  double ymax1 = (y2 > y3) ? y2 : y3;
  double ymax = ymax0>ymax1 ? ymax0 : ymax1;

  double dscan0 = (d->c->y0 - ymax)/d->c->yfac - 0.5;
  double dscan1 = (d->c->y0 - ymin)/d->c->yfac - 0.5;

  double rdscan0 = ceil(dscan0);
  double rdscan1 = floor(dscan1);

  int iscan0 = (int)rdscan0;
  int iscan1 = (int)rdscan1;

  if ((iscan0 > iscan1 &&
       (((y1 != ymin || y0 == ymin) &&
         y2 != ymin &&
         (y1 != ymax || y0 == ymax) &&
         y2 != ymax                    ) || ymin == ymax)) ||
      iscan1 < -1 || iscan0 >= d->c->n_scanlines)
    {
      return 0;
    }
  else if (t1-t0 < 1.0e-8 && iscan0 == iscan1)
    {
      int o0;

      if (y1 > y0) o0 = 1;
      else if (y1 < y0) o0 = -1;
      else if (y2 > y0) o0 = 1;
      else if (y2 < y0) o0 = -1;
      else if (y3 > y0) o0 = 1;
      else if (y3 < y0) o0 = -1;
      else o0 = 0;

      int o1;

      if (y2 > y3) o1 = -1;
      else if (y2 < y3) o1 = 1;
      else if (y1 > y3) o1 = -1;
      else if (y1 < y3) o1 = 1;
      else if (y0 > y3) o1 = -1;
      else if (y0 < y3) o1 = 1;
      else o1 = 0;

      // horizontal cut.
      if (o0*o1 == 0) 
        {
          return 0;
        }

      // filter out startpoint hit.
      if (t0 == -0.5 &&
          ((y0 == ymin && dscan1 == rdscan1) ||
           (y0 == ymax && dscan0 == rdscan0)   )) return 0;

      // filter out endpoint hit.
      if (t1 == 0.5 &&
          ((y3 == ymin && dscan1 == rdscan1) ||
           (y3 == ymax && dscan0 == rdscan0)   )) return 0;

      double t = 0.5*(t0+t1);
      double x = hpgs_bezier_path_x(d->path,d->i,t);
      int o;

      if (o0*o1 > 0)
        o=o0;
      else
        o=0;

      return hpgs_bezier_clipper_alpha_cut_data_push(d,t,x,iscan0,256,o);
    }
  else if (iscan0 == iscan1+1 &&
           ((y1 == ymin && y0 != ymin) ||
            y2 == ymin ))
    {
      if (y2 == ymin && y2 == y3)
        {
          if (t1 != 0.5)
            {
              int order = (int)((dscan1-rdscan1)*256.0+0.5);
              double x = hpgs_bezier_path_x(d->path,d->i,t1);
              return hpgs_bezier_clipper_alpha_cut_data_push(d,t1,x,iscan0,order,0);
            }
          else
            return 0;
        }
      else
        return bezier_clipper_alpha_cut_isolate_extrema (d,t0,t1,y0,y1,y2,y3,HPGS_FALSE);
    }
  else if (iscan0 == iscan1+1 &&
           ((y1 == ymax && y0 != ymax) ||
            y2 == ymax   ))
    {
      if (y2 == ymax && y2 == y3)
        {
          if (t1 != 0.5)
            {
              int order = (int)((dscan1-rdscan1)*256.0+0.5);
              double x = hpgs_bezier_path_x(d->path,d->i,t1);
              return hpgs_bezier_clipper_alpha_cut_data_push(d,t1,x,iscan0,order,0);
            }
          else
            return 0;
        }

      return bezier_clipper_alpha_cut_isolate_extrema (d,t0,t1,y0,y1,y2,y3,HPGS_TRUE);
    }
  else if (t1-t0 < 1.0e-15)
    {
      return 0;
    }
  else
    {
      // partition the spline.
      double tmid = 0.5*(t0+t1);

      // midpoint.
      double ymid,y1l,y2l,y1u,y2u;
      
      y1l = 0.5 * (y0 + y1);

      y2l = 0.25 * (y0 + y2) + 0.5 * y1;

      ymid =
        (y0 + y3) * 0.125 + 0.375 * (y1 + y2);

      y1u = 0.25 * (y1 + y3) + 0.5 * y2;

      y2u = 0.5 * (y2 + y3);

      if (bezier_clipper_alpha_cut (d,t0,tmid,y0,y1l,y2l,ymid))
        return -1;

      if (bezier_clipper_alpha_cut (d,tmid,t1,ymid,y1u,y2u,y3))
        return -1;
          
      return 0;
    }
}

/*!
   Cuts the border of the given \c path with the scanlines of the given
   clipper \c cand creates alpha slopes on the generated scanline
   intersection points. This methid is used in order to create the
   basic data for antialiasing output in \c scanline_emit_alpha

   Return value:
   \li 0 Success.
   \li -1 The system is out of memory.   
*/
static int hpgs_paint_clipper_alpha_cut(hpgs_paint_clipper *c,
                                        hpgs_paint_path *path)
{
  int i;
  int iscan;

  if (path->n_points < 2) return 0;

  // catch path' which lie outside of the area of interest.
  if (path->bb.urx < c->bb.llx) return 0;
  if (path->bb.llx > c->bb.urx) return 0;
  if (path->bb.ury < c->bb.lly) return 0;
  if (path->bb.lly > c->bb.ury) return 0;

  for (i=0;i<path->n_points;++i)
    {
      hpgs_path_point *point = &path->points[i];
      hpgs_path_point *next_point;
      double dscan0,dscan1,rdscan0,rdscan1,x0,y0,x1,y1;
      int iscan0,iscan1,o,order;

      switch (point->flags & HPGS_POINT_ROLE_MASK)
	{
	case HPGS_POINT_LINE:
	case HPGS_POINT_FILL_LINE:
	  next_point = &path->points[i+1];

	  if (next_point->p.y < point->p.y)
            {
              o = 1;

              x0 = point->p.x;
              y0 = point->p.y;
              x1 = next_point->p.x;
              y1 = next_point->p.y;
            }
          else
            {
              o = -1;

              x0 = next_point->p.x;
              y0 = next_point->p.y;
              x1 = point->p.x;
              y1 = point->p.y;
            }

          dscan0 = (c->y0 - y0)/c->yfac + 0.5;
          dscan1 = (c->y0 - y1)/c->yfac  + 0.5;

          rdscan0 = floor(dscan0);
          rdscan1 = floor(dscan1);

          iscan0 = (int)rdscan0;
          iscan1 = (int)rdscan1;

	  if (iscan0 < c->n_scanlines)
            {
              int last_order;
              double last_x;

              if (iscan0 < 0)
                {
                  if (iscan1 < 0) continue;

                  double y = c->y0 + 0.5 * c->yfac;

                  if (x0!=x1)
                    last_x =
                      (x0 * (y1 - y) + x1 * (y - y0)  ) / (y1 - y0);
                  else
                    last_x = x0;

                  iscan0 = 0;
                  last_order = 0;
                  c->iscan0 = 0;
                }
              else
                {
                  last_order = o * (int)(256 * (dscan0 - rdscan0) + 0.5);
                  last_x = x0;

                  if (iscan0 < c->iscan0)
                    c->iscan0 = iscan0;
                }

              if (x0 != x1)
                for (iscan=iscan0;iscan<iscan1 && iscan < c->n_scanlines;++iscan)
                  {
                    double y = c->y0 - (iscan + 0.5) * c->yfac;
                    
                    double x =
                      (x0 * (y1 - y) + x1 * (y - y0)  ) / (y1 - y0);
                    
                    if (hpgs_paint_scanline_add_slope(c->scanlines+iscan,last_x,x,last_order,o*256))
                      return -1;

                    last_x = x;
                    last_order = 0;
                  }
              else
                for (iscan=iscan0;iscan<iscan1 && iscan < c->n_scanlines;++iscan)
                  {
                    if (hpgs_paint_scanline_add_slope(c->scanlines+iscan,last_x,last_x,last_order,o*256))
                      return -1;

                    last_order = 0;
                  }
              
              if (iscan < c->n_scanlines)
                {
                  order = o*(int)(256 * (dscan1 - rdscan1) + 0.5);

                  if (hpgs_paint_scanline_add_slope(c->scanlines+iscan,last_x,x1,last_order,order))
                    return -1;

                  if (iscan > c->iscan1) c->iscan1=iscan;
                }
              else
                c->iscan1 = c->n_scanlines-1;
            }

	  break;
	case HPGS_POINT_BEZIER:
	  next_point = &path->points[i+3];

          {
            double yc0,yc1;

            // start point.
            x0 = point->p.x;
            y0 = point->p.y;

            // end point
            x1 =  next_point->p.x;
            y1 =  next_point->p.y;
            
            // control points
            yc0 = path->points[i+1].p.y;
            yc1 = path->points[i+2].p.y;

            dscan0 = (c->y0 - y0)/c->yfac + 0.5;
            rdscan0 = floor(dscan0);

            hpgs_bezier_clipper_alpha_cut_data d =
              { c,path,i,-0.5,x0,(int)((dscan0-rdscan0)*256.0+0.5),(int)rdscan0 };

            if (bezier_clipper_alpha_cut (&d,-0.5,0.5,y0,yc0,yc1,y1))
              return -1;

            // add endpoint with appropriate order.
            if (d.iscan_last >= 0 && d.iscan_last < c->n_scanlines)
              {
                dscan1 = (c->y0 - y1)/c->yfac + 0.5;
                rdscan1 = floor(dscan1);
                iscan1=(int)rdscan1;

                if (d.iscan_last < c->iscan0)
                  c->iscan0 = d.iscan_last;
                
                if (d.iscan_last > c->iscan1)
                  c->iscan1 = d.iscan_last;
                
                order = (int)((dscan1-rdscan1)*256.0+0.5);

                if (hpgs_paint_scanline_add_slope(c->scanlines+d.iscan_last,
                                                  d.x_last,x1,
                                                  d.o_last,
                                                  order))
                  return -1;
              }
          }
          // ignore control points.
          i+=2;
	  break;
	}
    }

#ifdef HPGS_DEBUG_ALPHA_CUT
  for (iscan = c->iscan0;iscan<=c->iscan1;++iscan)
    {
      int j;
      int so = 0;

      hpgs_log("iscan = %d, np = %d ****************************************\n",
               iscan,c->scanlines[iscan].n_points);

      for (j=0;j<c->scanlines[iscan].n_points;++j)
        {
          so += c->scanlines[iscan].points[j].order;
          hpgs_log("%lg(%d,%d) ",
                   c->scanlines[iscan].points[j].x,
                   c->scanlines[iscan].points[j].order,so);
        }

      if (so)
        hpgs_log("so !!!!!!!!!!!!!!!");
      
      hpgs_log("\n");
    }
#endif

  return 0;
}

static int hpgs_paint_scanline_reserve_point(hpgs_paint_scanline *s)
{
  if (s->n_points >= s->points_malloc_size)
    {
      int nsz = s->points_malloc_size * 2;
      hpgs_scanline_point *np = (hpgs_scanline_point *)
	realloc(s->points,
		sizeof(hpgs_scanline_point)*nsz);

      if (!np) return -1;
      s->points = np;
      s->points_malloc_size = nsz;
    }

  return 0;
}

/*!
   Adds an intersection point with coordinate \c x and order \c 0 to the
   given scanline \c s.

   Return value:
   \li 0 Success.
   \li -1 The system is out of memory.   
*/
int hpgs_paint_scanline_add_point(hpgs_paint_scanline *s, double x, int o)
{
  if (hpgs_paint_scanline_reserve_point(s))
    return -1;

  s->points[s->n_points].x = x; 
  s->points[s->n_points].order = o;
  ++s->n_points;
  return 0;
}

int hpgs_paint_scanline_add_slope(hpgs_paint_scanline *s, double x1, double x2, int o1, int o2)
{
  int o = o2-o1;

  if (x1 > x2)
    {
      double tmp = x1;
      x1 = x2;
      x2 = tmp;
    }

  // binary search for x1.
  int i0 = 0;
  int i1 = s->n_points;

  while (i1>i0)
    {
      int i = i0+(i1-i0)/2;

      if (s->points[i].x < x1)
        i0 = i+1;
      else
        i1 = i;
    }

  // insert x1, if not found
  if (i0 >= s->n_points || s->points[i0].x > x1)
    {
      if (hpgs_paint_scanline_reserve_point(s))
        return -1;

      if (s->n_points - i0)
        memmove(s->points+i0+1,s->points+i0,
                sizeof(hpgs_scanline_point) * (s->n_points - i0));
      ++s->n_points;

      // interpolate order of already exiting segment.
      s->points[i0].x = x1; 
      int order =
        (i0 > 0 && i0+1 < s->n_points) ?
        (int)((s->points[i0+1].order * (x1-s->points[i0-1].x)  )/
              (s->points[i0+1].x-s->points[i0-1].x)+0.5          )
        : 0;

      s->points[i0].order = order;

      // subtract intermediate order from already existing next point.
      ++i0;
      if (order)
        s->points[i0].order -= order;
    }
  else
    ++i0;

  int last_order = 0;

  // go on to x2.
  while (i0 < s->n_points &&  s->points[i0].x < x2)
    {
      int order =
        (int)((o * (s->points[i0].x-x1))/(x2-x1)+0.5);

      s->points[i0].order += order - last_order;
      last_order = order;
      ++i0;
    }

  // insert x2, if not found
  if (i0 >= s->n_points || s->points[i0].x > x2)
    {
      if (hpgs_paint_scanline_reserve_point(s))
        return -1;

      if (s->n_points - i0)
        memmove(s->points+i0+1,s->points+i0,
                sizeof(hpgs_scanline_point) * (s->n_points - i0));
      ++s->n_points;

      // interpolate order of already exiting segment.
      int order =
        (i0 > 0 && i0+1 < s->n_points) ?
        (int)((s->points[i0+1].order * (x2-s->points[i0-1].x)  )/
              (s->points[i0+1].x-s->points[i0-1].x)+0.5          )
        : 0;

      s->points[i0].x = x2; 
      s->points[i0].order = order + o - last_order;

      // subtract intermediate order from already existing next point.
      ++i0;
      if (order)
        s->points[i0].order -= order;
    }
  else
    // x2 already present -> add point.
    s->points[i0].order += o-last_order;

  return 0;
}

/*!
   Cuts te given \c path with the scanlines of the given
   clipper \c c. If you emit the clipper afterwards, the interior
   of the given path is drawn to the image.

   If you want to stroke the path use either \c hpgs_paint_clipper_thin_cut
   for thin lines or contruct the outline of a thick line using
   the line style of a graphics state with \c hpgs_paint_path_stroke_path.

   Return value:
   \li 0 Success.
   \li -1 The system is out of memory.   
*/
int hpgs_paint_clipper_cut(hpgs_paint_clipper *c,
			   hpgs_paint_path *path)
{
  if (c->overscan)
    return hpgs_paint_clipper_alpha_cut(c,path);
  else
    return hpgs_paint_clipper_cut_0(c,path);
}


/*! Sets the intersection of the given scanlines \c orig and \c clip
    to the scanline \c res. This is the worker function, which
    finds the intersection of visible segment of \c orig with the
    visible segments of \c clip and adds them to \c res.

    If \c winding is \c HPGS_TRUE, the non-zero winding rule is used for the
    segment intersection, otherwise the exclusive-or rule applies.

    Return values:

    \li 0 Sucess.
    \li -1 The system is out of memory.   
*/
static int hpgs_paint_scanline_clip(hpgs_paint_scanline *res,
                                    const hpgs_paint_scanline *orig,
                                    const hpgs_paint_scanline *clip,
                                    hpgs_bool winding)
{
  int io = 0;
  int ic = 0;
  int owind=0;
  int cwind=0;

  int last_out_state = 0;
  double last_x=0.0;

  res->n_points = 0;

  while (io < orig->n_points && ic < clip->n_points)
    {
      int out_state;
      double x= orig->points[io].x;

      if (clip->points[ic].x < x) x = clip->points[ic].x;

      while (io < orig->n_points && x == orig->points[io].x)
	{
	  owind += orig->points[io].order;
	  ++io;
	}

      while (ic < clip->n_points && x == clip->points[ic].x)
	{
	  cwind += clip->points[ic].order;
	  ++ic;
	}
  
      out_state =
	cwind && ((winding && owind) || (!winding && (owind & 1)));

      if (!out_state && last_out_state)
	{
	  if (hpgs_paint_scanline_add_point(res,last_x,1) ||
	      hpgs_paint_scanline_add_point(res,x,-1)   )
	    return -1;
	}

      last_out_state = out_state;
      if (out_state) last_x = x;
    }

  return 0;
}

/*! Sets the intersection of the given scanlines \c orig and \c clip
    to the scanline \c res. This is the worker function, which
    finds the intersection of visible segment of \c orig with the
    visible segments of \c clip and adds them to \c res.

    Antialiasing method with broken down winding counts for alpha
    calculation.

    If \c winding is \c HPGS_TRUE, the non-zero winding rule is used for the
    segment intersection, otherwise the exclusive-or rule applies.

    Return values:

    \li 0 Sucess.
    \li -1 The system is out of memory.   
*/
static int hpgs_paint_scanline_clip_alpha(hpgs_paint_scanline *res,
                                          const hpgs_paint_scanline *orig,
                                          const hpgs_paint_scanline *clip,
                                          hpgs_bool winding)
{
  int io = 0;
  int ic = 0;
  int owind=0;
  int cwind=0;
  int last_out_state = 0;

  res->n_points = 0;

  while (io < orig->n_points && ic < clip->n_points)
    {
      int out_state;
      hpgs_bool have_2;
      double x= orig->points[io].x;

      if (clip->points[ic].x < x) x = clip->points[ic].x;

      if (io < orig->n_points && x == orig->points[io].x)
	{
	  owind += orig->points[io].order;
	  ++io;
	}

      if (ic < clip->n_points && x == clip->points[ic].x)
	{
	  cwind += clip->points[ic].order;
	  ++ic;
	}
  
      if (winding)
        {
          if (owind <= -256 || owind >= 256)
            out_state = 256;
          else if (owind >= 0)
            out_state = owind;
          else
            out_state = -owind;
        }
      else
        {
          if (owind & 0x100)
            out_state = 256 - (owind & 0xff);
          else
            out_state = (owind & 0xff);
        }

      if (out_state > cwind) out_state=cwind;

      if (hpgs_paint_scanline_add_point(res,x,out_state-last_out_state))
        return -1;

      last_out_state = out_state;

      have_2 = HPGS_FALSE;

      while (io < orig->n_points && x == orig->points[io].x)
	{
	  owind += orig->points[io].order;
	  ++io;
          have_2 = HPGS_TRUE;
	}

      while (ic < clip->n_points && x == clip->points[ic].x)
	{
	  cwind += clip->points[ic].order;
	  ++ic;
          have_2 = HPGS_TRUE;
	}

      if (have_2)
        {
          if (winding)
            {
              if (owind <= -256 || owind >= 256)
                out_state = 256;
              else if (owind >= 0)
                out_state = owind;
              else
                out_state = -owind;
            }
          else
            {
              if (owind & 0x100)
                out_state = 256 - (owind & 0xff);
              else
                out_state = (owind & 0xff);
            }
          
          if (out_state > cwind) out_state=cwind;
          
          if (out_state != last_out_state)
            {
              if (hpgs_paint_scanline_add_point(res,x,out_state-last_out_state))
                return -1;
          
              last_out_state = out_state;
            }
        }
    }

  return 0;
}

/*! Returna a new clipper on the heap, which holds the intersection of the
    given path clipper with the current clip clipper.

    If \c winding is \c HPGS_TRUE, the non-zero winding rule is used for the
    path intersection, otherwise the exclusive-or rule applies.

    Use \c hpgs_paint_clipper_destroy in order to destroy the returned
    \c hpgs_paint_clipper from the heap.

    Returns a null pointer, if the system is out of memory.   
*/
hpgs_paint_clipper *hpgs_paint_clipper_clip(const hpgs_paint_clipper *orig,
					    const hpgs_paint_clipper *clip,
					    hpgs_bool winding)
{
  hpgs_paint_clipper *ret = 0;
  int i;

  if (clip->height != orig->height ||
      clip->n_scanlines != orig->n_scanlines)
    return ret;

  ret = hpgs_new_paint_clipper(&orig->bb,
			       orig->height,
			       16,orig->overscan);

  if (!ret) return ret;

#ifdef HPGS_DEBUG_CLIP
  hpgs_log("orig: iscan0,iscan1 = %d,%d.\n",orig->iscan0,orig->iscan1);
  hpgs_log("clip: iscan0,iscan1 = %d,%d.\n",clip->iscan0,clip->iscan1);
#endif

  ret->iscan0 = orig->iscan0 > clip->iscan0 ? orig->iscan0 : clip->iscan0;
  ret->iscan1 = orig->iscan1 < clip->iscan1 ? orig->iscan1 : clip->iscan1;

  if (orig->overscan)
    {
      for (i=ret->iscan0;i<=ret->iscan1;++i)
        {
          if (hpgs_paint_scanline_clip_alpha(ret->scanlines+i,
                                             orig->scanlines+i,
                                             clip->scanlines+i,winding))
            { hpgs_paint_clipper_destroy(ret); ret =0; break; }
        }
    }
  else
    {
      for (i=ret->iscan0;i<=ret->iscan1;++i)
        {
          if (hpgs_paint_scanline_clip(ret->scanlines+i,
                                       orig->scanlines+i,
                                       clip->scanlines+i,winding))
            { hpgs_paint_clipper_destroy(ret); ret =0; break; }
        }
    }

  return ret;
}

/* Internal:
   Worker routine, which finds the visible segments from the
   intersection of the two scanlines and writes them to the physical
   row\c iy of the image.   
*/
static int scanline_emit_0(hpgs_image *image,
			   double llx, double urx, int iy,
			   const hpgs_paint_scanline *img,
			   const hpgs_paint_scanline *clip,
			   const hpgs_paint_color *c,
			   hpgs_bool winding, hpgs_bool stroke)
{
  int io = 0;
  int ic = 0;
  int owind=0;
  int cwind=0;

  int last_out_state = 0;
  double last_x=0.0;

  while (io < img->n_points && ic < clip->n_points)
    {
      int out_state;
      double x= img->points[io].x;

      if (clip->points[ic].x < x) x = clip->points[ic].x;

      while (io < img->n_points && x == img->points[io].x)
	{
	  owind += img->points[io].order;
	  ++io;
	  // output zero-width chunks for stroke,
	  // so we quit here.
	  if (stroke) break;
	}

      while (ic < clip->n_points && x == clip->points[ic].x)
	{
	  cwind += clip->points[ic].order;
	  ++ic;
	}
  
      if (winding)
	out_state = cwind && owind;
      else 
	out_state = cwind && (owind & 1);

      if (last_out_state && out_state!=last_out_state)
	{
	  // overestimate the thickness of chunks
	  int ix1 = (int)(image->width*(last_x - llx)/(urx-llx));
	  int ix2 = (int)(image->width*(x - llx)/(urx-llx));

	  if (ix1 < 0) ix1=0; 
	  else if (ix1 >= image->width) ix1=image->width-1;

	  if (ix2 < 0) ix2=0; 
	  else if (ix2 >= image->width) ix2=image->width-1;

	  if (ix2<ix1) ix2=ix1;

#ifdef HPGS_DEBUG_EMIT_0
	  hpgs_log("last_x,x,ix1,ix2,last_out_state,out_state = %lg,%lg,%d,%d,%d,%d.\n",
                   last_x,x,ix1,ix2,last_out_state,out_state);

#endif

	  if (hpgs_image_rop3_chunk (image,ix1,ix2,iy,c))
	    return -1;
	}

      if (out_state!=last_out_state)
	{
	  last_x = x;
	  last_out_state = out_state;
	}
    }

  return 0;
}

/* Internal:
   Worker routine, which does the alpha calculation from n scanlines and
   writes the calculated pixels to a physical row of the image.

   The calculation of the alpha values is based on broken down winding
   values. A non-antialiased winding value of 1 is represented as
   a broken down winding value of 256, value between 0 and 256
   correspond to the corresponding alpha value in the interval [0.0,1.0].

*/
static int scanline_emit_alpha(hpgs_image *image,
                               double llx, double urx, int iy,
                               const hpgs_paint_scanline *img,
                               const hpgs_paint_scanline *clip,
                               const hpgs_paint_color *c,
                               hpgs_bool winding)
{
  int io = 0.0;
  int ic = 0.0;
  int owind = 0.0;
  int cwind = 0.0;
  double last_alpha = 0.0;

  int last_partial_ix = -1;
  double last_partial_alpha = 0.0;

  double last_x=0.0;
  double clip_last_x=0.0;

  while (io < img->n_points && ic < clip->n_points)
    {
      int out_state;
      double x= img->points[io].x;
      double alpha;
      double clip_alpha;

      if (clip->points[ic].x < x) x = clip->points[ic].x;

      if (io < img->n_points && x == img->points[io].x)
	{
	  owind += img->points[io].order;
	  ++io;
	}

      if (ic < clip->n_points && x == clip->points[ic].x)
	{
          clip_last_x = clip->points[ic].x;
	  cwind += clip->points[ic].order;
	  ++ic;
	}
  
      if (ic == 0 || ic >= clip->n_points || x == clip_last_x)
        clip_alpha = (double)cwind/256.0;
      else
        clip_alpha =
          (cwind+clip->points[ic].order*(x-clip_last_x)/(clip->points[ic].x-clip_last_x))/256.0;

#ifdef HPGS_DEBUG_EMIT_ALPHA
      hpgs_log("ic,x,clip_last_x,cx,cwind,o,clip_alpha = %d,%lg,%lg,%lg,%d,%d,%lg.\n",
               ic,x,clip_last_x,
               ic < clip->n_points ? clip->points[ic].x : -1.0e20,
               cwind,
               ic < clip->n_points ? clip->points[ic].order : -1,
               clip_alpha);
#endif

      if (winding)
        {
          if (owind <= -256 || owind >= 256)
            out_state = 256;
          else if (owind >= 0)
            out_state = owind;
          else
            out_state = -owind;
        }
      else
        {
          if (owind & 0x100)
            out_state = 256 - (owind & 0xff);
          else
            out_state = (owind & 0xff);
        }


      alpha = out_state /  256.0;

      if (alpha > clip_alpha)
        alpha=clip_alpha;

      if (x>last_x && (alpha > 0.0 || last_alpha > 0.0))
	{
	  double x1 = image->width*(last_x - llx)/(urx-llx);
	  double x2 = image->width*(x - llx)/(urx-llx);

	  int i,ix1,ix2;

#ifdef HPGS_DEBUG_EMIT_ALPHA
	  hpgs_log("last_x,x,out_state,last_alpha,alpha = %lg,%lg,%d,%lg,%lg.\n",
                   last_x,x,out_state,last_alpha,alpha);

#endif
	  ix1 = (int)floor(x1);
	  ix2 = (int)floor(x2);

	  if (ix1 < -1) ix1=-1; 
	  else if (ix1 >= image->width) ix1=image->width-1;

	  if (ix2 < 0) ix2=0; 
	  else if (ix2 >= image->width) ix2=image->width;

	  // alpha value for a given pixel position.
#define ALPHA_X(xx)  ((last_alpha*(x2-(xx))+alpha*((xx)-x1))/(x2-x1))

	  // emit first partial pixel.
	  if (ix1 >= 0)
	    {
	      double aa;

	      if (ix2 > ix1)
		aa = (1.0 - (x1 - ix1)) * 0.5 * (last_alpha+ALPHA_X(ix1+1));
	      else
		aa = (x2 - x1) * 0.5 * (last_alpha + alpha);

	      if (ix1 != last_partial_ix)
		{
		  if (last_partial_ix >= 0 &&
		      hpgs_image_rop3_pixel (image,last_partial_ix,iy,
                                             c,last_partial_alpha))
		    return -1;

		  last_partial_ix = ix1;
		  last_partial_alpha = 0.0;
		}

	      last_partial_alpha += aa;
	    }

	  // emit solid core segment.
	  if (last_alpha == 1.0 && alpha == 1.0)
	    {
	      if (ix2 > ix1+1 && hpgs_image_rop3_chunk (image,ix1+1,ix2-1,iy,c))
		return -1;
	    }
	  else
	    {
	      for (i=ix1+1;i<ix2;++i)
		{
		  if (hpgs_image_rop3_pixel (image,i,iy,c,
                                             ALPHA_X(i+0.5)))
		    return -1;
		}
	    }

	  // emit last partial pixel.
	  if (ix2 < image->width && ix2 > ix1)
	    {
	      double aa = (x2 - ix2) * 0.5 * (alpha + ALPHA_X(ix2));

	      if (ix2 != last_partial_ix)
		{
		  if (last_partial_ix >= 0 &&
		      hpgs_image_rop3_pixel (image,last_partial_ix,iy,
                                             c,last_partial_alpha))
		    return -1;

		  last_partial_ix = ix2;
		  last_partial_alpha = 0.0;
		}

	      last_partial_alpha += aa;
	    }
#undef ALPHA_X
	}

      last_alpha = alpha;
      last_x = x;
    }

  if (last_partial_ix >= 0 &&
      hpgs_image_rop3_pixel (image,last_partial_ix,iy,
                            c,last_partial_alpha))
    return -1;

  return 0;
}

/*! Emits the intersection of the visible segments of the given clippers
    \c img and \c clip to the given image \c img using
    the given color \c c.

    If \c stroke is \c HPGS_TRUE, zero-width segments are also emitted, which
    is feasible for clippers retrieved through \c hpgs_paint_clipper_thin_cut.

    If \c winding is \c HPGS_TRUE, the non-zero winding rule is used for the
    segment intersection, otherwise the exclusive-or rule applies.

    Return values:

    \li 0 Sucess.
    \li -1 The system is out of memory or an error from the image has occurred.   
*/
int hpgs_paint_clipper_emit (hpgs_image *image,
			     const hpgs_paint_clipper *img,
			     const hpgs_paint_clipper *clip,
			     const hpgs_paint_color *c,
			     hpgs_bool winding, hpgs_bool stroke)
{
  int iscan0,iscan1,iscan;

  if (image->height != img->height ||
      image->height != clip->height ||
      clip->n_scanlines != img->n_scanlines)
    return -1;

  iscan0 = img->iscan0 > clip->iscan0 ? img->iscan0 : clip->iscan0;
  iscan1 = img->iscan1 < clip->iscan1 ? img->iscan1 : clip->iscan1;

#if defined (HPGS_DEBUG_EMIT_0) || defined(HPGS_DEBUG_EMIT_ALPHA)
  hpgs_log("emit: iscan0,iscan1 =  %d,%d *************************\n",iscan0,iscan1);
#endif

  if (img->overscan)
    {
      for (iscan = iscan0; iscan<=iscan1; ++iscan)
        {
#ifdef HPGS_DEBUG_EMIT_ALPHA
          hpgs_log("emit_alpha: iscan =  %d ********************\n",iscan);
#endif
          if (scanline_emit_alpha(image,img->bb.llx,img->bb.urx,iscan,
                                  img->scanlines + iscan,
                                  clip->scanlines + iscan,
                                  c,winding))
            return -1;
        }
    }
  else
    {
      for (iscan = iscan0; iscan<=iscan1; ++iscan)
        {
#ifdef HPGS_DEBUG_EMIT_0
          hpgs_log("emit_0: iscan =  %d **************************\n",iscan);
#endif
          if (scanline_emit_0(image,img->bb.llx,img->bb.urx,iscan,
                              img->scanlines + iscan,
                              clip->scanlines + iscan,
                              c,winding,stroke))
            return -1;
        }
    }
  return 0;
}

/*! Returns a new clipper on the heap, which covers the given bounding box
    an applies to an image with \c height rows.

    \c scanline_msize determines the number of intersection points, for which
    memory is preallocated in each scanline.

    The value of \c overscan determines the type of antialiasing applied as
    described under \c hpgs_paint_device_st::overscan.

    Use \c hpgs_paint_clipper_destroy in order to destroy the returned
    \c hpgs_paint_clipper from the heap.

    Returns a null pointer, if the system is out of memory.   
*/
hpgs_paint_clipper *hpgs_new_paint_clipper(const hpgs_bbox * bb,
                                           int height,
					   int scanline_msize,
					   int overscan)
{
  hpgs_paint_clipper *ret =
    (hpgs_paint_clipper *)malloc(sizeof(hpgs_paint_clipper));

  if (ret)
    {
      int i;

      ret->bb = *bb;

      ret->n_scanlines = height;
      // slightly reinterpret lly and ury in order to
      // meet scanline y coordinates
      ret->yfac = (bb->ury-bb->lly) / height;
      ret->y0 = bb->ury - 0.5 * ret->yfac;
  
      ret->overscan = overscan;
      ret->height = height;
      ret->iscan0 = ret->n_scanlines;
      ret->iscan1 = -1;

      ret->scanlines = (hpgs_paint_scanline *)
	malloc(sizeof(hpgs_paint_scanline)*ret->n_scanlines);

      if (ret->scanlines)
	for (i=0;i<ret->n_scanlines;++i)
	  {
	    if (hpgs_paint_scanline_init(ret->scanlines+i,scanline_msize))
	      break;
	  }
      else
	i=-1;

      // error recovery
      if (i<ret->n_scanlines)
	{
	  for (;i>=0;--i)
	    hpgs_paint_scanline_cleanup(ret->scanlines+i);

	  if (ret->scanlines) free(ret->scanlines);

	  free(ret);
	  ret = 0;
	}
    }
  return ret;
}

/*! Resets a  clipper to be empty and to cover the whole
    rectanlge of an image covering the \c x coordinates in the range
    from \c llx to \c urx.

    \li 0 Sucess.
    \li -1 The system is out of memory.   
*/
int hpgs_paint_clipper_reset(hpgs_paint_clipper *c, double llx, double urx)
{
  int i;

  if (c->overscan)
    {
      for (i=0;i<c->n_scanlines;++i)
        {
          hpgs_paint_scanline *s =  c->scanlines+i;

          s->n_points=0;
          if (hpgs_paint_scanline_add_point(s,llx,0)   ||
              hpgs_paint_scanline_add_point(s,llx,256) ||
              hpgs_paint_scanline_add_point(s,urx,0)   ||
              hpgs_paint_scanline_add_point(s,urx,-256)  )
            return -1;
        }
    }
  else
    {
      for (i=0;i<c->n_scanlines;++i)
        {
          hpgs_paint_scanline *s =  c->scanlines+i;

          s->n_points=0;
          if (hpgs_paint_scanline_add_point(s,llx,1) ||
              hpgs_paint_scanline_add_point(s,urx,-1)   )
            return -1;
        }
    }

  c->iscan0 = 0;
  c->iscan1 = c->n_scanlines-1;

  return 0;
}

/*! Resets a clipper to be completely empty .

    \li 0 Sucess.
    \li -1 The system is out of memory.   
*/
void hpgs_paint_clipper_clear(hpgs_paint_clipper *c)
{
  int i;

  for (i=c->iscan0;i<=c->iscan1;++i)
    {
      hpgs_paint_scanline *s =  c->scanlines+i;

      s->n_points=0;
    }

  c->iscan0 = c->n_scanlines;
  c->iscan1 = -1;
}

/*! Destroys a clipper from the heap.   
*/
void hpgs_paint_clipper_destroy(hpgs_paint_clipper *c)
{
  int i;

  if (c->scanlines)
    {
      for (i=0;i<c->n_scanlines;++i)
	hpgs_paint_scanline_cleanup(c->scanlines+i);

      free(c->scanlines);
    }
  
  free(c);
}

