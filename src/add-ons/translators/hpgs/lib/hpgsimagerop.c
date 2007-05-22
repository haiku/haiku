/***********************************************************************
 *                                                                     *
 * $Id: hpgsimagerop.c 372 2007-01-14 18:16:03Z softadm $
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
 * The implementation of the ROP3 clip frame extraction from images.   *
 *                                                                     *
 ***********************************************************************/

#include <hpgs.h>
#include <assert.h>
#if defined ( __MINGW32__ ) || defined ( _MSC_VER )
#include<malloc.h>
#else
#include<alloca.h>
#endif

// #define HPGS_IMAGE_ROP_DEBUG

typedef struct hpgs_img_clip_seg_st hpgs_img_clip_seg;

struct hpgs_img_clip_seg_st
{
  int j;
  int i_vert;
};

typedef struct hpgs_img_clip_line_st hpgs_img_clip_line;

struct hpgs_img_clip_line_st
{
  int i0;
  int j0;

  int i1;
  int j1;

  int key;
  int usage;
};

#define MK_LINE_KEY(i,j) ((j)+(img->width+1)*(i))

static int compare_clip_lines (const void *a, const void *b)
{
  const hpgs_img_clip_line *la = (const hpgs_img_clip_line *)a;
  const hpgs_img_clip_line *lb = (const hpgs_img_clip_line *)b;

  if (la->key < lb->key) return -1;
  if (la->key > lb->key) return 1;
  return 0;
}

static int grow_clip_lines (hpgs_device *device,
                            hpgs_img_clip_line **clip_lines,
                            size_t *clip_lines_sz)
{
  size_t nsz = *clip_lines_sz*2;
  hpgs_img_clip_line *ncl=realloc(*clip_lines,nsz*sizeof(hpgs_img_clip_line));

  if (!ncl)
    return hpgs_set_error(hpgs_i18n("hpgs_image_rop3_clip: Out of memory growing temporary storage."));

  *clip_lines = ncl;
  *clip_lines_sz = nsz;
  return 0;
}

/*! 
   Sets a clip frame to the given device, which encloses all regions
   of the device, which are cover by the image taking into account
   the given ROP3 transfer function.

   The argument \c data must contain a pointer to a two-dimensional
   array of raw pixels of the size of the image. This array is filled
   with pixel values as if the image has been painted to a white
   destination area using the given ROP3 function.

   Return values:
     \li 0 The clip frame is empty, no operation has been performed on
           the output device.

     \li 1 The clip frame is not empty, the clip path has been
           set to the output device using clipsave/moveto/lineto/clip
           operations.

     \li 2 The clip frame covers the whole image, no operation has been
           performed on the output device.

     \li 3 The clip frame is not empty, the visible pixels have all the same color
           and clip path has been set to the output device using
           moveto/lineto/setrgbcolor/fill operations. The image does not have to be
           transferred by subsequent functions, the rgb color of the device has been
           altered.

     \li -1 An error occured on the output device.

*/
int hpgs_image_rop3_clip(hpgs_device *device,
                         hpgs_palette_color *data,
                         const hpgs_image *img,
                         const hpgs_point *ll, const hpgs_point *lr,
                         const hpgs_point *ur,
                         const hpgs_palette_color *p,
                         hpgs_xrop3_func_t xrop3)
{
  // The upper bound for clip_lines_sz is (w+1)*(h+1)*2.
  // This formula is chosen so, that for w==1 and h==1,
  // this result is achieved. For larger pictures, we
  // use 1/8th of the upper bound, which is pretty conservative.
  // If not doing so, the algorithm easily consumes up to 1GB or more,
  // which is too much for alloca, too. So this estimation combined
  // with the usage of malloc should make the code robust.
  size_t clip_lines_sz = ((img->width+1)*(img->height+1)+31)/4;

  hpgs_img_clip_line *clip_lines = (hpgs_img_clip_line *)
    malloc(sizeof(hpgs_img_clip_line)*clip_lines_sz);

  hpgs_img_clip_seg *segs0 = (hpgs_img_clip_seg *)
    hpgs_alloca(sizeof(hpgs_img_clip_seg)*(img->width+1));

  hpgs_img_clip_seg *segs1 = (hpgs_img_clip_seg *)
    hpgs_alloca(sizeof(hpgs_img_clip_seg)*(img->width+1));

  if (!clip_lines || !segs0 || !segs1)
    return hpgs_set_error(hpgs_i18n("hpgs_image_rop3_clip: Out of memory allocating temporary storage."));

  int n_clip_lines = 0;
  int ret = -1;
  int i_seg0,n_segs0 = 0;
  int i_seg1,n_segs1 = 0;

  hpgs_point ul;
  int i,j;

  hpgs_bool all_visible = HPGS_TRUE;
  hpgs_palette_color *first_visible_pixel = 0;
  hpgs_bool single_color = HPGS_FALSE;

  ul.x = ll->x + (ur->x - lr->x);
  ul.y = ll->y + (ur->y - lr->y);

  // first, accumulate lines.
  for (i=0;i<img->height+1;++i)
    {
      // cut the current raster line
      int t_last = 1;
      n_segs1 = 0;

      // this is here in order to construct all lines
      // for the very last grid line
      if (i<img->height)
        {
          for (j=0;j<img->width;++j)
            {
              int t;
              hpgs_paint_color s;
              unsigned r,g,b;
              
              hpgs_image_get_pixel(img,j,i,&s,0);

              r = xrop3(s.r,p->r);
              g = xrop3(s.g,p->g);
              b = xrop3(s.b,p->b);

              // transparent ?
              t = (r == 0x00ff && g == 0x00ff && b == 0x00ff);

              if (t != t_last) // last pixel different transparency ?
                {
                  segs1[n_segs1].j = j;
                  segs1[n_segs1].i_vert = -1;
                  ++n_segs1;
                }

              t_last = t;

              data->r = (unsigned char)r;
              data->g = (unsigned char)g;
              data->b = (unsigned char)b;

              if (!t)
                {
                  if (!first_visible_pixel)
                    {
                      first_visible_pixel = data;
                      single_color = HPGS_TRUE;
                    }
                  else if (single_color &&
                           (first_visible_pixel->r != data->r ||
                            first_visible_pixel->g != data->g ||
                            first_visible_pixel->b != data->b   ))
                    single_color = HPGS_FALSE;
                }

              ++data;
            }

          if (n_segs1 != 1 || segs1[0].j > 0)
            all_visible = HPGS_FALSE;

          // close trailing visible segment.
          if (t_last == 0)
            {
              segs1[n_segs1].j = j;
              segs1[n_segs1].i_vert = -1;
              ++n_segs1;
            }
        }

      assert(n_segs1 <= (img->width+1));

#ifdef HPGS_IMAGE_ROP_DEBUG
      hpgs_log("i=%d: segs1:",i);

      for (j=0;j<n_segs1;++j)
        hpgs_log("%c%d",j?',':' ',segs1[j].j);
      hpgs_log("\n");

      j=n_clip_lines;
#endif

      // construct lines.
      i_seg0 = 0;
      i_seg1 = 0;
      t_last = -1;

      while (i_seg0 < n_segs0 || i_seg1 < n_segs1)
        {
          if (i_seg1 >= n_segs1 || (i_seg0 < n_segs0 && segs0[i_seg0].j < segs1[i_seg1].j))
            {
              // horizontal line.
              if (t_last >= 0)
                {
                  clip_lines[n_clip_lines].i0 = i; 
                  clip_lines[n_clip_lines].i1 = i; 
                  // check for the orientation.
                  if (i_seg0 & 1)
                    {
                      clip_lines[n_clip_lines].j0 = t_last; 
                      clip_lines[n_clip_lines].j1 = segs0[i_seg0].j;
                    }
                  else
                    {
                      clip_lines[n_clip_lines].j0 = segs0[i_seg0].j;
                      clip_lines[n_clip_lines].j1 = t_last; 
                    }

                  if (++n_clip_lines >= clip_lines_sz &&
                      grow_clip_lines(device,&clip_lines,&clip_lines_sz))
                    goto cleanup;

                  t_last = -1;
                }
              else
                t_last = segs0[i_seg0].j;


              ++i_seg0;
            }
          else if (i_seg0 >= n_segs0 ||  segs1[i_seg1].j < segs0[i_seg0].j)
            {
              // horizontal line.
              if (t_last >= 0)
                {
                  clip_lines[n_clip_lines].i0 = i; 
                  clip_lines[n_clip_lines].i1 = i; 
                  // check for the orientation.
                  if (i_seg1 & 1)
                    {
                      clip_lines[n_clip_lines].j0 = segs1[i_seg1].j; 
                      clip_lines[n_clip_lines].j1 = t_last;
                    }
                  else
                    {
                      clip_lines[n_clip_lines].j0 = t_last; 
                      clip_lines[n_clip_lines].j1 = segs1[i_seg1].j;
                    }
                  if (++n_clip_lines >= clip_lines_sz &&
                      grow_clip_lines(device,&clip_lines,&clip_lines_sz))
                    goto cleanup;
                  t_last = -1;
                }
              else
                t_last = segs1[i_seg1].j;

              // create vertical line.
              clip_lines[n_clip_lines].j0 = segs1[i_seg1].j; 
              clip_lines[n_clip_lines].j1 = segs1[i_seg1].j; 

              // check for the orientation.
              if (i_seg1 & 1)
                {
                  clip_lines[n_clip_lines].i0 = i+1;
                  clip_lines[n_clip_lines].i1 = i;
                }
              else
                {
                  clip_lines[n_clip_lines].i0 = i;
                  clip_lines[n_clip_lines].i1 = i+1;
                }

              segs1[i_seg1].i_vert = n_clip_lines;
              if (++n_clip_lines >= clip_lines_sz &&
                  grow_clip_lines(device,&clip_lines,&clip_lines_sz))
                goto cleanup;

              ++i_seg1;
            }
          else
            {
              assert(segs0[i_seg0].j == segs1[i_seg1].j);
              
              // horizontal line.
              if (t_last >= 0)
                {
                  clip_lines[n_clip_lines].i0 = i; 
                  clip_lines[n_clip_lines].i1 = i; 
                  // check for the orientation.
                  if (i_seg1 & 1)
                    {
                      clip_lines[n_clip_lines].j0 = segs1[i_seg1].j; 
                      clip_lines[n_clip_lines].j1 = t_last;
                    }
                  else
                    {
                      clip_lines[n_clip_lines].j0 = t_last; 
                      clip_lines[n_clip_lines].j1 = segs1[i_seg1].j;
                    }

                  if (++n_clip_lines >= clip_lines_sz &&
                      grow_clip_lines(device,&clip_lines,&clip_lines_sz))
                    goto cleanup;
                }

              if ((i_seg0 & 1) == (i_seg1 & 1))
                {
                  // extend segment
                  int il = segs0[i_seg0].i_vert;
                  if (i_seg1 & 1)
                    clip_lines[il].i0 = i+1;
                  else
                    clip_lines[il].i1= i+1;
                  
                  segs1[i_seg1].i_vert = il;
                  t_last = -1;
                }
              else
                {
                  // create new segment.
                  clip_lines[n_clip_lines].j0 = segs1[i_seg1].j;
                  clip_lines[n_clip_lines].j1 = segs1[i_seg1].j;

                  if (i_seg1 & 1)
                    {
                      clip_lines[n_clip_lines].i0 = i+1;
                      clip_lines[n_clip_lines].i1 = i;
                    }
                  else
                    {
                      clip_lines[n_clip_lines].i0 = i;
                      clip_lines[n_clip_lines].i1 = i+1;
                    }

                  segs1[i_seg1].i_vert = n_clip_lines;

                  if (++n_clip_lines >= clip_lines_sz &&
                      grow_clip_lines(device,&clip_lines,&clip_lines_sz))
                    goto cleanup;

                  t_last = segs1[i_seg1].j;
                }

              ++i_seg0;
              ++i_seg1;
            }
        }

#ifdef HPGS_IMAGE_ROP_DEBUG
      hpgs_log("i=%d: lines: ",i);

      for (;j<n_clip_lines;++j)
        hpgs_log("(%d,%d,%d,%d)",
                clip_lines[j].i0,clip_lines[j].j0,
                clip_lines[j].i1,clip_lines[j].j1);
      hpgs_log("\n");
#endif
      assert (t_last == -1);

      // swap clip segment caches
      {
        hpgs_img_clip_seg *tmp = segs0;
        segs0=segs1;
        segs1=tmp;
      }
      n_segs0 = n_segs1;
    }

#ifdef HPGS_IMAGE_ROP_DEBUG
  hpgs_log("clip_img: n_clip_lines,all_visible = %d,%d.\n",
           n_clip_lines,all_visible);
#endif

  if (n_clip_lines <= 0) { ret = 0; goto cleanup; }
  if (all_visible && !single_color)  { ret = 2; goto cleanup; }

  assert(n_clip_lines <= (img->width+1)*(img->height+1)*2);

  // OK, now create the lookup key of the lines.
  for (i=0;i<n_clip_lines;++i)
    {
      clip_lines[i].key = MK_LINE_KEY(clip_lines[i].i0,clip_lines[i].j0);
      clip_lines[i].usage = 0;
    }

  // sort the table
  qsort(clip_lines,n_clip_lines,sizeof(hpgs_img_clip_line),compare_clip_lines);

  if (!single_color && hpgs_clipsave(device)) goto cleanup;

  // now construct the clip path.
  for (i = 0;i<n_clip_lines;++i)
    {
      hpgs_img_clip_line *line = clip_lines+i;
      int iline = 0;

      if (line->usage) continue;

      do
        {
          hpgs_point p;
          int key,i0,i1;

          p.x = ul.x +
            line->j0 * (lr->x - ll->x) / img->width +
            line->i0 * (lr->x - ur->x) / img->height ;
          
          p.y = ul.y +
            line->j0 * (lr->y - ll->y) / img->width +
            line->i0 * (lr->y - ur->y) / img->height ;
          
          key = MK_LINE_KEY(line->i1,line->j1);

          if (iline)
            {
              if (hpgs_lineto(device,&p))  goto cleanup;
            }
          else
            {
              if (hpgs_moveto(device,&p))  goto cleanup;
            }

#ifdef HPGS_IMAGE_ROP_DEBUG
          hpgs_log("(%d,%d,%d,%d)",
                  line->i0,line->j0,
                  line->i1,line->j1);
#endif
          ++iline;
          line->usage=1;

          // binary search
          i0 = 0;
          i1 = n_clip_lines;

          while (i1>i0)
            {
              int ii = i0+(i1-i0)/2;
              
              if (clip_lines[ii].key < key)
                i0 = ii+1;
              else
                i1 = ii;
            }

          while (clip_lines[i0].usage &&
                 i0 < n_clip_lines-1 &&
                 clip_lines[i0+1].key == key)
            ++i0;

          assert(i0 < n_clip_lines && key == clip_lines[i0].key);

          line = clip_lines+i0;

          assert (line);

          if (line->usage && line < clip_lines + n_clip_lines &&
              line[1].key == key)
            ++line;
        }
      while (!line->usage);

      assert (line->i0 == clip_lines[i].i0 &&
              line->j0 == clip_lines[i].j0   );

#ifdef HPGS_IMAGE_ROP_DEBUG
      hpgs_log("\n");
#endif
      if (hpgs_closepath(device)) goto cleanup;
    }

  if (single_color)
    {
      hpgs_color c;

      c.r = first_visible_pixel->r / 255.0;
      c.g = first_visible_pixel->g / 255.0;
      c.b = first_visible_pixel->b / 255.0;

      if (hpgs_setrgbcolor(device,&c)) goto cleanup;
      if (hpgs_fill(device,HPGS_FALSE)) goto cleanup;
      if (hpgs_newpath(device)) goto cleanup;

      ret = 3;
    }

  else
    {
      if (hpgs_clip(device,HPGS_FALSE)) goto cleanup;
      if (hpgs_newpath(device)) goto cleanup;

      ret = 1;
    }

 cleanup:
  if (clip_lines) free(clip_lines);
  return ret;
}
