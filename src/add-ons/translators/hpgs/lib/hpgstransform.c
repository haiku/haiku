/***********************************************************************
 *                                                                     *
 * $Id: hpgstransform.c 362 2006-10-16 14:13:48Z softadm $
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

//#define HPGS_DEBUG_XFORM

static void apply_scale(hpgs_reader *reader,
                        const hpgs_point *p1, const hpgs_point *p2)
{
  double xf,yf;
  double dx = 0.0,dy=0.0;
  hpgs_point p2u;

  if (reader->sc_type < 0) return;

  // get P2 in old user coordinates
  p2u.x = reader->frame_x + p2->x * HP_TO_PT;
  p2u.y = reader->frame_y + p2->y * HP_TO_PT;

  hpgs_matrix_ixform(&p2u,&p2u,&reader->world_matrix);

#ifdef HPGS_DEBUG_XFORM
    {
      hpgs_point p1u;

      p1u.x = reader->frame_x + p1->x * HP_TO_PT;
      p1u.y = reader->frame_y + p1->y * HP_TO_PT;

      hpgs_matrix_ixform(&p1u,&reader->world_matrix,&p1u);

      hpgs_log("SC: P1 = %g,%g.\n",p1->x,p1->y);
      hpgs_log("SC: P2 = %g,%g.\n",p2->x,p2->y);
      hpgs_log("SC: p1u = %g,%g.\n",p1u.x,p1u.y);
      hpgs_log("SC: p2u = %g,%g.\n",p2u.x,p2u.y);
      hpgs_log("SC: xmin,xmax = %g,%g.\n",reader->sc_xmin,reader->sc_xmax);
      hpgs_log("SC: ymin,ymax = %g,%g.\n",reader->sc_ymin,reader->sc_ymax);
    }
#endif

  switch (reader->sc_type)
    {
    case 0:
      xf = p2u.x / (reader->sc_xmax - reader->sc_xmin);
      yf = p2u.y / (reader->sc_ymax - reader->sc_ymin);
      break;
    case 1:
      xf = p2u.x / (reader->sc_xmax - reader->sc_xmin);
      yf = p2u.y / (reader->sc_ymax - reader->sc_ymin);

      if (xf < yf)
	{
	  dy = (yf-xf) * (reader->sc_ymax - reader->sc_ymin) * 0.01 * reader->sc_left;
	  yf = xf;
	}
      else
	{
	  dx = (xf-yf) * (reader->sc_xmax - reader->sc_xmin) * 0.01 * reader->sc_bottom;
	  xf = yf;
	}
      break;

    case 2:
      xf = reader->sc_xmax;
      yf = reader->sc_ymax;
      break;

    default:
      return;
    }

#ifdef HPGS_DEBUG_XFORM
  hpgs_log("SC: xf,yf = %g,%g.\n",xf,yf);
#endif

  dx -= reader->sc_xmin * xf;
  dy -= reader->sc_ymin * yf;

  // concatenate transofrmation matrices.
  //
  // | 1    0   0 |  | 1   0   0 |   
  // | x0 mxx mxy |* | dx  xf  0 | = 
  // | y0 myx myy |  | dy  0  yf |   
  //
  // | 1                     0      0 |
  // | x0+mxx*dx+mxy*dy mxx*xf mxy*yf |
  // | y0+myx*dx+myy*dy myx*xf myy*yf |
  //

  reader->world_matrix.dx += reader->world_matrix.mxx*dx + reader->world_matrix.mxy*dy;
  reader->world_matrix.dy += reader->world_matrix.myx*dx + reader->world_matrix.myy*dy;

  reader->world_matrix.mxx *= xf;
  reader->world_matrix.myx *= xf;
  reader->world_matrix.mxy *= yf;
  reader->world_matrix.myy *= yf;

#ifdef HPGS_DEBUG_XFORM
  hpgs_log("SC: %10g %10g %10g\n",reader->world_matrix.dx,reader->world_matrix.mxx,reader->world_matrix.mxy);
  hpgs_log("SC: %10g %10g %10g\n",reader->world_matrix.dy,reader->world_matrix.myx,reader->world_matrix.myy);
#endif
}

void hpgs_reader_set_default_transformation (hpgs_reader *reader)
{
  // transformation matrix for user to PS coordinates.
  int angle = reader->y_size >= reader->x_size ? 90 : 0;

  hpgs_point p1 = reader->P1;
  hpgs_point p2 = reader->P2;

  angle += reader->rotation;

  if ((angle % 180) == 90)
    {
      p2.y -= p1.y;
      p1.y = 0.0;
    }


  reader->world_matrix.dx = reader->frame_x + p1.x * HP_TO_PT;
  reader->world_matrix.dy = reader->frame_y + p1.y * HP_TO_PT;

  switch (angle % 360)
    {
    case 90:
      reader->world_matrix.mxx = 0.0;
      reader->world_matrix.mxy = -HP_TO_PT;
      reader->world_matrix.myx = HP_TO_PT;
      reader->world_matrix.myy = 0.0;
      break;
    case 180:
      reader->world_matrix.mxx = -HP_TO_PT;
      reader->world_matrix.mxy = 0.0;
      reader->world_matrix.myx = 0.0;
      reader->world_matrix.myy = -HP_TO_PT;
      break;
    case 270:
      reader->world_matrix.mxx = 0.0;
      reader->world_matrix.mxy = HP_TO_PT;
      reader->world_matrix.myx = -HP_TO_PT;
      reader->world_matrix.myy = 0.0;
      break;
    default: // 0
      reader->world_matrix.mxx = HP_TO_PT;
      reader->world_matrix.mxy = 0.0;
      reader->world_matrix.myx = 0.0;
      reader->world_matrix.myy = HP_TO_PT;
    }

#ifdef HPGS_DEBUG_XFORM
  hpgs_log("xform: %10g %10g %10g\n",reader->world_matrix.dx,reader->world_matrix.mxx,reader->world_matrix.mxy);
  hpgs_log("xform: %10g %10g %10g\n",reader->world_matrix.dy,reader->world_matrix.myx,reader->world_matrix.myy);
#endif

  apply_scale(reader,&p1,&p2);

  reader->world_scale =
    sqrt (fabs(reader->world_matrix.mxx * reader->world_matrix.myy -
               reader->world_matrix.mxy * reader->world_matrix.myx  ) );

  // finally transform from model space to the page.
  hpgs_matrix_concat(&reader->total_matrix,&reader->page_matrix,&reader->world_matrix);

  reader->total_scale = reader->page_scale * reader->world_scale;
}

/* 
  HPGL command PS (Plot Size)
*/
int hpgs_reader_do_PS (hpgs_reader *reader)
{
  double x_size=reader->x_size/HP_TO_PT;
  double y_size=reader->y_size/HP_TO_PT;

  if (reader->eoc)
    {
      // Well it seems, that some oldstyle HPGL files use
      // PS w/o arguments in order to enforce a coordinate setup
      // as if a portrait paper size has been selected.
      // (resulting in a prerotation of 90 degrees). 
      x_size = 33600.0;
      y_size = 47520.0;
    }
  else
    {
      if (hpgs_reader_read_double(reader,&x_size)) return -1;

      // set y-size to sqrt(0.5) * x_size in order to prevent
      // the picture from rotation. This is better then setting
      // the y-size to the (fictional) hard-clip limit, because
      // without rotation we definitely can calculate our own plotsize
      // using -i.
      if (reader->eoc)
	y_size = sqrt(0.5) * x_size;
    }

  if (!reader->eoc &&
      hpgs_reader_read_double(reader,&y_size)) return -1;

  return hpgs_reader_set_plotsize (reader,x_size,y_size);
}

/* 
  Actually set the plot size. User by PS command above and by PJL parser.
*/
int hpgs_reader_set_plotsize (hpgs_reader *reader, double x_size, double y_size)
{
  hpgs_bbox bb = { 0.0,0.0,x_size*HP_TO_PT,y_size*HP_TO_PT };

  reader->x_size = x_size;
  reader->y_size = y_size;

  if (y_size >= x_size)
    {
      reader->P1.x = x_size;
      reader->P1.y = 0.0;
      reader->P2.x = 0.0;
      reader->P2.y = y_size;
    }
  else
    {
      reader->P1.x = 0.0;
      reader->P1.y = 0.0;
      reader->P2.x = x_size;
      reader->P2.y = y_size;
    }

  reader->delta_P.x = reader->P2.x - reader->P1.x; 
  reader->delta_P.y = reader->P2.y - reader->P1.y; 

  // undo any effects from an RO or SC statement.
  reader->rotation = 0;
  reader->sc_type = -1;

  // change the page matrix only, if we don't have a plotsize device.
  if (!reader->plotsize_device)
    hpgs_reader_set_page_matrix (reader,&bb);

  hpgs_reader_set_default_transformation(reader);

  // report plot size only if
  // when we don't have a plotsize device at hands.
  if (reader->plotsize_device)
    return 0;

  return hpgs_setplotsize(reader->device,&reader->page_bbox);
}

/* 
  Change the page matrix according to this content bounding box.
*/
void hpgs_reader_set_page_matrix (hpgs_reader *reader, const hpgs_bbox *bb)
{
  hpgs_bbox rbb;
  double xscale,yscale;
  hpgs_point rcp;

  hpgs_matrix_ixform(&rcp,&reader->current_point,&reader->page_matrix);
  
  // save the content bounding box for propagating it to the
  // page_asset function.
  reader->content_bbox = *bb;

  if  (reader->page_mode == 0)
    {
      reader->page_bbox = *bb;
      hpgs_matrix_set_identity(&reader->page_matrix);
      reader->page_scale = 1.0;
      goto restore_cb;
    }

  reader->page_matrix.mxx =  cos(reader->page_angle * M_PI / 180.0);
  reader->page_matrix.mxy = -sin(reader->page_angle * M_PI / 180.0);

  reader->page_matrix.myx =  sin(reader->page_angle * M_PI / 180.0);
  reader->page_matrix.myy =  cos(reader->page_angle * M_PI / 180.0);
  
  reader->page_matrix.dx = 0.0;
  reader->page_matrix.dy = 0.0;

  if (reader->page_mode == 2) // dynamic page.
    {
      hpgs_matrix_xform_bbox(&rbb,&reader->page_matrix,bb);

      reader->page_bbox.llx = 0.0;
      reader->page_bbox.lly = 0.0;

      reader->page_bbox.urx = (rbb.urx-rbb.llx) + 2.0 * reader->page_border;
      reader->page_bbox.ury = (rbb.ury-rbb.lly) + 2.0 * reader->page_border;

      reader->page_matrix.dx = reader->page_border - rbb.llx;
      reader->page_matrix.dy = reader->page_border - rbb.lly;

      // do we fit on the maximal page size?
      if ((reader->page_width <= 0.0 || reader->page_bbox.urx <= reader->page_width) &&
          (reader->page_height <= 0.0 || reader->page_bbox.ury <= reader->page_height) )
        {
          reader->page_scale = 1.0;
          goto restore_cb;
        }

      if (reader->page_bbox.urx > reader->page_width)
        xscale = reader->page_width/reader->page_bbox.urx;
      else
        xscale = 1.0;

      if (reader->page_bbox.ury> reader->page_height)
        yscale = reader->page_height/reader->page_bbox.ury;
      else
        yscale = 1.0;

      reader->page_scale = HPGS_MIN(xscale,yscale);

      double rscale = 0.0;
      double xx = 1.0;

      // transform the scale to a human-interceptable scale.
      do
        {
          xx *= 0.1;
          rscale = floor(reader->page_scale / xx);
        }
      while (rscale < 2.0);

      reader->page_scale = rscale * xx;

      reader->page_matrix.mxx *= reader->page_scale;
      reader->page_matrix.mxy *= reader->page_scale;

      reader->page_matrix.myx *= reader->page_scale;
      reader->page_matrix.myy *= reader->page_scale;

      reader->page_matrix.dx *= reader->page_scale;
      reader->page_matrix.dy *= reader->page_scale;

      reader->page_bbox.urx *= reader->page_scale;
      reader->page_bbox.ury *= reader->page_scale;

      goto restore_cb;
    }

  // fixed page.
  reader->page_bbox.llx = 0.0;
  reader->page_bbox.lly = 0.0;

  reader->page_bbox.urx = reader->page_width;
  reader->page_bbox.ury = reader->page_height;

  hpgs_matrix_xform_bbox(&rbb,&reader->page_matrix,bb);

  xscale = (reader->page_width - 2.0 * reader->page_border) / (rbb.urx-rbb.llx);
  yscale = (reader->page_height - 2.0 * reader->page_border) / (rbb.ury-rbb.lly);
  
  reader->page_scale = HPGS_MIN(xscale,yscale);

  reader->page_matrix.mxx *= reader->page_scale;
  reader->page_matrix.mxy *= reader->page_scale;
  
  reader->page_matrix.myx *= reader->page_scale;
  reader->page_matrix.myy *= reader->page_scale;
  
  if (reader->page_scale == xscale)
    {
      reader->page_matrix.dx = reader->page_border - reader->page_scale * rbb.llx;
      reader->page_matrix.dy =
        0.5 * (reader->page_height - reader->page_scale * (rbb.ury-rbb.lly)) - reader->page_scale * rbb.lly;
    }
  else
    {
      reader->page_matrix.dx = 
        0.5 * (reader->page_width - reader->page_scale * (rbb.urx-rbb.llx)) - reader->page_scale * rbb.llx;
      reader->page_matrix.dy = reader->page_border - reader->page_scale * rbb.lly;
    }

 restore_cb:
  hpgs_matrix_xform(&reader->current_point,&reader->page_matrix,&rcp);
}

/* 
  HPGL command FR (advance FRame)
*/
int hpgs_reader_do_FR (hpgs_reader *reader)
{
  double advance = reader->x_size;

  if (!reader->eoc)
    {
      if (hpgs_reader_read_double(reader,&advance)) return -1;
    }

  if (hpgs_reader_checkpath(reader)) return -1;

  while (reader->clipsave_depth > 0)
    {
      hpgs_cliprestore(reader->device);
      --reader->clipsave_depth;
    }

  if (reader->frame_asset_func)
    {
      hpgs_bbox frame_bbox;
      
      frame_bbox.llx = reader->frame_x == 0.0 ? reader->content_bbox.llx : reader->frame_x;
      frame_bbox.lly = reader->content_bbox.lly;
      frame_bbox.urx = reader->frame_x + advance * HP_TO_PT;
      frame_bbox.ury = reader->content_bbox.ury;

      if (frame_bbox.urx > frame_bbox.llx)
        {
          int ipage = reader->current_page;
          
          if (reader->frame_asset_func(reader->frame_asset_ctxt,
                                       reader->device,
                                       &reader->page_matrix,
                                       &reader->total_matrix,
                                       &frame_bbox,ipage <= 1 ? 0 : ipage-1) )
            return -1;
        }
    }

  reader->frame_x += advance * HP_TO_PT;
  reader->P1.x -= advance;
  reader->P2.x -= advance;

  return 0;
}

/* 
  HPGL command RO (ROtate)
*/
int hpgs_reader_do_RO (hpgs_reader *reader)
{
  int rot=0;
  hpgs_point p1,p2;
  double dx,dy;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&rot)) return -1;

  switch ((rot - reader->rotation) % 360)
    {
    case 90:
      p1.x = -reader->P1.y;
      p1.y =  reader->P1.x;
      p2.x = -reader->P2.y;
      p2.y =  reader->P2.x;
      break;
    case 180:
      p1.x = -reader->P1.x;
      p1.y = -reader->P1.y;
      p2.x = -reader->P2.x;
      p2.y = -reader->P2.x;
      break;
    case 270:
      p1.x =  reader->P1.y;
      p1.y = -reader->P1.x;
      p2.x =  reader->P2.y;
      p2.y = -reader->P2.x;
      break;
    default: /* 0,360 */
      p1.x = reader->P1.x;
      p1.y = reader->P1.y;
      p2.x = reader->P2.x;
      p2.y = reader->P2.x;
      break;
    }
  
  dx = p1.x < p2.x ? p1.x : p2.x;
  dy = p1.y < p2.y ? p1.y : p2.y;

#ifdef HPGS_DEBUG_XFORM
  hpgs_log("RO: rot_old,rot = %d,%d.\n",reader->rotation,rot);
  hpgs_log("RO: P1 = %g,%g.\n",reader->P1.x,reader->P1.y);
  hpgs_log("RO: P2 = %g,%g.\n",reader->P2.x,reader->P2.y);
#endif

  reader->P1.x = p1.x-dx;
  reader->P1.y = p1.y-dy;
  reader->P2.x = p2.x-dx;
  reader->P2.y = p2.y-dy;

#ifdef HPGS_DEBUG_XFORM
  hpgs_log("RO: P1 = %g,%g.\n",reader->P1.x,reader->P1.y);
  hpgs_log("RO: P2 = %g,%g.\n",reader->P2.x,reader->P2.y);
#endif

  reader->rotation = rot;

  hpgs_reader_set_default_transformation(reader);

  return 0;
}

/* 
  HPGL command SC (SCale)
*/
int hpgs_reader_do_SC (hpgs_reader *reader)
{
  double xmin,xmax,ymin,ymax,left=50.0,bottom=50.0;
  int type=0;

  if (reader->eoc)
    {
      reader->sc_type = -1;
      hpgs_reader_set_default_transformation(reader);
      return 0;
    }

  if (hpgs_reader_read_double(reader,&xmin)) return -1;
  if (reader->eoc) return -1;
  if (hpgs_reader_read_double(reader,&xmax)) return -1;
  if (reader->eoc) return -1;
  if (hpgs_reader_read_double(reader,&ymin)) return -1;
  if (reader->eoc) return -1;
  if (hpgs_reader_read_double(reader,&ymax)) return -1;
  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&type)) return -1;

  if (type == 1 && !reader->eoc)
    {
      if (hpgs_reader_read_double(reader,&left)) return -1;
      if (reader->eoc) return -1;
      if (hpgs_reader_read_double(reader,&bottom)) return -1;
    }

  reader->sc_type = type;
  reader->sc_xmin = xmin;
  reader->sc_xmax = xmax;
  reader->sc_ymin = ymin;
  reader->sc_ymax = ymax;
  reader->sc_bottom = bottom;
  reader->sc_left = left;

  hpgs_reader_set_default_transformation(reader);
  return 0;
}

/* 
  HPGL command IP (Input Point)
*/
int hpgs_reader_do_IP (hpgs_reader *reader)
{
  // get default input point.
  int angle = reader->y_size >= reader->x_size ? 90 : 0;
  
  angle += reader->rotation;

  switch (angle % 360)
    {
    case 90:
      reader->P1.x = reader->x_size;
      reader->P1.y = 0.0;
      reader->P2.x = 0.0;
      reader->P2.y = reader->y_size;
      break;
    case 180:
      break;
      reader->P1.x = reader->x_size;
      reader->P1.y = reader->y_size;
      reader->P2.x = 0.0;
      reader->P2.y = 0.0;
      break;
    case 270:
      reader->P1.x = 0.0;
      reader->P1.y = reader->y_size;
      reader->P2.x = reader->x_size;
      reader->P2.y = 0.0;
      break;
    default: /* 0 */ 
      reader->P1.x = 0.0;
      reader->P1.y = 0.0;
      reader->P2.x = reader->x_size;
      reader->P2.y = reader->y_size;
    }


  // read input point
  if (!reader->eoc)
    {
      if (hpgs_reader_read_double(reader,
                                  angle%180 ?
                                  &reader->P1.y :
                                  &reader->P1.x  )) return -1;
      if (reader->eoc) return -1;
      if (hpgs_reader_read_double(reader,angle%180 ?
                                  &reader->P2.x :
                                  &reader->P1.y  )) return -1;
    }

  if (!reader->eoc)
    {
      if (hpgs_reader_read_double(reader,
                                  angle%180 ?
                                  &reader->P2.y :
                                  &reader->P2.x  )) return -1;
      if (reader->eoc) return -1;
      if (hpgs_reader_read_double(reader,
                                  angle%180 ?
                                  &reader->P1.x :
                                  &reader->P2.y  )) return -1;
    }

  reader->delta_P.x = reader->P2.x - reader->P1.x; 
  reader->delta_P.y = reader->P2.y - reader->P1.y; 

#ifdef HPGS_DEBUG_XFORM
  hpgs_log("IP: angle = %d.\n",angle);
  hpgs_log("IP: P1 = %g,%g.\n",reader->P1.x,reader->P1.y);
  hpgs_log("IP: P2 = %g,%g.\n",reader->P2.x,reader->P2.y);
#endif

  reader->sc_type = -1;
  hpgs_reader_set_default_transformation(reader);

  return 0;
}

/* 
  HPGL command IR (Input point Relative)
*/
int hpgs_reader_do_IR (hpgs_reader *reader)
{
  // get default input point.
  double p1x=0.0,p1y=0.0,p2x,p2y;

  int angle = reader->y_size >= reader->x_size ? 90 : 0;
  
  angle += reader->rotation;

  switch (angle % 360)
    {
    case 90:
      p1x = reader->x_size;
      p1y = 0.0;
      p2x = 0.0;
      p2y = reader->y_size;
      break;
    case 180:
      break;
      p1x = reader->x_size;
      p1y = reader->y_size;
      p2x = 0.0;
      p2y = 0.0;
      break;
    case 270:
      p1x = 0.0;
      p1y = reader->y_size;
      p2x = reader->x_size;
      p2y = 0.0;
      break;
    default:
      p1x = 0.0;
      p1y = 0.0;
      p2x = reader->x_size;
      p2y = reader->y_size;
    }

  // read input point
  if (!reader->eoc)
    {
      double x,y;

      if (hpgs_reader_read_double(reader,&x)) return -1;
      if (hpgs_reader_read_double(reader,&y)) return -1;

      p1x = reader->x_size * x * 0.01;
      p2x = p1x + reader->delta_P.x;
      p1y = reader->y_size * y * 0.01;
      p2y = p1y + reader->delta_P.y;
    }

  if (!reader->eoc)
    {
      if (hpgs_reader_read_double(reader,&p2x)) return -1;
      if (hpgs_reader_read_double(reader,&p2y)) return -1;

      p2x = reader->x_size * p2x * 0.01;
      p2y = reader->y_size * p2y * 0.01;

      reader->delta_P.x = p2x - p1x;
      reader->delta_P.y = p2y - p1y; 
    }

  reader->P1.x = p1x;
  reader->P1.y = p1y;
  reader->P2.x = p2x;
  reader->P2.y = p2y;
  
#ifdef HPGS_DEBUG_XFORM
  hpgs_log("IR: P1 = %g,%g.\n",reader->P1.x,reader->P1.y);
  hpgs_log("IR: P2 = %g,%g.\n",reader->P2.x,reader->P2.y);
#endif

  hpgs_reader_set_default_transformation(reader);

  return 0;
}

/* 
  HPGL command IW (Input Window)
*/
int hpgs_reader_do_IW (hpgs_reader *reader)
{
  // get default input point.
  hpgs_point ll;
  hpgs_point ur;
  hpgs_point p;

  if (hpgs_reader_checkpath(reader)) return -1;

  while (reader->clipsave_depth > 0)
    {
      hpgs_cliprestore(reader->device);
      --reader->clipsave_depth;
    }

  if (reader->eoc) return 0;

  // read input point
  if (hpgs_reader_read_point(reader,&ll,1)) return -1;
  if (hpgs_reader_read_point(reader,&ur,1)) return -1;

  if (hpgs_clipsave(reader->device)) return -1;
  ++reader->clipsave_depth;

  if (hpgs_newpath(reader->device)) return -1;
  if (hpgs_moveto(reader->device,&ll)) return -1;
  p.x = ll.x;
  p.y = ur.y;
  if (hpgs_lineto(reader->device,&p)) return -1;
  if (hpgs_lineto(reader->device,&ur)) return -1;
  p.x = ur.x;
  p.y = ll.y;
  if (hpgs_lineto(reader->device,&p)) return -1;
  if (hpgs_closepath(reader->device)) return -1;
  if (hpgs_clip(reader->device,HPGS_TRUE)) return -1; 

  reader->have_current_point = 0;

  return hpgs_newpath(reader->device);
}
