/***********************************************************************
 *                                                                     *
 * $Id: hpgspaintpath.c 369 2007-01-13 22:44:32Z softadm $
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
 * The implementation of the API for paint path handling.              *
 *                                                                     *
 ***********************************************************************/

#include<hpgspaint.h>
#include<assert.h>
#include<math.h>
#include<string.h>

// #define HPGS_STROKE_DEBUG

/*! \defgroup path Path contruction functions.

  This module contains the documentation of the path contruction
  layer of the vector renderer \c hpgs_paint_device.

  The dcomumentation of \c hpgs_paint_path_st explains the
  most important parts of this module.
*/

/*! Returns a new, empty path created on the heap.

    Use \c hpgs_paint_path_destroy in order to destroy the returned
    \c hpgs_paint_path from the heap.

    Returns a null pointer, if the system is out of memory.   
*/
hpgs_paint_path *hpgs_new_paint_path(void)
{
  hpgs_paint_path *ret = (hpgs_paint_path *)malloc(sizeof(hpgs_paint_path));

  if (ret)
    {
      ret->points_malloc_size = 256;
      ret->points = (hpgs_path_point *)
	malloc(sizeof(hpgs_path_point)*ret->points_malloc_size);

      ret->n_points = 0;
      ret->last_start = 0;

      hpgs_bbox_null(&ret->bb);

      if (!ret->points)
	{
	  free(ret);
	  ret = 0;
	}
    }

  return ret;
}

/*! Destroys a path from the heap.   
*/
void hpgs_paint_path_destroy(hpgs_paint_path *_this)
{
  if (_this->points)
    free(_this->points);

  free(_this);
}

/*! Resets the given path to be empty.   
*/
void hpgs_paint_path_truncate(hpgs_paint_path *_this)
{
  hpgs_bbox_null(&_this->bb);

  _this->n_points = 0;
  _this->last_start = 0;
}

/* Internal:
   Pushes a point with given flags to the path \c _this.
*/
static int push_path_point (hpgs_paint_path *_this,
			    const hpgs_point *p,
			    int flags)
{
  if (_this->n_points >= _this->points_malloc_size)
    {
      _this->points_malloc_size *= 2;
      
      _this->points = (hpgs_path_point *)
	realloc(_this->points,sizeof(hpgs_path_point)*_this->points_malloc_size);

      if (!_this->points) return -1;
    }

  hpgs_path_point *cp = _this->points + _this->n_points;

  cp->p = *p;
  cp->flags = flags;
  
  hpgs_bbox_add(&_this->bb,p);

  ++_this->n_points;

  return 0;
}

/*! Adds a new point to the path. If the path already has some points,
    the current subpolygon is implicitly closed.   
*/
int hpgs_paint_path_moveto(hpgs_paint_path *_this,
			   const hpgs_point *p  )
{
#ifdef HPGS_STROKE_DEBUG
  hpgs_log("%lg %lg moveto\n",p->x,p->y);
#endif

  // check for current point and do an implicit path close.
  if (_this->n_points > 0)
    {
      hpgs_path_point *cp = _this->points + (_this->n_points - 1);

      if ((cp->flags & HPGS_POINT_SUBPATH_END) == 0)
	{ 
	  assert(_this->last_start < _this->n_points);

	  hpgs_path_point *sp = _this->points + _this->last_start;

	  if (cp->p.x == sp->p.x && cp->p.y == sp->p.y)
	    cp->flags |= HPGS_POINT_SUBPATH_END;
	  else
	    {
	      // do an implicit closepath.
	      cp->flags &= ~HPGS_POINT_ROLE_MASK;
	      cp->flags |= HPGS_POINT_FILL_LINE;

	      if (push_path_point(_this,&sp->p,HPGS_POINT_SUBPATH_END))
		return -1;
	    }
	}
      else
	// catch a double moveto.
	if ((cp->flags & (HPGS_POINT_SUBPATH_START | HPGS_POINT_DOT)) ==
            HPGS_POINT_SUBPATH_START)
	  {
	    cp->p = *p;
            hpgs_bbox_add(&_this->bb,p);
	    return 0;
	  }
    }

  _this->last_start= _this->n_points;

  if (push_path_point(_this,p,HPGS_POINT_SUBPATH_START))
    return -1;

  return 0;
}

/*! Adds a new point to the path and mark the last segment as line.   
*/
int hpgs_paint_path_lineto(hpgs_paint_path *_this,
			   const hpgs_point *p  )
{
#ifdef HPGS_STROKE_DEBUG
  hpgs_log("%lg %lg lineto\n",p->x,p->y);
#endif

  if (_this->n_points <= 0)
    return -1;

  hpgs_path_point *cp = _this->points + (_this->n_points - 1);

  // zero line ignoration.
  if (cp->p.x == p->x && cp->p.y == p->y)
    {
      if (cp->flags & HPGS_POINT_SUBPATH_START)
        {
          cp->flags &= ~HPGS_POINT_ROLE_MASK;
          cp->flags |= HPGS_POINT_DOT;
        }
      return 0;
    }

  cp->flags &= ~HPGS_POINT_ROLE_MASK;
  cp->flags |= HPGS_POINT_LINE;

  // implicitly start a subpath.
  if (cp->flags & HPGS_POINT_SUBPATH_END)
    {
      cp->flags |= HPGS_POINT_SUBPATH_START;
      _this->last_start = _this->n_points - 1;
    }

  return push_path_point(_this,p,0);
}

/*! Adds three new points to the path and mark the last segment as bezier spline.   
*/
int hpgs_paint_path_curveto(hpgs_paint_path *_this,
			    const hpgs_point *p1,
			    const hpgs_point *p2,
			    const hpgs_point *p3  )
{
#ifdef HPGS_STROKE_DEBUG
  hpgs_log("%lg %lg %lg %lg %lg %lg curveto\n",
	   p1->x,p1->y,
	   p2->x,p2->y,
	   p3->x,p3->y);
#endif

  if (_this->n_points <= 0)
    return -1;

  hpgs_path_point *cp = _this->points + (_this->n_points - 1);

  cp->flags &= ~HPGS_POINT_ROLE_MASK;
  cp->flags |= HPGS_POINT_BEZIER;

  // implicitly start a subpath.
  if (cp->flags & HPGS_POINT_SUBPATH_END)
    {
      cp->flags |= HPGS_POINT_SUBPATH_START;
      _this->last_start = _this->n_points - 1;
    }

  if (push_path_point(_this,p1,HPGS_POINT_CONTROL)) return -1;
  if (push_path_point(_this,p2,HPGS_POINT_CONTROL)) return -1;
  
  return push_path_point(_this,p3,0);
}

/*! Closes the current subpolygon. If the current point is distinct from the
    start point of the current subpolygon, a line is drawn from
    the start point of the current subpolygon to the last point.
*/
int hpgs_paint_path_closepath(hpgs_paint_path *_this)
{
  // check for current point and do an implicit path close.
  if (_this->n_points <= 0) return 0;

  hpgs_path_point *cp = _this->points + (_this->n_points - 1);

  if (cp->flags & (HPGS_POINT_SUBPATH_END | HPGS_POINT_SUBPATH_START))
    return 0;

  assert(_this->last_start < _this->n_points);

  hpgs_path_point *sp = _this->points + _this->last_start;

  if (cp->p.x == sp->p.x && cp->p.y == sp->p.y)
    cp->flags |= (HPGS_POINT_SUBPATH_END | HPGS_POINT_SUBPATH_CLOSE);
  else
    {
      // do an explicit closepath.
      cp->flags &= ~HPGS_POINT_ROLE_MASK;
      cp->flags |= HPGS_POINT_LINE;

      if (push_path_point(_this,&sp->p,
			  HPGS_POINT_SUBPATH_END | HPGS_POINT_SUBPATH_CLOSE))
	return -1;

      _this->last_start=_this->n_points;
    }
  return 0;
}

// caclulate tan(alpha/2) from tan(alpha)
static double half_tan (double x)
{
  return (sqrt(1.0+x*x)-1.0)/x;
}

/*! Adds a buldged arc segment to the path. Buldge is defined to
    be \c tan(alpha/4), where \c alpha is the turning angle of the arc.
*/
int hpgs_paint_path_buldgeto(hpgs_paint_path *_this,
			     const hpgs_point *p,
			     double buldge)
{
  hpgs_point d,p1,p2;
  hpgs_path_point *cp;
  double x1,x2;

  if (_this->n_points <= 0) return -1;

  cp = _this->points + (_this->n_points - 1);

  // calculate sekant.
  d.x = p->x - cp->p.x;
  d.y = p->y - cp->p.y;

  // for large buldges (angle greater than 90 deg),
  // split in to two bezier splines
  if (fabs(buldge)+1.0 > M_SQRT2)
    {
      hpgs_point pm;
      double b2 = half_tan(buldge);

      pm.x = 0.5 * (cp->p.x + p->x) + 0.5 * buldge * d.y;
      pm.y = 0.5 * (cp->p.y + p->y) - 0.5 * buldge * d.x;
      
      if (hpgs_paint_path_buldgeto(_this,&pm,b2)) return -1;
      return hpgs_paint_path_buldgeto(_this,p,b2);
    }

  x1 = 1.0/3.0*(1.0-buldge*buldge);
  x2 = 2.0/3.0*buldge;

  // first ctrl point.
  p1.x = cp->p.x + x1 * d.x + x2 * d.y;
  p1.y = cp->p.y + x1 * d.y - x2 * d.x;

  // second ctrl point.
  p2.x = p->x - x1 * d.x + x2 * d.y;
  p2.y = p->y - x1 * d.y - x2 * d.x;

  return hpgs_paint_path_curveto(_this,&p1,&p2,p);
}

/* Internal: Adds a line join to the extruded path.
   The next extruded point is \c e1, the tangent of
   the previous and current path segment are passed.
*/
static int join_lines(hpgs_paint_path *path, const hpgs_point *edge,
		      const hpgs_point *e1,
		      const hpgs_point *prev_tangent,
		      const hpgs_point *tangent,
		      hpgs_line_join join,
		      double ml)
{
  double sa = prev_tangent->x * tangent->y - prev_tangent->y * tangent->x;
  double ca = prev_tangent->x * tangent->x + prev_tangent->y * tangent->y;

  // inner edge ?
  if (sa < 0.0)
    {
      if (hpgs_paint_path_lineto(path,edge)) return -1;
      return hpgs_paint_path_lineto(path,e1);
    }

  switch (join)
    {
    case hpgs_join_round:
      if (hpgs_paint_path_buldgeto(path,e1,tan(atan2(sa,ca)*0.25))) return -1;
      break;
    case hpgs_join_miter:
      //
      // This is the derivation of the miter condition used below.
      //
      // sin(a/2) = sqrt(0.5*(1.0+ca))
      // 1.0/sin(a/2) >= ml
      // sin(a/2) <= 1.0/ml
      // 0.5*(1.0+ca) <= 1.0/(ml*ml)
      // (1.0+ca) <= 2.0/(ml*ml)

      // additionally, for very small angles the miter effect will not be visible
      // for the kind user, so skip the miter calculation for these, too.
      if ((1.0+ca) >= 2.0/(ml*ml) && sa > 0.01)
	{
	  double det;
	
	  // determine miter point.
	  //
	  // solve:
	  //
	  // e1->x - cp->x = t1 * prev_tangent->x - t2 * tangent->x 
	  // e1->y - cp->y = t1 * prev_tangent->y - t2 * tangent->y 
	  //

	  det = prev_tangent->y * tangent->x - prev_tangent->x * tangent->y;
	
	  if (det > 1.0e-8 || det < -1.0e-8)
	    {
	      hpgs_path_point *cp = path->points + (path->n_points - 1);
	      double t1;
	      double x = e1->x - cp->p.x;
	      double y = e1->y - cp->p.y;
	      hpgs_point mp;

	      t1 = (tangent->x * y - tangent->y * x)/det;
	      // t2 = ( prev_tangent->y * x - prev_tangent->x * y)/det;

	      mp.x = cp->p.x + t1 * prev_tangent->x;
	      mp.y = cp->p.y + t1 * prev_tangent->y;

              return hpgs_paint_path_lineto(path,&mp);
	    }
	}
    default:
    /* hpgs_join_bevel */
      if (hpgs_paint_path_lineto(path,e1)) return -1;
    }
  return 0;
}

/* Internal: Adds a line cap to the extruded path.
   The next extruded point is \c e1.
*/
static int cap_line(hpgs_paint_path *path,
		    const hpgs_point *e1,
		    hpgs_line_cap cap)
{
  switch (cap)
    {
    case hpgs_cap_round:
      if (hpgs_paint_path_buldgeto(path,e1,1.0)) return -1;
      break;
    case hpgs_cap_square:
      {
	hpgs_path_point *cp = path->points + (path->n_points - 1);
	hpgs_point d;
	hpgs_point p;

	d.x = cp->p.x - e1->x;
	d.y = cp->p.y - e1->y;
	
	p.x = cp->p.x + 0.5 * d.y;
	p.y = cp->p.y - 0.5 * d.x;

	if (hpgs_paint_path_lineto(path,&p)) return -1;

	p.x += d.x;
	p.y += d.y;

	if (hpgs_paint_path_lineto(path,&p)) return -1;
      }
    default:
      /* hpgs_cap_butt */
      if (hpgs_paint_path_lineto(path,e1)) return -1;
    }
  return 0;
}

static void normalize(hpgs_point *p)
{
  double l=hypot(p->x,p->y);

  if (l==0.0) return;

  p->x /= l;
  p->y /= l;
}

/* Internal: Extrudes a bezier segment starting at
   point \c i of path \c _this by the half linewidth \c d.

   Start parameter is \c tt1,
   end parameter is \c tt2. Returns the four defining points of the extruded
   bezier spline and the starting/ending tangent.
*/
static void extrude_bezier_internal (const hpgs_paint_path *_this, int i,
                                     double tt1, double tt2, double d,
                                     hpgs_path_point *ep, int *n_ep,
                                     hpgs_point *tan1,
                                     hpgs_point *tan2)
{
  double t_m = 0.5 * (tt1+tt2);
  double alpha1,alpha2,det;
  hpgs_point tan_m;
  hpgs_point e_m;

  hpgs_bezier_path_point(_this,i,tt1,&ep[*n_ep+0].p);

  hpgs_bezier_path_tangent(_this,i,tt1,tt2 > tt1 ? 1 : -1,1.0e-8,tan1);

  if (tt2 < tt1)
    {
      tan1->x = -tan1->x;
      tan1->y = -tan1->y;
    }

  if (*n_ep && tan1->x == tan2->x && tan1->y == tan2->y)
    -- *n_ep;
  else
    {
      if (*n_ep)
        ep[*n_ep-1].flags  = HPGS_POINT_LINE;
      ep[*n_ep+0].p.x += tan1->y * d;
      ep[*n_ep+0].p.y -= tan1->x * d;
      ep[*n_ep+0].flags = HPGS_POINT_BEZIER;
    }

  hpgs_bezier_path_point(_this,i,tt2,&ep[*n_ep+3].p);
  hpgs_bezier_path_point(_this,i,t_m,&e_m);

  hpgs_bezier_path_tangent(_this,i,tt2,tt2 > tt1 ? -1 : 1,1.0e-8,tan2);
  hpgs_bezier_path_tangent(_this,i,t_m, 0,1.0e-8,&tan_m);

  if (tt2 < tt1)
    {
      tan2->x = -tan2->x;
      tan2->y = -tan2->y;
      tan_m.x = -tan_m.x;
      tan_m.y = -tan_m.y;
    }

  // extrude endpoints.
  ep[*n_ep+3].p.x += tan2->y * d;
  ep[*n_ep+3].p.y -= tan2->x * d;
  ep[*n_ep+3].flags = HPGS_POINT_BEZIER;

  e_m.x += tan_m.y * d;
  e_m.y -= tan_m.x * d;

  //
  //  solve: 
  //
  // 8 * e_m.x = 4 * (e1->x + e2->x) + 3*(alpha1 * tan1->x + alpha2 * tan2->x)
  // 8 * e_m.y = 4 * (e1->y + e2->y) + 3*(alpha1 * tan1->y + alpha2 * tan2->y)
  //

  alpha1 = tt2 - tt1;
  alpha2 = tt1 - tt2;

  det =  tan1->x * tan2->y - tan1->y * tan2->x;

#ifdef HPGS_STROKE_DEBUG
  hpgs_log("extrude_bezier: tt1,tt2,det= %lg,%lg,%lg.\n",
	   tt1,tt2,det);
#endif

  if (det)
    {
      double x = 8.0/3.0 *e_m.x - 4.0/3.0 * (ep[*n_ep+0].p.x + ep[*n_ep+3].p.x);
      double y = 8.0/3.0 *e_m.y - 4.0/3.0 * (ep[*n_ep+0].p.y + ep[*n_ep+3].p.y);

      alpha1 = (tan2->y * x - tan2->x * y)/det;
      alpha2 = (tan1->x * y - tan1->y * x)/det;
    }

  ep[*n_ep+1].p.x = ep[*n_ep+0].p.x + alpha1 * tan1->x; 
  ep[*n_ep+1].p.y = ep[*n_ep+0].p.y + alpha1 * tan1->y; 
  ep[*n_ep+1].flags = HPGS_POINT_CONTROL;

  ep[*n_ep+2].p.x = ep[*n_ep+3].p.x + alpha2 * tan2->x; 
  ep[*n_ep+2].p.y = ep[*n_ep+3].p.y + alpha2 * tan2->y;
  ep[*n_ep+2].flags = HPGS_POINT_CONTROL;

  *n_ep += 4;
}

static void extrude_bezier (const hpgs_paint_path *_this, int i,
                     double tt1, double tt2, double d,
                     hpgs_path_point *ep, int *n_ep,
                     hpgs_point *tan1,
                     hpgs_point *tan2)
{
  int nx;
  double tx[2];

  hpgs_bezier_path_singularities(_this,i,
                                 tt1<tt2?tt1:tt2,tt1<tt2?tt2:tt1,
                                 &nx,tx);

  if (tt1>tt2 && nx > 1)
    {
      double tmp = tx[1];
      tx[1] = tx[0];
      tx[0] = tmp;
    }

  if (nx == 0 && fabs(tt2 - tt1) > 0.4)
    {
      // break spline, if it runs by more than 90 degrees.
      if ((_this->points[i+1].p.x - _this->points[i].p.x) *
          (_this->points[i+2].p.x - _this->points[i+1].p.x) +
          (_this->points[i+1].p.y- _this->points[i].p.y) *
          (_this->points[i+2].p.y - _this->points[i+1].p.y) < 0.0)
        ++nx;
      if ((_this->points[i+2].p.x - _this->points[i+1].p.x) *
          (_this->points[i+3].p.x - _this->points[i+2].p.x) +
          (_this->points[i+2].p.y- _this->points[i+1].p.y) *
          (_this->points[i+3].p.y - _this->points[i+2].p.y) < 0.0)
        ++nx;

     switch (nx)
       {
       case 1:
          tx[0] = 0.5 * (tt1 + tt2);
          break;
       case 2:
          tx[0] = .61803398874989484820 * tt1 + .38196601125010515180 * tt2;
          tx[1] = .38196601125010515180 * tt1 + .61803398874989484820 * tt2;
          break;
       default:
          break;
       }
    }
  
  *n_ep = 0;

  if (nx == 0)
    extrude_bezier_internal (_this,i,tt1,tt2,d,ep,n_ep,tan1,tan2);
  else
    {
      hpgs_point tan_m;

      extrude_bezier_internal (_this,i,tt1,tx[0],d,ep,n_ep,tan1,tan2);

      if (nx > 1)
        {
          extrude_bezier_internal (_this,i,tx[0],tx[1],d,ep,n_ep,&tan_m,tan2);
          extrude_bezier_internal (_this,i,tx[1],tt2,d,ep,n_ep,&tan_m,tan2);
        }
      else
        extrude_bezier_internal (_this,i,tx[0],tt2,d,ep,n_ep,&tan_m,tan2);
    }
}


/* Internal: Extrudes a line segment starting at
   point \c i of path \c _this by the half linewidth \c d.

   Start parameter is \c tt1,
   end parameter is \c tt2. Returns the two defining points of the extruded
   line and the starting/ending tangent.
*/
static void extrude_line (const hpgs_paint_path *_this, int i,
			  double tt1, double tt2, double d,
			  hpgs_path_point *ep, int *n_ep,
			  hpgs_point *tangent)
{
  if (tt2 >= tt1)
    {
      tangent->x = _this->points[i+1].p.x - _this->points[i].p.x;
      tangent->y = _this->points[i+1].p.y - _this->points[i].p.y;
    }
  else
    {
      tangent->x = _this->points[i].p.x - _this->points[i+1].p.x;
      tangent->y = _this->points[i].p.y - _this->points[i+1].p.y;
    }

  normalize(tangent);

  ep[0].p.x =
    (0.5-tt1) * _this->points[i].p.x +
    (0.5+tt1) * _this->points[i+1].p.x +
    tangent->y * d;

  ep[0].p.y =
    (0.5-tt1) * _this->points[i].p.y +
    (0.5+tt1) * _this->points[i+1].p.y -
    tangent->x * d;

  ep[0].flags = HPGS_POINT_LINE;

  ep[1].p.x =
    (0.5-tt2) * _this->points[i].p.x +
    (0.5+tt2) * _this->points[i+1].p.x +
    tangent->y * d;

  ep[1].p.y =
    (0.5-tt2) * _this->points[i].p.y +
    (0.5+tt2) * _this->points[i+1].p.y -
    tangent->x * d;

  ep[1].flags = HPGS_POINT_SUBPATH_END;

  *n_ep = 2;
}

/* Internal:
   Extrude a subpath (dash segment) of \this path by the half linewidth
   \c d and append the result to \c path.
  
   The subpath starts at the path segment starting at point \c i0 at the
   curve parameter \c t0 (t=-0.5 start of segment, t=0.5 end of segment).

   The subpath ends at the path segment starting at point \c i1 at the
   curve parameter \c t1.

   The extruded path is appended to \c path. The linecaps for the start
   and end of the subpath are passed and the line join.  

   if \c closed is non-zero, no line cap is applied and the extruded
   path consists of two closed polygons, one interior and one exterior
   border.
*/
static int extrude_segment (const hpgs_paint_path *_this,
			    hpgs_paint_path *path,
			    int i0, double t0, hpgs_line_cap cap0,
			    int i1, double t1, hpgs_line_cap cap1,
			    double d, int closed,
			    hpgs_line_join join, double ml)
{
  int i;

  hpgs_point start_point;
  hpgs_point start_tangent;
  hpgs_point prev_tangent;

  // forward extrusion.
  for (i=i0;i<=i1;++i)
    {
      double tt1 = i==i0 ? t0 : -0.5;
      double tt2 = i==i1 ? t1 :  0.5;
      hpgs_point tangent1;
      hpgs_point tangent2;
      hpgs_path_point ep[12];
      int j,n_ep;

      int role = _this->points[i].flags & HPGS_POINT_ROLE_MASK;

      switch (role)
	{
	case HPGS_POINT_LINE:
	  extrude_line(_this,i,tt1,tt2,d,
		       ep,&n_ep,&tangent1);
          n_ep = 2;
	  tangent2 = tangent1;
	  break;

	case HPGS_POINT_BEZIER:
	  extrude_bezier(_this,i,tt1,tt2,d,
			 ep,&n_ep,&tangent1,&tangent2);
	  break;

	default:
	  continue;
	}

      if (i > i0)
	{
	  if (join_lines(path,&_this->points[i].p,&ep[0].p,&prev_tangent,&tangent1,join,ml))
	    return -1;
	}
      else
	{
	  start_point = ep[0].p;
	  start_tangent = tangent1;
	  if (hpgs_paint_path_moveto(path,&ep[0].p)) return -1;
	}

      for (j=1;j<n_ep;)
        if (ep[j].flags == HPGS_POINT_CONTROL)
          {
            if (hpgs_paint_path_curveto(path,&ep[j].p,&ep[j+1].p,&ep[j+2].p))
              return -1;
            j+=3;
          }
        else
          {
            if (hpgs_paint_path_lineto(path,&ep[j].p)) return -1;
            ++j;
          }

      prev_tangent = tangent2;
    }

  // close path.
  if (closed)
    {
      if (join_lines(path,&_this->points[i0].p,&start_point,
		     &prev_tangent,&start_tangent,join,ml))
	return -1;
    
      if (hpgs_paint_path_closepath(path))
	return -1;
    }

  // backward extrusion.
  for (i=i1;i>=i0;--i)
    {
      double tt1 = i==i1 ? t1 : 0.5;
      double tt2 = i==i0 ? t0 : -0.5;
      hpgs_point tangent1;
      hpgs_point tangent2;
      hpgs_path_point ep[12];
      int j,n_ep;
      
      int role = _this->points[i].flags & HPGS_POINT_ROLE_MASK;

      switch (role)
	{
	case HPGS_POINT_LINE:
	  extrude_line(_this,i,tt1,tt2,d,
                       ep,&n_ep,&tangent1);
	  tangent2 = tangent1;
          n_ep=2;
	  break;

	case HPGS_POINT_BEZIER:
	  extrude_bezier(_this,i,tt1,tt2,d,
			 ep,&n_ep,&tangent1,&tangent2);
	  break;

	default:
	  continue;
	}

      if (i < i1)
	{
	  if (join_lines(path,&_this->points[i + ((role== HPGS_POINT_BEZIER) ? 3 : 1)].p,&ep[0].p,&prev_tangent,&tangent1,join,ml))
	    return -1;
	}
      else
	{
	  if (closed)
	    {
	      start_point = ep[0].p;
	      start_tangent = tangent1;
	      if (hpgs_paint_path_moveto(path,&ep[0].p)) return -1;
	    }
	  else
	    {
	      if (cap_line(path,&ep[0].p,cap1))
		return -1;
	    }
	}

      for (j=1;j<n_ep;)
        if (ep[j].flags == HPGS_POINT_CONTROL)
          {
            if (hpgs_paint_path_curveto(path,&ep[j].p,&ep[j+1].p,&ep[j+2].p))
              return -1;
            j+=3;
          }
        else
          {
            if (hpgs_paint_path_lineto(path,&ep[j].p)) return -1;
            ++j;
          }

      prev_tangent = tangent2;
    }

  // close path.
  if (closed)
    {
      if (join_lines(path,&_this->points[i0].p,&start_point,
		     &prev_tangent,&start_tangent,join,ml))
	return -1;
    
      if (hpgs_paint_path_closepath(path))
	return -1;
    }
  else
    {
      if (cap_line(path,&start_point,cap0))
	return -1;
    }

  return hpgs_paint_path_closepath(path);
}

/* Internal:
   Extrude a dot of \this path by the half linewidth
   \c d and append the result to \c path.
  
   The extruded path is appended to \c path and consists of a circle
   or a square depending on the supplied line cap.  
*/
static int extrude_dot (hpgs_paint_path *path,
                        const hpgs_point *p,
                        hpgs_line_cap cap, double d)
{
  switch(cap)
    {
    case hpgs_cap_round:
      {
        hpgs_point ps = { p->x+d, p->y };
        hpgs_point p1 = { p->x, p->y+d };
        hpgs_point p2 = { p->x-d, p->y };
        hpgs_point p3 = { p->x, p->y-d };

        if (hpgs_paint_path_moveto(path,&ps)) return -1;

        if (hpgs_paint_path_buldgeto(path,&p1,0.41421356237309504879)) return -1;
        if (hpgs_paint_path_buldgeto(path,&p2,0.41421356237309504879)) return -1;
        if (hpgs_paint_path_buldgeto(path,&p3,0.41421356237309504879)) return -1;
        if (hpgs_paint_path_buldgeto(path,&ps,0.41421356237309504879)) return -1;
        break;
      }
    case hpgs_cap_square:
      {
        hpgs_point ps = { p->x+d, p->y+d };
        hpgs_point p1 = { p->x+d, p->y-d };
        hpgs_point p2 = { p->x-d, p->y-d };
        hpgs_point p3 = { p->x-d, p->y+d };

        if (hpgs_paint_path_moveto(path,&ps)) return -1;
        if (hpgs_paint_path_lineto(path,&p1)) return -1;
        if (hpgs_paint_path_lineto(path,&p2)) return -1;
        if (hpgs_paint_path_lineto(path,&p3)) return -1;
        if (hpgs_paint_path_closepath(path)) return -1;
        break;
      }
    case hpgs_cap_butt:
      break;
    }

  return 0;
}

/* Internal:
   Extrude a path according to the passed gstate and append the
   extruded parts to \c path.
*/
static int decompose_solid (const hpgs_paint_path *_this,
                            hpgs_paint_path *path,
                            const hpgs_gstate *gstate,
                             
                            int (*segment_processor) (const hpgs_paint_path *_this,
                                                      hpgs_paint_path *path,
                                                      int i0, double t0, hpgs_line_cap cap0,
                                                      int i1, double t1, hpgs_line_cap cap1,
                                                      double d, int closed,
                                                      hpgs_line_join join, double ml),
                            int (*dot_processor) (hpgs_paint_path *path,
                                                  const hpgs_point *p,
                                                  hpgs_line_cap cap, double d)
                            )
{
  int i0=0,i1=-1,i;

  for (i=0;i<_this->n_points;++i)
    {
      int role = _this->points[i].flags & HPGS_POINT_ROLE_MASK;

      if (role == HPGS_POINT_BEZIER || role == HPGS_POINT_LINE)
	i1 = i;

      if (_this->points[i].flags & HPGS_POINT_SUBPATH_START)
        {
          if (role == HPGS_POINT_DOT)
            {
              if (dot_processor(path,&(_this->points[i].p),gstate->line_cap,0.5*gstate->linewidth))
                return -1;

              i1 =-1;
              continue;
            }
          else
            i0 = i1;
        }

      if (i1>=0 && ((_this->points[i].flags & HPGS_POINT_SUBPATH_END) ||
		    i == _this->n_points-1 ) )
	{
	  if (segment_processor(_this,path,
                                i0,-0.5,gstate->line_cap,
                                i1,0.5,gstate->line_cap,
                                0.5*gstate->linewidth,
                                _this->points[i].flags & HPGS_POINT_SUBPATH_CLOSE,
                                gstate->line_join,
                                gstate->miterlimit))
	    return -1;
	  i1 = -1;
	}
    }
  return 0;
}

/* Internal:
   Extrude a dashed path according to the passed gstate and append the
   extruded parts to \c path.
*/
static int decompose_dashed (const hpgs_paint_path *_this,
                             hpgs_paint_path *path,
                             const hpgs_gstate *gstate,
                             
                             int (*segment_processor) (const hpgs_paint_path *_this,
                                                       hpgs_paint_path *path,
                                                       int i0, double t0, hpgs_line_cap cap0,
                                                       int i1, double t1, hpgs_line_cap cap1,
                                                       double d, int closed,
                                                       hpgs_line_join join, double ml),
                             int (*dot_processor) (hpgs_paint_path *path,
                                                   const hpgs_point *p,
                                                   hpgs_line_cap cap, double d)
                             )
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

  for (i=0;i<_this->n_points;++i)
    {
      int role = _this->points[i].flags & HPGS_POINT_ROLE_MASK;
      hpgs_line_cap cap0 = hpgs_cap_butt;
      int end_line = 0;

      if (role == HPGS_POINT_BEZIER || role == HPGS_POINT_LINE)
	i1 = i;

      if (_this->points[i].flags & HPGS_POINT_SUBPATH_START)
	{
          if (role == HPGS_POINT_DOT)
            {
              if (dot_processor(path,&(_this->points[i].p),gstate->line_cap,0.5*gstate->linewidth))
                return -1;

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
              cap0 = gstate->line_cap;
            }
	}

      switch (role)
	{
	case HPGS_POINT_LINE:
	  lseg = hypot(_this->points[i+1].p.x - _this->points[i].p.x,
		       _this->points[i+1].p.y - _this->points[i].p.y );

	  end_line =
	    (_this->points[i+1].flags & HPGS_POINT_SUBPATH_END) ||
	    i >= _this->n_points-2;

	  break;
	case HPGS_POINT_BEZIER:
          hpgs_bezier_length_init(&bl,_this,i);
	  
	  lseg = bl.l[HPGS_BEZIER_NSEGS];

	  end_line =
	    (_this->points[i+3].flags & HPGS_POINT_SUBPATH_END) ||
	    i >= _this->n_points-4;
	  break;

	default:
	  lseg = 0.0;
	}

      if  (role == HPGS_POINT_BEZIER || role == HPGS_POINT_LINE)
	{
	  while (l+lseg > next_l)
	    {
	      if (role == HPGS_POINT_BEZIER)
		t1 =  hpgs_bezier_length_param(&bl,
					       next_l - l);
	      else
		t1 = (next_l - l) / lseg - 0.5;

	      idash = (idash+1) % gstate->n_dashes;
	      next_l += gstate->dash_lengths[idash];

	      if (out_state)
		{
		  hpgs_line_cap cap1 = hpgs_cap_butt;

		  // does the current subpath end with an empty segment ?
		  // if yes, do a nice cap here.
		  if (end_line && l+lseg <= next_l)
		    cap1 = gstate->line_cap;

		  if (segment_processor(_this,path,
                                        i0,t0,cap0,
                                        i1,t1,cap1,
                                        0.5*gstate->linewidth,0,
                                        gstate->line_join,
                                        gstate->miterlimit))
		    return -1;
		}

	      i0 = i1;
	      t0 = t1;
	      cap0 = hpgs_cap_butt;

	      out_state = (idash+1) & 1; 
	    }

	  l+=lseg;
	}

      if (end_line && out_state && (i0<i1 || t0 < 0.5) )
	{
	  if (segment_processor(_this,path,
                                i0,t0,cap0,
                                i1,0.5,gstate->line_cap,
                                0.5*gstate->linewidth,0,
                                gstate->line_join,
                                gstate->miterlimit))
	    return -1;
	}
    }
  return 0;
}

/*!
   Returns a new path on the heap, which represents the extruded border
   line of \c _this. The setting of \c gstate are applied accordingly.

   Use \c hpgs_paint_path_destroy in order to destroy the returned
   \c hpgs_paint_path from the heap.

   Returns a null pointer, if the system is out of memory.   
*/
hpgs_paint_path *hpgs_paint_path_stroke_path(const hpgs_paint_path *_this,
					     const hpgs_gstate *gstate)
{
  hpgs_paint_path *path = hpgs_new_paint_path();

  if (!path) return 0;

#ifdef HPGS_STROKE_DEBUG
  hpgs_log("************** strokepath start.\n");
#endif

  if (gstate->n_dashes <= 0)
    {
      if (decompose_solid (_this,path,gstate,extrude_segment,extrude_dot))
	{ hpgs_paint_path_destroy(path); path = 0; }
    }
  else
    {
      if (decompose_dashed (_this,path,gstate,extrude_segment,extrude_dot))
	{ hpgs_paint_path_destroy(path); path = 0; }
    }

#ifdef HPGS_STROKE_DEBUG
  hpgs_log("************** strokepath end.\n");
#endif

  return path;
}

/* Internal:
   Append a subpath (dash segment) of _this to \c path.
  
   The subpath starts at the path segment starting at point \c i0 at the
   curve parameter \c t0 (t=-0.5 start of segment, t=0.5 end of segment).

   The subpath ends at the path segment starting at point \c i1 at the
   curve parameter \c t1.

   The extruded path is appended to \c path. The linecaps for the start
   and end of the subpath are passed and the line join.  

   if \c closed is non-zero, no line cap is applied and the extruded
   path consists of two closed polygons, one interior and one exterior
   border.
*/
static int append_segment (const hpgs_paint_path *_this,
                           hpgs_paint_path *path,
                           int i0, double t0, hpgs_line_cap cap0,
                           int i1, double t1, hpgs_line_cap cap1,
                           double d, int closed,
                           hpgs_line_join join, double ml)
{
  int i;

  // get segment and push the result.
  for (i=i0;i<=i1;++i)
    {
      double tt1 = i==i0 ? t0 : -0.5;
      double tt2 = i==i1 ? t1 :  0.5;
      hpgs_point ep[4];
      int n_ep;

      int role = _this->points[i].flags & HPGS_POINT_ROLE_MASK;

      switch (role)
	{
	case HPGS_POINT_LINE:
          ep[0].x =
            (0.5-tt1) * _this->points[i].p.x +
            (0.5+tt1) * _this->points[i+1].p.x;

          ep[0].y =
            (0.5-tt1) * _this->points[i].p.y +
            (0.5+tt1) * _this->points[i+1].p.y;

          ep[1].x =
            (0.5-tt2) * _this->points[i].p.x +
            (0.5+tt2) * _this->points[i+1].p.x;
          
          ep[1].y =
            (0.5-tt2) * _this->points[i].p.y +
            (0.5+tt2) * _this->points[i+1].p.y;

          n_ep = 2;
	  break;

	case HPGS_POINT_BEZIER:
          hpgs_bezier_path_point(_this,i,tt1,&ep[0]);

          hpgs_bezier_path_delta(_this,i,tt1,&ep[1]);
          ep[1].x = ep[0].x + (tt2-tt1) * ep[1].x;
          ep[1].y = ep[0].y + (tt2-tt1) * ep[1].y;

          hpgs_bezier_path_point(_this,i,tt2,&ep[3]);

          hpgs_bezier_path_delta(_this,i,tt2,&ep[2]);
          ep[2].x = ep[3].x - (tt2-tt1) * ep[2].x;
          ep[2].y = ep[3].y - (tt2-tt1) * ep[2].y;

          n_ep = 4;
	  break;

	default:
	  continue;
	}

      if (i == i0)
	{
	  if (hpgs_paint_path_moveto(path,&ep[0])) return -1;
	}

      if (n_ep == 4)
        {
          if (hpgs_paint_path_curveto(path,&ep[1],&ep[2],&ep[3]))
            return -1;
        }
      else
        {
          if (hpgs_paint_path_lineto(path,&ep[1])) return -1;
        }
    }

  if (closed)
    return hpgs_paint_path_closepath(path);
  else
    return 0;
}

static int append_dot (hpgs_paint_path *path,
                       const hpgs_point *p,
                       hpgs_line_cap cap, double d)
{
  if (hpgs_paint_path_moveto(path,p)) return -1;
  if (hpgs_paint_path_lineto(path,p)) return -1;
  return 0;
}

/*!
   Returns a new path on the heap, which represents a path which conists of
   solid line segments which should be drawn instead of _this, if your output
   device does not support dashed lines.

   Use \c hpgs_paint_path_destroy in order to destroy the returned
   \c hpgs_paint_path from the heap.

   Returns a null pointer, if the system is out of memory.   
*/
hpgs_paint_path *hpgs_paint_path_decompose_dashes(const hpgs_paint_path *_this,
                                                  const hpgs_gstate *gstate)
{
  hpgs_paint_path *path = hpgs_new_paint_path();

  if (!path) return 0;

  if (gstate->n_dashes <= 0)
    {
      if (decompose_solid (_this,path,gstate,append_segment,append_dot))
	{ hpgs_paint_path_destroy(path); path = 0; }
    }
  else
    {
      if (decompose_dashed (_this,path,gstate,append_segment,append_dot))
	{ hpgs_paint_path_destroy(path); path = 0; }
    }
     
  return path;
}

