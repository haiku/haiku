/***********************************************************************
 *                                                                     *
 * $Id: hpgspaintimage.c 281 2006-02-25 19:40:31Z softadm $
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
 * The implementation of the public API for the pixel painter.         *
 *                                                                     *
 ***********************************************************************/

#include<hpgspaint.h>
#include<math.h>
#include<assert.h>
#include<string.h>

//#define HPGS_PAINT_IMAGE_DEBUG

#ifdef HPGS_PAINT_IMAGE_DEBUG
static void log_mat_6(const char *lead, const double *mat)
{
  hpgs_log("%s%10lg,%10lg,%10lg\n%*s%10lg,%10lg,%10lg.\n",
           lead,mat->dx,mat->mxx,mat->mxy,
           strlen(lead)," ",mat->dy,mat->myx,mat->myy);

}
#endif

// multiplicates the given vector with the specified matrix.
// This is here in order to allow efficient inlining of the code.
// 
static void mat_6_mul(hpgs_point *pout, const hpgs_matrix *mat, double xin, double yin)
{
  pout->x = mat->dx + mat->mxx * xin + mat->mxy * yin;
  pout->y = mat->dy + mat->myx * xin + mat->myy * yin;
}

// build matrix from destination to source pixel coordinates.
static void build_inv_matrix(hpgs_matrix *m,
                             const hpgs_paint_clipper *c,
                             const hpgs_image *dest,
                             const hpgs_image *src,
                             const hpgs_point *ll,
                             const hpgs_point *ul,
                             const hpgs_point *lr,
                             const hpgs_point *ur)
{
  // image to device coordinates.
  hpgs_matrix a = { ul->x, ul->y,
                    (lr->x-ll->x)/src->width, (lr->x-ur->x)/src->height,
                    (lr->y-ll->y)/src->width, (lr->y-ur->y)/src->height };

  // device to target raster coordinates.
  hpgs_matrix b = {-0.5-c->bb.llx*dest->width/(c->bb.urx-c->bb.llx), c->y0/c->yfac,
                   dest->width/(c->bb.urx-c->bb.llx), 0.0,
                   0.0,                              -1.0/c->yfac };

  hpgs_matrix tmp;

  // take half a pixel into account.
  a.dx += 0.5 * (a.mxx + a.mxy);
  a.dy += 0.5 * (a.myx + a.myy);

  hpgs_matrix_concat(&tmp,&b,&a);
#ifdef HPGS_PAINT_IMAGE_DEBUG
  log_mat_6("a = ",a);
  log_mat_6("b = ",b);
  log_mat_6("b*a = ",tmp);
#endif
  hpgs_matrix_invert(m,&tmp);
}

typedef int (*put_point_func_t)(hpgs_image *, const hpgs_image *,
                                int, int, const hpgs_point *, double);

static int put_point_0(hpgs_image *dest, const hpgs_image *src,
                       int dest_i, int dest_j, const hpgs_point *src_point,
                       double alpha)
{
  hpgs_paint_color col;

  double a;

  int src_i = (int)rint(src_point->x);
  int src_j = (int)rint(src_point->y);

  if (src_i < 0 || src_i >= src->width) return 0;
  if (src_j < 0 || src_j >= src->height) return 0;

  hpgs_image_get_pixel(src,src_i,src_j,&col,&a);

  if (hpgs_image_define_color (dest,&col))
    return -1;

  return hpgs_image_rop3_pixel(dest,dest_i,dest_j,&col,
                               alpha*a);
}

static int put_point_1(hpgs_image *dest, const hpgs_image *src,
                       int dest_i, int dest_j, const hpgs_point *src_point,
                       double alpha)
{
  hpgs_paint_color col;

  double aa,a=0.0;
  double r=0.0,g=0.0,b=0.0;

  double w;
  double sw = 0.0;

  int src_i = (int)ceil(src_point->x);
  int src_j = (int)ceil(src_point->y);

  double wx = ceil(src_point->x) - src_point->x;
  double wy = ceil(src_point->y) - src_point->y;

  if (src_i < 0 || src_i > src->width) return 0;
  if (src_j < 0 || src_j > src->height) return 0;

  if (src_i > 0)
    {
      if (src_j > 0)
        {
          hpgs_image_get_pixel(src,src_i-1,src_j-1,&col,&aa);
          w = wx+wy;
          r+=w*col.r;
          g+=w*col.g;
          b+=w*col.b;
          a += w*aa;
          sw += w;
        }

      if (src_j < src->height)
        {
          hpgs_image_get_pixel(src,src_i-1,src_j,&col,&aa);
          w = 1.0-wy+wx;
          r+=w*col.r;
          g+=w*col.g;
          b+=w*col.b;
          a += w*aa;
          sw += w;
        }
    }

  if (src_i < src->width)
    {
      if (src_j > 0)
        {
          hpgs_image_get_pixel(src,src_i,src_j-1,&col,&aa);
          w = 1.0-wx+wy;
          r+=w*col.r;
          g+=w*col.g;
          b+=w*col.b;
          a += w*aa;
          sw += w;
        }

      if (src_j < src->height)
        {
          hpgs_image_get_pixel(src,src_i,src_j,&col,&aa);
          w=2.0-wx-wy;
          r+=w*col.r;
          g+=w*col.g;
          b+=w*col.b;
          a += w*aa;
          sw += w;
        }
    }

  if (!sw) return 0;

  col.r = r / sw;
  col.g = g / sw;
  col.b = b / sw;

  if (hpgs_image_define_color (dest,&col))
    return -1;

  return hpgs_image_rop3_pixel(dest,dest_i,dest_j,&col,
                               0.25*alpha*a);
}

/*!

    Draws the intersection of the given image with the current clip
    path of the paint device \c pdv to the destination image of \c pdv.

    The arguments \c ll, \c lr and \c ur are the 
    lower left, lower right and upper right corner of the drawn image
    in world coordinates.

    Return values:

    \li 0 Sucess.
    \li -1 An error ocurred.
            Call \c hpgs_device_get_error in order to retrieve the error message.

*/
int hpgs_paint_device_drawimage(hpgs_paint_device *pdv,
                                const hpgs_image *img,
                                const hpgs_point *ll, const hpgs_point *lr,
                                const hpgs_point *ur)
{
  const hpgs_paint_clipper *c = pdv->clippers[pdv->current_clipper];
  put_point_func_t put_point_func = put_point_0;
  hpgs_point ul;
  double ll_i,lr_i,ur_i,ul_i;
  int iscan0;
  int iscan1;
  int iscan;
  hpgs_matrix imat[6];

  if (pdv->image_interpolation)
    put_point_func = put_point_1;

  ul.x = ll->x + (ur->x - lr->x);
  ul.y = ll->y + (ur->y - lr->y);

  build_inv_matrix(imat,c,pdv->image,img,ll,&ul,lr,ur);

  ll_i = (c->y0 - ll->y)/c->yfac;
  lr_i = (c->y0 - lr->y)/c->yfac;
  ur_i = (c->y0 - ur->y)/c->yfac;
  ul_i = (c->y0 - ul.y)/c->yfac;

  iscan0 = (int)ceil(ll_i);
  if (lr_i < iscan0) iscan0 = (int)ceil(lr_i);
  if (ur_i < iscan0) iscan0 = (int)ceil(ur_i);
  if (ul_i < iscan0) iscan0 = (int)ceil(ul_i);

  if (iscan0 < c->iscan0) iscan0 = c->iscan0;

  iscan1 = (int)floor(ll_i);
  if (lr_i > iscan1) iscan1 = (int)floor(lr_i);
  if (ur_i > iscan1) iscan1 = (int)floor(ur_i);
  if (ul_i > iscan1) iscan1 = (int)floor(ul_i);

  if (iscan1 > c->iscan1) iscan1 = c->iscan1;
  
#ifdef HPGS_PAINT_IMAGE_DEBUG
  hpgs_log("ll_i,lr_i,ur_i,ul_i,iscan0,iscan1=%lg,%lg,%lg,%lg,%d,%d.\n",
           ll_i,lr_i,ur_i,ul_i,iscan0,iscan1);

  print_mat_6(stderr,"imat = ",imat);
#endif

  for (iscan = iscan0; iscan <= iscan1; ++iscan)
    {
      double x[2];
      int ix = 0;
      int io;
      const hpgs_paint_scanline *scanline = c->scanlines+iscan;

      if ((iscan > ll_i && iscan < lr_i) ||
          (iscan < ll_i && iscan > lr_i)   )
        {
          x[ix] = ((ll_i - iscan)*lr->x + (iscan - lr_i)*ll->x) / (ll_i-lr_i);
          ++ix;
        }

      if ((iscan > lr_i && iscan < ur_i) ||
          (iscan < lr_i && iscan > ur_i)   )
        {
          x[ix] = ((lr_i - iscan)*ur->x + (iscan - ur_i)*lr->x) / (lr_i-ur_i);
          ++ix;
        }
      if ((iscan > ur_i && iscan < ul_i) ||
          (iscan < ur_i && iscan > ul_i)   )
        {
          x[ix] = ((ur_i - iscan)*ul.x + (iscan - ul_i)*ur->x) / (ur_i-ul_i);
          ++ix;
        }
      if ((iscan > ul_i && iscan < ll_i) ||
          (iscan < ul_i && iscan > ll_i)   )
        {
          x[ix] = ((ul_i - iscan)*ll->x + (iscan - ll_i)*ul.x) / (ul_i-ll_i);
          ++ix;
        }

      if (ix!=2) continue;

      if (x[0] > x[1])
        {
          double tmp = x[0];
          x[0] = x[1];
          x[1] = tmp;
        }

      if (pdv->overscan)
        {
          int out_state = 0;
          io = 0;

          // go through clip segments antialiased
          while (io<scanline->n_points)
            {
              double xleft,xright,ixleft,ixright;
              double ileft,iright,ii,alpha_left,alpha_right;
              hpgs_point src_point;

              xleft= scanline->points[io].x;

              while (io<scanline->n_points && scanline->points[io].x == xleft)
                {
                  out_state += scanline->points[io].order;
                  ++io;
                }

              if (io >= scanline->n_points) break;

              alpha_left = out_state/256.0;

              xright = scanline->points[io].x;
              alpha_right = (out_state+scanline->points[io].order)/256.0;

              ixright = xright;
              ixleft = xleft;

              if (ixleft < x[0]) ixleft = x[0];
              if (ixleft > x[1]) ixleft = x[1];

              if (ixright < x[0]) ixright = x[0];
              if (ixright > x[1]) ixright = x[1];

              if ((alpha_left == 0.0 && alpha_right == 0.0) || 
                  ixleft >= ixright) continue;

              ixleft = pdv->image->width*(ixleft-c->bb.llx) / (c->bb.urx-c->bb.llx);
              ixright = pdv->image->width*(ixright-c->bb.llx) / (c->bb.urx-c->bb.llx);

              ileft  = ceil(ixleft);
              iright = floor(ixright);
         
              // output left corner points with alpha
              if (ileft >= 1.0)
                {
                  double alpha = (ileft - ixleft) *
                    (alpha_left*(xright-0.5*(ileft+xleft))+alpha_right*(0.5*(ileft+xleft)-xleft))/(xright-xleft);

                  mat_6_mul(&src_point,imat,ileft-1.0,iscan);
              
                  if (put_point_func(pdv->image,img,ileft-1.0,iscan,&src_point,alpha))
                    return -1;

#ifdef HPGS_PAINT_IMAGE_DEBUG
                  hpgs_log("src: %lg,%lg, dest: %lg,%d.\n",
                           src_point->x,src_point->y,ileft-1.0,iscan);
#endif
                }
          
              // output intermediate pixels.
              for (ii = ileft; ii < iright; ++ii)
                {
                  double alpha = (alpha_left*(xright-(ii+0.5))+alpha_right*((ii+0.5)-xleft))/(xright-xleft);

                  mat_6_mul(&src_point,imat,ii,iscan);
                  if (put_point_func(pdv->image,img,ii,iscan,&src_point,alpha))
                    return -1;
                }

              // output right corner points with alpha
              if (iright >= ileft && iright < pdv->image->width)
                {
                  double alpha = (ixright - iright) *
                    (alpha_left*(xright-0.5*(iright+xright))+alpha_right*(0.5*(iright+xright)-xleft))/(xright-xleft);
                  
                  mat_6_mul(&src_point,imat,iright,iscan);
                  
                  if (put_point_func(pdv->image,img,iright,iscan,&src_point,alpha))
                    return -1;
                  
#ifdef HPGS_PAINT_IMAGE_DEBUG
                  hpgs_log("src: %lg,%lg, dest: %lg,%d.\n",
                           src_point->x,src_point->y,iright,iscan);
#endif
                }
            }
        }
      else
        {
          // go through clip segments - non-antialiased
          for (io = 0; io+1<scanline->n_points; io+=2)
            {
              double xleft= scanline->points[io].x;
              double xright= scanline->points[io+1].x;
              double ileft,iright,ii;
              hpgs_point src_point;

              if (xleft < x[0]) xleft = x[0];
              if (xleft > x[1]) xleft = x[1];

              if (xright < x[0]) xright = x[0];
              if (xright > x[1]) xright = x[1];

              if (xleft >= xright) continue;
          
              xleft = pdv->image->width*(xleft-c->bb.llx) / (c->bb.urx-c->bb.llx);
              xright = pdv->image->width*(xright-c->bb.llx) / (c->bb.urx-c->bb.llx);

              ileft  = ceil(xleft);
              iright = floor(xright);
         
              // output left corner points with alpha
              if (ileft >= 1.0)
                {
                  double alpha = ileft - xleft;

                  mat_6_mul(&src_point,imat,ileft-1.0,iscan);
              
                  if (put_point_func(pdv->image,img,ileft-1.0,iscan,&src_point,alpha))
                    return -1;

#ifdef HPGS_PAINT_IMAGE_DEBUG
                  hpgs_log("src: %lg,%lg, dest: %lg,%d.\n",
                           src_point->x,src_point->y,ileft-1.0,iscan);
#endif
                }
          
              // output intermediate pixels.
              for (ii = ileft; ii < iright; ++ii)
                {
                  mat_6_mul(&src_point,imat,ii,iscan);
                  if (put_point_func(pdv->image,img,ii,iscan,&src_point,1.0))
                    return -1;
                }

              // output right corner points with alpha
              if (iright >= ileft && iright < pdv->image->width)
                {
                  double alpha = xright - iright;
                  
                  mat_6_mul(&src_point,imat,iright,iscan);
                  
                  if (put_point_func(pdv->image,img,iright,iscan,&src_point,alpha))
                    return -1;
                  
#ifdef HPGS_PAINT_IMAGE_DEBUG
                  hpgs_log("src: %lg,%lg, dest: %lg,%d.\n",
                           src_point->x,src_point->y,iright,iscan);
#endif
                }
            }
        }
    }

  return 0;
}
