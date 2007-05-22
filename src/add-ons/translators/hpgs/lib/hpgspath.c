/***********************************************************************
 *                                                                     *
 * $Id: hpgspath.c 352 2006-10-10 20:58:26Z softadm $
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
 * The implementation of the HPGL reader.                              *
 *                                                                     *
 ***********************************************************************/

#include <hpgsreader.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288
#endif

/*
  Do check for an open path.
*/

int hpgs_reader_checkpath(hpgs_reader *reader)
{
  if (!reader->polygon_mode && reader->polygon_open >= 2)
    return hpgs_reader_stroke(reader);

  return 0;
}

static int push_poly_point(hpgs_reader *reader, hpgs_point *p, int flag)
{
	if (flag == 0 && reader->poly_buffer_size>0 &&
	    reader->poly_buffer[reader->poly_buffer_size-1].flag == 0 &&
	    reader->poly_buffer[reader->poly_buffer_size-1].p.x == p->x &&
		  reader->poly_buffer[reader->poly_buffer_size-1].p.y == p->y   )
		return 0;
	
  if (reader->poly_buffer_size >= reader->poly_buffer_alloc_size)
    {
      reader->poly_buffer_alloc_size *= 2;
      reader->poly_buffer = (hpgs_reader_poly_point *)
	realloc(reader->poly_buffer,
		sizeof(hpgs_reader_poly_point)*reader->poly_buffer_alloc_size);

      if (!reader->poly_buffer)
	return hpgs_set_error(hpgs_i18n("out of memory in push_poly_point."));
    }

  reader->poly_buffer[reader->poly_buffer_size].p = *p;
  reader->poly_buffer[reader->poly_buffer_size].flag = flag;
  ++reader->poly_buffer_size;

  return 0;
}

static void add_path_point(hpgs_reader *reader, hpgs_point *p)
{
  if (p->x < reader->min_path_point.x) reader->min_path_point.x = p->x;
  if (p->y < reader->min_path_point.y) reader->min_path_point.y = p->y;
  if (p->x > reader->max_path_point.x) reader->max_path_point.x = p->x;
  if (p->y > reader->max_path_point.y) reader->max_path_point.y = p->y;
}

static int do_polygon(hpgs_reader *reader, hpgs_bool fill)
{
  int ret = 0;

  int i0 = 0;
  
  while (i0 < reader->poly_buffer_size)
    {
      // find endpoint of current loop.
      int i1 = i0;

      while (i1 < reader->poly_buffer_size && reader->poly_buffer[i1].flag != 3)
        ++i1;

      if (i1 > i0+1)
        {
          int i=i0;
          int open = 0;
          hpgs_point *start=0;

          // Do optimize away a polygon like (PostScript notation):
          //  p1 moveto p2 moveto p3 lineto p4 lineto p2 lineto closepath fill
          // Such polygons are easily created if the pushed currentpoint of PM 0
          // is followed by a closed polygon.
          // This optimization avoids some strange graphical effects caused by the
          // line of thickness zero produced by the final closepath.
          if (fill && reader->poly_buffer[i+1].flag==0 && i1 > i+2 &&
              fabs(reader->poly_buffer[i+1].p.x - reader->poly_buffer[i1-1].p.x) < 1.0e-8 &&  	
              fabs(reader->poly_buffer[i+1].p.y - reader->poly_buffer[i1-1].p.y) < 1.0e-8   )
        	   ++i;
 
          while (i<i1)
            {
              switch (reader->poly_buffer[i].flag)
	              {
	              case 0:
	              	// Well, there are HPGL files in the wild, which write polygons with
                	// the pen up all the time and do a fill afterwards.
	              	if (open && fill)
	                	{
	                  	if (open == 1)
	                    	{
	                      	if (hpgs_moveto(reader->device,start))
	                        	return -1;
	    		    						open = 2;
	        							}
	    	
	      							if (hpgs_lineto(reader->device,&reader->poly_buffer[i].p))
	       					 			return -1;
	    							}
									else
										{
											start = &reader->poly_buffer[i].p;
											open = 1;
										}
	    
									if (i)
	    							add_path_point(reader,&reader->poly_buffer[i].p);
	  							else
	    							{
	      							reader->min_path_point = reader->poly_buffer[0].p;
	      							reader->max_path_point = reader->poly_buffer[0].p;
	    							}
	  							break;

								case 1:
							    if (open == 0)
								    return hpgs_set_error(hpgs_i18n("Missing moveto in do_polygon."));
	    	
									if (open == 1 && hpgs_moveto(reader->device,start))
									  return -1;
	        
									if (hpgs_lineto(reader->device,&reader->poly_buffer[i].p))
										return -1;
	    
									add_path_point(reader,&reader->poly_buffer[i].p);
									open = 2;
									break;
							  case 2:
							    if (open == 0)
							      return hpgs_set_error(hpgs_i18n("Missing moveto in do_polygon."));
							    
							    if (open == 1 && hpgs_moveto(reader->device,start))
							      return -1;
							      
							    if (i+2>=reader->poly_buffer_size)
							      return hpgs_set_error(hpgs_i18n("curveto error in do_polygon."));
							      
							    if (hpgs_curveto(reader->device,
							        &reader->poly_buffer[i].p,
											&reader->poly_buffer[i+1].p,
											&reader->poly_buffer[i+2].p))
										return -1;

									add_path_point(reader,&reader->poly_buffer[i].p);
									add_path_point(reader,&reader->poly_buffer[i+1].p);
									add_path_point(reader,&reader->poly_buffer[i+2].p);
									i+=2;
	                open = 2;
                  break;
                default:
                  return hpgs_set_error(hpgs_i18n("internal error in do_polygon."));
                }
              ++i;
            }

          if (open >= 2)
	          {
	            if (i < reader->poly_buffer_size &&
	                hpgs_closepath(reader->device))
	              return -1;
              ret = 1;
	         }
        }
        
      i0=i1+1;
      
    }  
    
  return ret;
}

/*
  Basic path operations.
*/
int hpgs_reader_moveto(hpgs_reader *reader, hpgs_point *p)
{
  reader->current_point = *p;
  reader->cr_point = *p;

  if (!reader->polygon_open)
    {
      reader->first_path_point = reader->current_point;
      reader->min_path_point = reader->current_point;
      reader->max_path_point = reader->current_point;
    }

  if (reader->polygon_mode)
    {
      if (push_poly_point(reader,&reader->current_point,0))
        return -1;

      if (reader->polygon_open == 0)
         reader->polygon_open = 1;
      else if (reader->polygon_open == 1)
         reader->polygon_open = 2;
    }

  return 0;
}

int hpgs_reader_lineto(hpgs_reader *reader, hpgs_point *p)
{
  if (!reader->polygon_open)
    {
      if (reader->polygon_mode)
	{
	  if (push_poly_point(reader,&reader->current_point,0))
	    return -1;
	}
      else
	{
	  if (hpgs_moveto(reader->device,&reader->current_point))
	    return -1;
	}

      reader->first_path_point = reader->current_point;

      reader->min_path_point = reader->current_point;
      reader->max_path_point = reader->current_point;
    }

  if (reader->polygon_mode)
    {
      if (push_poly_point(reader,p,1))
	return -1;
    }
  else
    {
      if (hpgs_lineto(reader->device,p))
	return -1;
      add_path_point(reader,p);
    }

  reader->polygon_open = 2;
  reader->current_point = *p;
  reader->cr_point = *p;

  return 0;
}

int hpgs_reader_curveto(hpgs_reader *reader,
			hpgs_point *p1, hpgs_point *p2, hpgs_point *p3)
{
  if (!reader->polygon_open)
    {
      if (reader->polygon_mode)
	{
	  if (push_poly_point(reader,&reader->current_point,0))
	    return -1;
	}
      else
	{
	  if (hpgs_moveto(reader->device,&reader->current_point))
	    return -1;
	}

      reader->first_path_point = reader->current_point;
      reader->min_path_point = reader->current_point;
      reader->max_path_point = reader->current_point;
    }

  if (reader->polygon_mode)
    {
      if (push_poly_point(reader,p1,2) ||
	  push_poly_point(reader,p2,2) ||
	  push_poly_point(reader,p3,2)   )
	return -1;
    }
  else
    {
      if (hpgs_curveto(reader->device,p1,p2,p3))
	return -1;

      add_path_point(reader,p1);
      add_path_point(reader,p2);
      add_path_point(reader,p3);
    }

  reader->polygon_open = 2;
  reader->current_point = *p3;
  reader->cr_point = *p3;

  return 0;
}

int hpgs_reader_closepath(hpgs_reader *reader)
{
  if (reader->polygon_open < 2)
    {
      reader->polygon_open = 0;
      return 0;
    }
  
  reader->current_point = reader->first_path_point;
  reader->polygon_open = 0;

  if (reader->polygon_mode)
    return push_poly_point(reader,&reader->first_path_point,3);
  else
    return hpgs_closepath(reader->device);
}

int hpgs_reader_stroke(hpgs_reader *reader)
{
  reader->polygon_open = 0;
  reader->have_current_point = 0;
  return hpgs_stroke(reader->device);
}

static int hatch(hpgs_reader *reader, double spacing, double angle, int cross, hpgs_bool winding)
{
  int i;
  double ca = cos (angle*M_PI/180.0);
  double sa = sin (angle*M_PI/180.0);
  hpgs_point h_min,h_max;
  hpgs_point p,ph;

  if (spacing <= 1.0)
    spacing= hypot(reader->P2.x-reader->P1.x,
		   reader->P2.y-reader->P1.y ) * 0.01 * HP_TO_PT;

  // rotate corner points to hatch coordinates and calculate min/max hatches.
  p.x = reader->min_path_point.x-reader->anchor_point.x;
  p.y = reader->min_path_point.y-reader->anchor_point.y;

  h_max.x = h_min.x = (p.x * ca + p.y * sa)/spacing;
  h_max.y = h_min.y = (p.y * ca - p.x * sa)/spacing;

  p.x = reader->min_path_point.x-reader->anchor_point.x;
  p.y = reader->max_path_point.y-reader->anchor_point.y;

  ph.x = (p.x * ca + p.y * sa)/spacing;
  ph.y = (p.y * ca - p.x * sa)/spacing;
  
  if (ph.x < h_min.x) h_min.x = ph.x; 
  if (ph.y < h_min.y) h_min.y = ph.y; 
  if (ph.x > h_max.x) h_max.x = ph.x; 
  if (ph.y > h_max.y) h_max.y = ph.y; 

  p.x = reader->max_path_point.x-reader->anchor_point.x;
  p.y = reader->max_path_point.y-reader->anchor_point.y;

  ph.x = (p.x * ca + p.y * sa)/spacing;
  ph.y = (p.y * ca - p.x * sa)/spacing;
  
  if (ph.x < h_min.x) h_min.x = ph.x; 
  if (ph.y < h_min.y) h_min.y = ph.y; 
  if (ph.x > h_max.x) h_max.x = ph.x; 
  if (ph.y > h_max.y) h_max.y = ph.y; 

  p.x = reader->max_path_point.x-reader->anchor_point.x;
  p.y = reader->min_path_point.y-reader->anchor_point.y;

  ph.x = (p.x * ca + p.y * sa)/spacing;
  ph.y = (p.y * ca - p.x * sa)/spacing;

  if (ph.x < h_min.x) h_min.x = ph.x; 
  if (ph.y < h_min.y) h_min.y = ph.y; 
  if (ph.x > h_max.x) h_max.x = ph.x; 
  if (ph.y > h_max.y) h_max.y = ph.y; 

  if (hpgs_clipsave(reader->device)) return -1;
  
  if (hpgs_clip(reader->device,winding)) return -1;

  if (hpgs_newpath(reader->device)) return -1;

  // go through verticaltal hatches hatches.
  for (i = (int)ceil(h_min.y); i <= (int)floor(h_max.y); ++i)
    {
      ph.y = i;
      ph.x = h_min.x;

      p.x = (ph.x * ca - ph.y * sa) * spacing + reader->anchor_point.x;
      p.y = (ph.y * ca + ph.x * sa) * spacing + reader->anchor_point.y;

      if (hpgs_moveto(reader->device,&p)) return -1;
      
      ph.x = h_max.x;
    
      p.x = (ph.x * ca - ph.y * sa) * spacing + reader->anchor_point.x;
      p.y = (ph.y * ca + ph.x * sa) * spacing + reader->anchor_point.y;

      if (hpgs_lineto(reader->device,&p)) return -1;
      if (hpgs_stroke(reader->device)) return -1;
    }

  if (cross)
    // go through horizontal hatches hatches.
    for (i = (int)ceil(h_min.x); i <= (int)floor(h_max.x); ++i)
      {
	ph.x = i;
	ph.y = h_min.y;

	p.x = (ph.x * ca - ph.y * sa) * spacing + reader->anchor_point.x;
	p.y = (ph.y * ca + ph.x * sa) * spacing + reader->anchor_point.y;

	if (hpgs_moveto(reader->device,&p)) return -1;
	
	ph.y = h_max.y;
    
	p.x = (ph.x * ca - ph.y * sa) * spacing + reader->anchor_point.x;
	p.y = (ph.y * ca + ph.x * sa) * spacing + reader->anchor_point.y;

	if (hpgs_lineto(reader->device,&p)) return -1;
	if (hpgs_stroke(reader->device)) return -1;
      }

  if (hpgs_cliprestore(reader->device))
    return -1;

  return hpgs_newpath(reader->device);
}

// filltype 10.
static int shade(hpgs_reader *reader, double level, hpgs_bool winding)
{
  int pen = reader->current_pen;
  double alpha = level * 0.01;

  if (alpha < 0.0) alpha = 0.0;
  if (alpha > 1.0) alpha = 1.0;

  if  (alpha != 1.0)
    {
      hpgs_color rgb;

      rgb.r = (1.0-alpha) + reader->pen_colors[pen].r*alpha;
      rgb.g = (1.0-alpha) + reader->pen_colors[pen].g*alpha;
      rgb.b = (1.0-alpha) + reader->pen_colors[pen].b*alpha;

      if (hpgs_setrgbcolor(reader->device,&rgb))
        return -1;
    }

  
  if (hpgs_fill(reader->device,winding)) return -1;

  if (alpha == 1.0) return 0;

  return hpgs_setrgbcolor(reader->device,
			  &reader->pen_colors[pen]);
}

int hpgs_reader_fill(hpgs_reader *reader, hpgs_bool winding)
{
  reader->polygon_open = 0;
  reader->have_current_point = 0;

  switch (reader->current_ft)
    {
    case 3:
      return hatch(reader,reader->ft3_spacing,reader->ft3_angle,0,winding);
    case 4:
      return hatch(reader,reader->ft4_spacing,reader->ft4_angle,1,winding);
    case 10:
      return shade(reader,reader->ft10_level,winding);
    default:
      return hpgs_fill(reader->device,winding);
    }
  return 0;
}

/*
 HPGL Command AC (AnChor Point)
*/
int hpgs_reader_do_AC (hpgs_reader *reader)
{
  if (reader->eoc)
    {
      reader->anchor_point.x = 0.0;
      reader->anchor_point.y = 0.0;
      return 0;
    }
  else
    return hpgs_reader_read_point(reader,&reader->anchor_point,1);
}

static double sqr(double x);
__inline__ double sqr(double x) { return x*x; }

/*
  Transform an arc from the current point through p2 to p3 to
  center/sweep.

  Return value: 0 ... arc successfully constructed.
                1 ... points lie on a straight line.
*/

static int arc_by_points(hpgs_reader *reader,
			 hpgs_point *p2,
			 hpgs_point *p3,
			 hpgs_point *center,
			 double *r,
			 double *sweep )
{
  hpgs_point * p1 = &reader->current_point;

  double dx12 = p1->x-p2->x;
  double dy12 = p1->y-p2->y;
  double dx23 = p2->x-p3->x;
  double dy23 = p2->y-p3->y;
  double dx13 = p1->x-p3->x;
  double dy13 = p1->y-p3->y;

  double det=dy12*dx23-dy23*dx12;
  double angle1,angle2;

  // don't ask for scaling, since graphical coordinates are
  // reasonably scaled in most situations.
  if (fabs(det)<1.0e-8)
    {
      // circle detection
      // end points coincide
      if (hypot(dx13,dy13)<1.0e-8)
	{
	  center->x=0.5*(p1->x+p2->x);
	  center->y=0.5*(p1->y+p2->y);
	  *sweep=360.0;
	  *r=0.5*hypot(dx12,dy12);
	  return 0;
	}

      // other points coincide or points lie on a straight line
      center->x=0.5*(p1->x+p3->x);
      center->y=0.5*(p1->y+p3->y);
      *sweep=360.0;
      *r=0.5*hypot(dx13,dy13);
      return 1;
    }

  center->x=dy13*dy12*dy23
    +(sqr(p1->x)-sqr(p2->x))*dy23
    -(sqr(p2->x)-sqr(p3->x))*dy12;

  center->y=dx13*dx12*dx23
    +(sqr(p1->y)-sqr(p2->y))*dx23
    -(sqr(p2->y)-sqr(p3->y))*dx12;

  center->x /= -2.0 * det;
  center->y /=  2.0 * det;

  *r=hypot(p1->x-center->x,p1->y-center->y);

  angle1 = atan2(p1->y-center->y,p1->x-center->x)*180.0/M_PI;
  angle2 = atan2(p2->y-center->y,p2->x-center->x)*180.0/M_PI;

  // det telsl us, whether the assertion
  // angle1<angle2 or angle2<angle1 holds.
  if (det>0)
    {
      // This is necessary, because atan2 breaks at M_PI.
      if (angle2 > angle1) angle2-=360.0;
    }
  else
    {
      // This is necessary, because atan2 breaks at M_PI.
      if (angle2 < angle1) angle2+=360.0;
    }

  *sweep = angle2-angle1;

  return 0;
}


// draw bezier spline parts for an elliptical arc.
static int arc_to_bezier  (hpgs_reader *reader,
			   hpgs_point *center,
			   double sweep)
{
  int nseg = (int)ceil(fabs(sweep)/90.0);
  double a,seg_alpha_2,beta;
  int i;
  hpgs_point *p0,axis;

  if (!nseg) return 0;

  axis.x = reader->current_point.x - center->x;
  axis.y = reader->current_point.y - center->y;

  seg_alpha_2 = 0.5*M_PI/180.0*sweep/nseg;
  beta = 4.0/3.0 * (1.0-cos(seg_alpha_2))/sin(seg_alpha_2);

  a=0.0;
  
  p0 = &reader->current_point;

  for (i=1;i<=nseg;i++)
    {
      hpgs_point p1=*p0;
      hpgs_point p2;
      hpgs_point p3=*center;
      
      p1.x -= sin(a)*beta*axis.x;
      p1.y -= sin(a)*beta*axis.y;
 
      p1.x -= cos(a)*beta*axis.y;
      p1.y += cos(a)*beta*axis.x;
      
      a=M_PI/180.0 * i*sweep/nseg;

      p3.x += cos(a)*axis.x;
      p3.y += cos(a)*axis.y;

      p3.x -= sin(a)*axis.y;
      p3.y += sin(a)*axis.x;

      p2=p3;
      
      p2.x += sin(a)*beta*axis.x;
      p2.y += sin(a)*beta*axis.y;

      p2.x += cos(a)*beta*axis.y;
      p2.y -= cos(a)*beta*axis.x;

      if (hpgs_reader_curveto(reader,&p1,&p2,&p3))
	return -1;

      *p0=p3;
    }

  return 0;
}

/*
 HPGL Command AA (Arc Absolute)
*/
int hpgs_reader_do_AA (hpgs_reader *reader)
{
  hpgs_point center;
  double sweep;
  double chord=0.0;

  if (hpgs_reader_read_point(reader,&center,1)) return -1;
  if (hpgs_reader_read_double(reader,&sweep)) return -1;
  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&chord)) return -1;

  // Be careful about the direction of the rotation for
  // transformation matrices with neg. determinant.
  if (reader->world_matrix.mxx * reader->world_matrix.myy -
      reader->world_matrix.mxy * reader->world_matrix.myx   < 0.0)
    sweep=-sweep;

  return  arc_to_bezier(reader,&center,sweep);
}

/*
 HPGL Command AR (Arc Relative)
*/
int hpgs_reader_do_AR (hpgs_reader *reader)
{
  hpgs_point center;
  double sweep;
  double chord=0.0;

  if (hpgs_reader_read_point(reader,&center,-1)) return -1;
  if (hpgs_reader_read_double(reader,&sweep)) return -1;
  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&chord)) return -1;

  center.x += reader->current_point.x;
  center.y += reader->current_point.y;

  // Be careful about the direction of the rotation for
  // transformation matrices with neg. determinant.
  if (reader->world_matrix.mxx * reader->world_matrix.myy -
      reader->world_matrix.mxy * reader->world_matrix.myx   < 0.0)
    sweep=-sweep;

  return  arc_to_bezier(reader,&center,sweep);
}

/*
 HPGL Command AT (Absolute Arc Three Points)
*/
int hpgs_reader_do_AT (hpgs_reader *reader)
{
  hpgs_point p2,p3,center;
  double r,sweep,chord=0.0;

  if (hpgs_reader_read_point(reader,&p2,1)) return -1;
  if (hpgs_reader_read_point(reader,&p3,1)) return -1;
  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&chord)) return -1;

  reader->polygon_open = 1;

  if (arc_by_points(reader,&p2,&p3,&center,&r,&sweep))
    return hpgs_reader_lineto(reader,&p3);
  else
    return  arc_to_bezier(reader,&center,sweep);
}

/*
 HPGL Command RT (Relative arc Three points)
*/
int hpgs_reader_do_RT (hpgs_reader *reader)
{
  hpgs_point p2,p3,center;
  double r,sweep,chord=0.0;

  if (hpgs_reader_read_point(reader,&p2,-1)) return -1;
  if (hpgs_reader_read_point(reader,&p3,-1)) return -1;

  p2.x += reader->current_point.x;
  p2.y += reader->current_point.y;
  p3.x += reader->current_point.x;
  p3.y += reader->current_point.y;

  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&chord)) return -1;

  if (arc_by_points(reader,&p2,&p3,&center,&r,&sweep))
    return hpgs_reader_lineto(reader,&p3);
  else
    return  arc_to_bezier(reader,&center,sweep);
}

/*
 HPGL Command CI (CIrcle)
*/
int hpgs_reader_do_CI (hpgs_reader *reader)
{
  double r;
  double chord=0.0;
  hpgs_point p0,center = reader->current_point;

  // read input point
  if (hpgs_reader_read_double(reader,&r)) return -1;
  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&chord)) return -1;

  if (reader->polygon_mode)
    {
      if (hpgs_reader_closepath(reader)) return -1;
    }

  // scale r with the sqrt down to the paper space.
  r *= reader->total_scale;

  p0.x = reader->current_point.x + r;
  p0.y = reader->current_point.y;
  reader->pen_down = 1;

  if (hpgs_reader_moveto(reader,&p0))
    return -1;

  if (arc_to_bezier(reader,&center,360.0)) return -1;

  if (reader->polygon_mode)
    {
      if (hpgs_reader_closepath(reader)) return -1;
    }
  
  return hpgs_reader_moveto(reader,&center);
}

/*
 HPGL Command BZ (BeZier absolute)
*/
int hpgs_reader_do_BZ (hpgs_reader *reader)
{
  hpgs_point p1;
  hpgs_point p2;
  hpgs_point p3;

  while (!reader->eoc)
    {
      if (hpgs_reader_read_point(reader,&p1,1)) return -1;
      if (hpgs_reader_read_point(reader,&p2,1)) return -1;
      if (hpgs_reader_read_point(reader,&p3,1)) return -1;

      if (hpgs_reader_curveto(reader,&p1,&p2,&p3)) return -1;
   }

  return 0;
}

/*
 HPGL Command BR (Bezier Relative)
*/
int hpgs_reader_do_BR (hpgs_reader *reader)
{
  hpgs_point p1;
  hpgs_point p2;
  hpgs_point p3;

  while (!reader->eoc)
    {
      if (hpgs_reader_read_point(reader,&p1,-1)) return -1;
      if (hpgs_reader_read_point(reader,&p2,-1)) return -1;
      if (hpgs_reader_read_point(reader,&p3,-1)) return -1;

      p1.x += reader->current_point.x;
      p1.y += reader->current_point.y;
      p2.x += reader->current_point.x;
      p2.y += reader->current_point.y;
      p3.x += reader->current_point.x;
      p3.y += reader->current_point.y;

      if (hpgs_reader_curveto(reader,&p1,&p2,&p3)) return -1;
   }

  return 0;
}

/*
 HPGL Command PA (Plot Absolute)
*/
int hpgs_reader_do_PA (hpgs_reader *reader)
{
  hpgs_point p;

  if (!reader->pen_down &&
      hpgs_reader_checkpath(reader)) return -1;

  while (!reader->eoc)
    {
      if (hpgs_reader_read_point(reader,&p,1)) return -1;

      if (reader->pen_down)
	{
	  if (hpgs_reader_lineto(reader,&p)) return -1;
	}
      else
	{
	  if (hpgs_reader_moveto(reader,&p)) return -1;
	}
    }

  reader->absolute_plotting = 1;

  return 0;
}

/*
 HPGL Command PD (Pen Down)
*/
int hpgs_reader_do_PD (hpgs_reader *reader)
{
  hpgs_point p;

  while (!reader->eoc)
    {
      if (hpgs_reader_read_point(reader,&p,
				 reader->absolute_plotting ? 1 : -1)) return -1;

      if (!reader->absolute_plotting)
	{
	  p.x += reader->current_point.x;
	  p.y += reader->current_point.y;
	}

      if (hpgs_reader_lineto(reader,&p)) return -1;
   }

  reader->pen_down = 1;

  return 0;
}

/*
 HPGL Command PR (Plot Relative)
*/
int hpgs_reader_do_PR (hpgs_reader *reader)
{
  hpgs_point p;

  if (!reader->pen_down &&
      hpgs_reader_checkpath(reader)) return -1;

  while (!reader->eoc)
    {
      if (hpgs_reader_read_point(reader,&p,-1)) return -1;

      p.x += reader->current_point.x;
      p.y += reader->current_point.y;

      if (reader->pen_down)
	{
	  if (hpgs_reader_lineto(reader,&p)) return -1;
	}
      else
	{
	  if (hpgs_reader_moveto(reader,&p)) return -1;
	}
    }

  reader->absolute_plotting = 0;

  return 0;
}

/*
 HPGL Command PU (Pen Up)
*/
int hpgs_reader_do_PU (hpgs_reader *reader)
{
  hpgs_point p;

  if (hpgs_reader_checkpath(reader)) return -1;

  while (!reader->eoc)
    {
      if (hpgs_reader_read_point(reader,&p,
				 reader->absolute_plotting ? 1 : -1)) return -1;

      if (!reader->absolute_plotting)
	{
	  p.x += reader->current_point.x;
	  p.y += reader->current_point.y;
	}

      if (hpgs_reader_moveto(reader,&p)) return -1;
   }

  reader->pen_down = 0;

  return 0;
}

/*
 HPGL Command PM (Polygon Mode)
*/
int hpgs_reader_do_PM (hpgs_reader *reader)
{
  int mode=0;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&mode)) return -1;

  switch(mode)
    {
    case 0:
      if (hpgs_reader_checkpath(reader)) return -1;
      reader->poly_buffer_size=0;
      reader->polygon_mode=1;
      reader->polygon_open=0;
      // push the current point to the polygon buffer.
      if (hpgs_reader_moveto(reader,&reader->current_point)) return -1;
      break;
    case 1:
      if (hpgs_reader_closepath(reader)) return -1;
      break;
    case 2:
      if (hpgs_reader_closepath(reader)) return -1;
      reader->polygon_mode=0;
      if (reader->poly_buffer_size > 0)
        reader->current_point = reader->poly_buffer[0].p;
      break;
    default:
      return hpgs_set_error(hpgs_i18n("PM: Illegal mode %d."),mode);
    };

  return 0;
}

/*
 HPGL Command FP (Fill Polygon)
*/
int hpgs_reader_do_FP (hpgs_reader *reader)
{
  hpgs_bool winding = HPGS_FALSE;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&winding)) return -1;

  if (hpgs_reader_checkpath(reader)) return -1;

  if (reader->poly_buffer_size <= 0) return 0;

  switch (do_polygon(reader,HPGS_TRUE))
    {
    case 0:
      return 0;
    case 1:
      return hpgs_reader_fill(reader,winding);
    default:
      return -1;
    }
}

/*
 HPGL Command PP (Pixel Placement)
*/
int hpgs_reader_do_PP (hpgs_reader *reader)
{
  int dummy = 0;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&dummy)) return -1;

  if (reader->verbosity > 1)
    hpgs_log("PP %d: unimplemented.\n",dummy);

  return 0;
}

/*
 HPGL Command FT (Fill Type)
*/
int hpgs_reader_do_FT (hpgs_reader *reader)
{
  double option;

  if (reader->eoc)
    {
      reader->current_ft = 1;
      return 0;
    }

  if (hpgs_reader_read_int(reader,&reader->current_ft)) return -1;

  if (!reader->eoc)
    {
      if (hpgs_reader_read_double(reader,&option)) return -1;

      switch (reader->current_ft)
	{
	case 3:
	  // spacing is measured in current units along the x axis.
	  reader->ft3_spacing = option * hypot(reader->total_matrix.mxx,reader->total_matrix.myx);
	  break;
	case 4:
	  // spacing is measured in current units along the x axis.
	  reader->ft4_spacing = option * hypot(reader->total_matrix.mxx,reader->total_matrix.myx);
	  break;
	case 10:
	  reader->ft10_level = option;
	}
    }

  if (!reader->eoc)
    {
      if (hpgs_reader_read_double(reader,&option)) return -1;

      switch (reader->current_ft)
	{
	case 3:
	  reader->ft3_angle = option;
	  break;
	case 4:
	  reader->ft4_angle = option;
	}
    }

  return 0;
}

/*
 HPGL Command EA (Edge rectangle Absolute)
*/
int hpgs_reader_do_EA (hpgs_reader *reader)
{
  hpgs_point p,pp,cp;

  if (hpgs_reader_read_point(reader,&p,1)) return -1;

  if (hpgs_reader_checkpath(reader)) return -1;
  reader->poly_buffer_size = 0;
  reader->polygon_mode = 1;

  cp = reader->current_point;

  if (hpgs_reader_moveto(reader,&cp)) return -1;

  pp.x = cp.x;
  pp.y = p.y;

  if (hpgs_reader_lineto(reader,&pp)) return -1;
  if (hpgs_reader_lineto(reader,&p)) return -1;

  pp.x = p.x;
  pp.y = cp.y;

  if (hpgs_reader_lineto(reader,&pp)) return -1;
  if (hpgs_reader_closepath(reader)) return -1;

  switch (do_polygon(reader,HPGS_FALSE))
    {
    case 1:
      if (hpgs_reader_fill(reader,HPGS_TRUE))
        return -1;

      if (hpgs_reader_stroke(reader))
        return -1;
        
    case 0:
      reader->polygon_mode = 0;
      return 0;
        
    default:
      return -1;
    }
}

/*
 HPGL Command RA (fill Rectangle Absolute)
*/
int hpgs_reader_do_RA (hpgs_reader *reader)
{
  hpgs_point p,pp,cp;

  if (hpgs_reader_read_point(reader,&p,1)) return -1;

  if (hpgs_reader_checkpath(reader)) return -1;
  reader->poly_buffer_size = 0;
  reader->polygon_mode = 1;

  cp = reader->current_point;

  if (hpgs_reader_moveto(reader,&cp)) return -1;

  pp.x = cp.x;
  pp.y = p.y;

  if (hpgs_reader_lineto(reader,&pp)) return -1;
  if (hpgs_reader_lineto(reader,&p)) return -1;

  pp.x = p.x;
  pp.y = cp.y;

  if (hpgs_reader_lineto(reader,&pp)) return -1;
  if (hpgs_reader_closepath(reader)) return -1;

  switch (do_polygon(reader,HPGS_TRUE))
    {
    case 1:
      if (hpgs_reader_fill(reader,1))
        return -1;
        
    case 0:
      reader->polygon_mode = 0;
      return 0;
        
    default:
      return -1;
    }
}

/*
 HPGL Command ER (Edge rectangle Relative)
*/
int hpgs_reader_do_ER (hpgs_reader *reader)
{
  hpgs_point p,pp,cp;

  if (hpgs_reader_read_point(reader,&p,-1)) return -1;

  p.x += reader->current_point.x;
  p.y += reader->current_point.y;

  if (hpgs_reader_checkpath(reader)) return -1;
  reader->poly_buffer_size = 0;
  reader->polygon_mode = 1;

  cp = reader->current_point;

  if (hpgs_reader_moveto(reader,&cp)) return -1;

  pp.x = cp.x;
  pp.y = p.y;

  if (hpgs_reader_lineto(reader,&pp)) return -1;
  if (hpgs_reader_lineto(reader,&p)) return -1;

  pp.x = p.x;
  pp.y = cp.y;

  if (hpgs_reader_lineto(reader,&pp)) return -1;
  if (hpgs_reader_closepath(reader)) return -1;

  switch (do_polygon(reader,HPGS_FALSE))
    {
    case 1:
      if (hpgs_reader_stroke(reader))
        return -1;
        
    case 0:
      reader->polygon_mode = 0;
      return 0;
        
    default:
      return -1;
    }
}

/*
 HPGL Command RR (fill Rectangle Relative)
*/
int hpgs_reader_do_RR (hpgs_reader *reader)
{
  hpgs_point p,pp,cp;

  if (hpgs_reader_read_point(reader,&p,-1)) return -1;

  p.x += reader->current_point.x;
  p.y += reader->current_point.y;

  if (hpgs_reader_checkpath(reader)) return -1;
  reader->poly_buffer_size = 0;
  reader->polygon_mode = 1;

  cp = reader->current_point;

  if (hpgs_reader_moveto(reader,&cp)) return -1;

  pp.x = cp.x;
  pp.y = p.y;

  if (hpgs_reader_lineto(reader,&pp)) return -1;
  if (hpgs_reader_lineto(reader,&p)) return -1;

  pp.x = p.x;
  pp.y = cp.y;

  if (hpgs_reader_lineto(reader,&pp)) return -1;
  if (hpgs_reader_closepath(reader)) return -1;

  switch (do_polygon(reader,HPGS_TRUE))
    {
    case 1:
      if (hpgs_reader_fill(reader,1))
        return -1;
        
    case 0:
      reader->polygon_mode = 0;
      return 0;
        
    default:
      return -1;
    }
}

/*
 HPGL Command EP (Edge Polygon)
*/
int hpgs_reader_do_EP (hpgs_reader *reader)
{
  if (hpgs_reader_checkpath(reader)) return -1;

  if (reader->poly_buffer_size <= 0) return 0;

  switch (do_polygon(reader,HPGS_FALSE))
    {
    case 1:
      if (hpgs_stroke(reader->device))
        return -1;

    case 0:
      return 0;
        
    default:
      return -1;
    }
}

/*
 HPGL Command EW (Edge Wedge)
*/
int hpgs_reader_do_EW (hpgs_reader *reader)
{
  double r,start,sweep,chord;
  hpgs_point p,center;

  if (hpgs_reader_read_double(reader,&r)) return -1;
  if (hpgs_reader_read_double(reader,&start)) return -1;
  if (hpgs_reader_read_double(reader,&sweep)) return -1;
  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&chord)) return -1;

  if (hpgs_reader_checkpath(reader)) return -1;
  reader->poly_buffer_size = 0;
  reader->polygon_mode = 1;

  // scale r with the sqrt down to the paper space.
  r *= reader->total_scale;

  center = reader->current_point;
  p=reader->current_point;
  p.x+=cos(start*M_PI/180.0)*r;
  p.y+=sin(start*M_PI/180.0)*r;

  if (hpgs_reader_moveto(reader,&reader->current_point)) return -1;
  if (hpgs_reader_lineto(reader,&p)) return -1;

  if (arc_to_bezier(reader,&center,sweep))
    
  if (hpgs_reader_closepath(reader)) return -1;

  switch (do_polygon(reader,HPGS_FALSE))
    {
    case 1:
      if (hpgs_reader_stroke(reader))
        return -1;
        
    case 0:
      reader->polygon_mode = 0;
      return 0;
        
    default:
      return -1;
    }
}

/*
 HPGL Command WG (fill WedGe)
*/
int hpgs_reader_do_WG (hpgs_reader *reader)
{
  double r,start,sweep,chord;
  hpgs_point p,center;

  if (hpgs_reader_read_double(reader,&r)) return -1;
  if (hpgs_reader_read_double(reader,&start)) return -1;
  if (hpgs_reader_read_double(reader,&sweep)) return -1;
  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&chord)) return -1;

  if (hpgs_reader_checkpath(reader)) return -1;
  reader->poly_buffer_size = 0;
  reader->polygon_mode = 1;

  // scale r with the sqrt down to the paper space.
  r *= reader->total_scale;

  center = reader->current_point;
  p=reader->current_point;
  p.x+=cos(start*M_PI/180.0)*r;
  p.y+=sin(start*M_PI/180.0)*r;

  if (hpgs_reader_moveto(reader,&reader->current_point)) return -1;
  if (hpgs_reader_lineto(reader,&p)) return -1;

  if (arc_to_bezier(reader,&center,sweep))
    
  if (hpgs_reader_closepath(reader)) return -1;

 switch (do_polygon(reader,HPGS_TRUE))
    {
    case 1:
      if (hpgs_reader_fill(reader,1))
        return -1;
        
    case 0:
      reader->polygon_mode = 0;
      return 0;
        
    default:
      return -1;
    }
}
