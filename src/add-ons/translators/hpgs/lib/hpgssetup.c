/***********************************************************************
 *                                                                     *
 * $Id: hpgssetup.c 362 2006-10-16 14:13:48Z softadm $
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
#include <errno.h>

/*
  Standard values.
*/
static hpgs_color std_pen_colors[8] =
  {
    { 1.0, 1.0, 1.0 },
    { 0.0, 0.0, 0.0 },
    { 1.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0 },
    { 1.0, 1.0, 0.0 },
    { 0.0, 0.0, 1.0 },
    { 1.0, 0.0, 1.0 },
    { 0.0, 1.0, 1.0 }
  };

static double std_pen_widths[8] =
  {
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0
  };

static int std_linetype_nsegs[17] =
  {
    9, 7, 7, 5, 5, 3, 3, 3, 0, 2, 2, 2, 4, 4, 6, 6, 8
  };

static float std_linetype_segs[17][20] =
  {
    {24, 10,  1, 10, 10, 10,  1, 10, 24, 0,0,0,0,0,0,0,0,0,0,0},
    {34, 10,  1, 10,  1, 10, 34,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {25, 10, 10, 10, 10, 10, 25,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {35, 10, 10, 10, 35,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {40, 10,  1, 10, 39,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {35, 30, 35,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {25, 50, 25,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    { 1, 99,  0,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    { 0,  0,  0,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    { 1, 99,  0,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {50, 50,  0,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {70, 30,  0,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {79, 10,  1, 10,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {70, 10, 10, 10,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {50, 10, 10, 10, 10, 10,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {68, 10,  1, 10,  1, 10,  0,  0,  0, 0,0,0,0,0,0,0,0,0,0,0},
    {48, 10,  1, 10, 10, 10,  1, 10,  0, 0,0,0,0,0,0,0,0,0,0,0}
  };


void hpgs_reader_set_default_state (hpgs_reader *reader)
{ 
  reader->frame_x=0.0;
  reader->frame_y=0.0;

  reader->x_size = 47520.0 * HP_TO_PT;
  reader->y_size = 33600.0 * HP_TO_PT;

  reader->P1.x = 0.0;
  reader->P1.y = 0.0;
  reader->P2.x = reader->x_size;
  reader->P2.y = reader->y_size;
  reader->delta_P.x = reader->P2.x - reader->P1.x; 
  reader->delta_P.y = reader->P2.y - reader->P1.y; 

  reader->rotation=0;

  reader->sc_type=-1;
  reader->sc_xmin=0.0;
  reader->sc_xmax=1.0;
  reader->sc_ymin=0.0;
  reader->sc_ymax=1.0;
  reader->sc_left = 50.0;
  reader->sc_bottom = 50.0;

  hpgs_reader_set_default_transformation (reader);

  reader->rop3 = 252;
  reader->src_transparency = HPGS_TRUE;
  reader->pattern_transparency = HPGS_TRUE;

  reader->pen_down = 0;
  reader->pen_width_relative = 0;

  reader->label_term= '\003';
  reader->label_term_ignore = 1;
  reader->polygon_mode = 0;
  reader->poly_buffer_size = 0;
  reader->polygon_open = 0;
  reader->have_current_point = 0;
  reader->current_point.x = reader->page_matrix.dx;
  reader->current_point.y = reader->page_matrix.dy;
  reader->current_pen = 0;
  reader->current_linetype = 0;
  reader->absolute_plotting = 1;
  reader->anchor_point.x = 0.0;
  reader->anchor_point.y = 0.0;

  reader->current_ft = 1;
  reader->ft3_angle = 0.0;
  reader->ft3_spacing = 0.0;
  reader->ft4_angle = 0.0;
  reader->ft4_spacing = 0.0;
  reader->ft10_level = 100.0;

  reader->default_encoding = 0;
  reader->default_face = 0;
  reader->default_spacing = 0;
  reader->default_pitch = 9.0;
  reader->default_height = 11.5;
  reader->default_posture = 0;
  reader->default_weight = 0;

  reader->alternate_encoding = 0;
  reader->alternate_face = 0;
  reader->alternate_spacing = 0;
  reader->alternate_pitch = 9.0;
  reader->alternate_height = 11.5;
  reader->alternate_posture = 0;
  reader->alternate_weight = 0;

  reader->alternate_font = 0;
  reader->cr_point = reader->current_point;
  reader->current_char_size.x = 0.0;
  reader->current_char_size.y = 0.0;
  reader->current_extra_space.x = 0.0;
  reader->current_extra_space.y = 0.0;
  reader->current_slant = 0.0;
  reader->current_label_origin = 1;
  reader->current_text_path = 0;
  reader->current_text_line = 0;
  reader->current_label_angle = 0.0;

  // standard linetypes. (-8,...8)
  memcpy(reader->linetype_nsegs,std_linetype_nsegs,sizeof(std_linetype_nsegs));
  memcpy(reader->linetype_segs,std_linetype_segs,sizeof(std_linetype_segs));
}

void hpgs_reader_set_std_pen_colors(hpgs_reader *reader,
                                    int i0, int n)
{
  int n_std = sizeof(std_pen_colors)/sizeof(hpgs_color);

  if (i0 >= n_std) return;
  if (n <= 0) return;
  if (i0+n >= n_std) n = n_std-i0;

  memcpy(reader->pen_colors+i0,std_pen_colors+i0,sizeof(hpgs_color)*n);
}

static hpgs_reader_pcl_palette *hpgs_reader_new_pcl_palette ()
{
  hpgs_reader_pcl_palette *ret=(hpgs_reader_pcl_palette *)malloc(sizeof(hpgs_reader_pcl_palette));

  if (!ret) return 0;

  memset(ret->colors,0,sizeof(hpgs_palette_color)*256);
  ret->colors[0].r = 0xff;
  ret->colors[0].g = 0xff;
  ret->colors[0].b = 0xff;

  ret->last_color.r = 0;
  ret->last_color.g = 0;
  ret->last_color.b = 0;

  ret->cid_space = 0;
  ret->cid_enc = 1;
  ret->cid_bpi = 1;
  ret->cid_bpc[0] = 8;
  ret->cid_bpc[1] = 8;
  ret->cid_bpc[2] = 8;

  return ret;
}

static hpgs_reader_pcl_palette *hpgs_reader_dup_pcl_palette (const hpgs_reader_pcl_palette *p)
{
  hpgs_reader_pcl_palette *ret=(hpgs_reader_pcl_palette *)malloc(sizeof(hpgs_reader_pcl_palette));

  if (!ret) return 0;

  memcpy(ret,p,sizeof(hpgs_reader_pcl_palette));

  return ret;
}

static void hpgs_reader_destroy_pcl_palette (hpgs_reader_pcl_palette *p)
{
  if (p)
    free(p);
}

int hpgs_reader_push_pcl_palette (hpgs_reader *reader)
{
  if (reader->pcl_i_palette >= HPGS_MAX_PCL_PALETTES-1)
    return hpgs_set_error(hpgs_i18n("PCL palette stack overflow (maximal stack size = %d)."),
                          HPGS_MAX_PCL_PALETTES);


  if (reader->pcl_i_palette >= 0)
    reader->pcl_palettes[reader->pcl_i_palette+1] =
      hpgs_reader_dup_pcl_palette (reader->pcl_palettes[reader->pcl_i_palette]);
  else
    reader->pcl_palettes[reader->pcl_i_palette+1] =
      hpgs_reader_new_pcl_palette ();

  if (!reader->pcl_palettes[reader->pcl_i_palette+1])
    return hpgs_set_error(hpgs_i18n("Out of memory pushing PCL palette."));

  ++reader->pcl_i_palette;

  return 0;
}

int hpgs_reader_pop_pcl_palette (hpgs_reader *reader)
{
  if (reader->pcl_i_palette <= 0)
    return hpgs_set_error(hpgs_i18n("PCL palette stack underflow."));

  hpgs_reader_destroy_pcl_palette(reader->pcl_palettes[reader->pcl_i_palette]);

  --reader->pcl_i_palette;

  return 0;
}

void hpgs_reader_set_defaults (hpgs_reader *reader)
{
  int i;

  if (reader->pen_widths)
    free(reader->pen_widths);

  if (reader->pen_colors)
    free(reader->pen_colors);

  if (reader->poly_buffer)
    free(reader->poly_buffer);

  // standard pen widths.
  reader->npens = 8;

  reader->pen_widths = (double*)malloc(sizeof(std_pen_widths));
  memcpy(reader->pen_widths,std_pen_widths,sizeof(std_pen_widths));

  reader->pen_colors = (hpgs_color *)malloc(sizeof(std_pen_colors));
  memcpy(reader->pen_colors,std_pen_colors,sizeof(std_pen_colors));

  reader->min_color.r=0.0;
  reader->min_color.g=0.0;
  reader->min_color.b=0.0;

  reader->max_color.r=255.0;
  reader->max_color.g=255.0;
  reader->max_color.b=255.0;

  reader->poly_buffer_alloc_size = 256;
  reader->poly_buffer_size = 0;

  reader->poly_buffer = (hpgs_reader_poly_point *)
    malloc(sizeof(hpgs_reader_poly_point)*reader->poly_buffer_alloc_size);

  hpgs_reader_set_default_state (reader);

  reader->clipsave_depth = 0;

  for (i=0;i<=reader->pcl_i_palette;++i)
    hpgs_reader_destroy_pcl_palette(reader->pcl_palettes[i]);

  reader->pcl_i_palette = -1;

  hpgs_reader_push_pcl_palette(reader);

  for (i=0;i<8;++i)
    if (reader->pcl_raster_data[i])
      {
        free(reader->pcl_raster_data[i]);
        reader->pcl_raster_data[i] = 0;
      }

  if (reader->pcl_image)
    hpgs_image_destroy(reader->pcl_image);

  reader->pcl_scale = 72.0/300.0;
  reader->pcl_hmi = 7.2;
  reader->pcl_vmi = 12.0;
  reader->pcl_point.x = 0.0;
  reader->pcl_point.y = 0.0;

  reader->pcl_raster_mode = -1;
  reader->pcl_raster_presentation = 0;
  reader->pcl_raster_src_width = 0;
  reader->pcl_raster_src_height = 0;
  reader->pcl_raster_dest_width = 0;
  reader->pcl_raster_dest_height = 0;
  reader->pcl_raster_res = 75;
  reader->pcl_raster_compression  = 0;
  reader->pcl_raster_y_offset  = 0;
  reader->pcl_raster_plane  = 0;
  reader->pcl_raster_line = 0;

  reader->pcl_raster_data_size = 0;
  reader->pcl_raster_planes = 1;

  reader->pcl_image=0;

  reader->bytes_ignored = 0;
  reader->eoc = 0;
}

/* initializes the device */
static int init_device(hpgs_reader *reader)
{
  hpgs_color pat = { 0.0,0.0,0.0 };

  reader->rop3 = 252;
  reader->src_transparency = HPGS_TRUE;
  reader->pattern_transparency = HPGS_TRUE;

  if (hpgs_setlinejoin(reader->device,hpgs_join_miter)) return -1;
  if (hpgs_setlinecap(reader->device,hpgs_cap_butt)) return -1;
  if (hpgs_setmiterlimit(reader->device,5.0)) return -1;
  if (hpgs_setrop3(reader->device,
                   reader->rop3,
                   reader->src_transparency,
                   reader->pattern_transparency)) return -1;
  if (hpgs_setpatcol(reader->device,&pat)) return -1;
  
  return 0;
}

/* 
  HPGL command BP (Begin Plot)
*/
int hpgs_reader_do_BP (hpgs_reader *reader)
{
  while (!reader->eoc)
    {
      int kind;
      int dummy;
      char header[HPGS_MAX_LABEL_SIZE];
      if (hpgs_reader_read_int(reader,&kind)) return -1;
      if (reader->eoc) return 0;

      switch (kind)
	{
	case 1:
	  if (hpgs_reader_read_new_string(reader,header)) return -1;
	  if (reader->verbosity)
	    hpgs_log(hpgs_i18n("Plot Title: %.*s\n"),HPGS_MAX_LABEL_SIZE,header);
	  break;
	case 2:
	case 3:
	case 4:
	case 5:
	  if (hpgs_reader_read_int(reader,&dummy)) return -1;
	  break;
	default:
	  return -1;
	}
    }

  return init_device(reader);
}

/* 
  HPGL command CO (COmment)
*/
int hpgs_reader_do_CO (hpgs_reader *reader)
{
  char comment[HPGS_MAX_LABEL_SIZE];

  if (reader->eoc) return 0;

  if (hpgs_reader_read_new_string(reader,comment)) return -1;

  if (reader->verbosity)
    hpgs_log(hpgs_i18n("Comment: %.*s\n"),HPGS_MAX_LABEL_SIZE,comment);

  return reader->eoc ? 0 : -1;
}

/* 
  HPGL command IN (Initialize Plot)
*/
int hpgs_reader_do_IN (hpgs_reader *reader)
{
  int i;

  if (hpgs_reader_checkpath(reader)) return -1;

  if (!reader->eoc)
    {
      int dummy;
      if (hpgs_reader_read_int(reader,&dummy)) return -1;
    }
  
  while (reader->clipsave_depth > 0)
    {
      hpgs_cliprestore(reader->device);
      --reader->clipsave_depth;
    }

  hpgs_reader_set_default_state (reader);
  
  for (i=0;i<reader->npens;++i)
    reader->pen_widths[i] = 1.0;

  return init_device(reader);
}

/*
   Internal function, which calls the page asset callback and
   does a showpage on the device.
 */
int hpgs_reader_showpage (hpgs_reader *reader, int ipage)
{
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
      frame_bbox.urx = reader->content_bbox.urx;
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

  if (reader->page_asset_func &&
      reader->page_asset_func(reader->page_asset_ctxt,
                              reader->device,
                              &reader->page_matrix,
                              &reader->total_matrix,
                              &reader->content_bbox,ipage <= 1 ? 0 : ipage-1) )
    return -1;

  return hpgs_showpage(reader->device,ipage);
}

/* 
  HPGL command PG (advance PaGe)
*/
int hpgs_reader_do_PG(hpgs_reader *reader)
{
  int ret;

  ret = hpgs_reader_showpage(reader,reader->current_page);

  if (ret) return -1;
  
  ++reader->current_page;

  // if wee are not in singlepage mode,
  // try to set the individual page size.
  if (reader->current_page > 0 &&
      reader->plotsize_device &&
      (hpgs_device_capabilities(reader->device) & HPGS_DEVICE_CAP_MULTISIZE))
    {
      hpgs_bbox page_bb;
      int ret = hpgs_getplotsize(reader->plotsize_device,reader->current_page,&page_bb);

      if (ret < 0) return -1;

      // if ret is 1, we got the overall bounding box and the plotsize device
      // knows about no more pages.
      // So we stop the interpretation at this point.
      return 2;

      // set the plotsize of the next page.
      if (reader->verbosity)
        hpgs_log(hpgs_i18n("Bounding Box of Page %d: %g %g %g %g.\n"),
                 reader->current_page,
                 page_bb.llx,page_bb.lly,
                 page_bb.urx,page_bb.ury );
      
      // change the page layout.
      hpgs_reader_set_page_matrix(reader,&page_bb);
      hpgs_reader_set_default_transformation(reader);

      if (hpgs_setplotsize(reader->device,&reader->page_bbox) < 0)
        return -1;
    }

  // stop the interpretation, if we are in singlepage mode.
  return reader->current_page > 0 ? 0 : 2;
}

/* 
  HPGL command UL (User defined Linetype)
*/
int hpgs_reader_do_UL (hpgs_reader *reader)
{
  int itype,nsegs;

  if (reader->eoc)
    {
      memcpy(reader->linetype_nsegs,std_linetype_nsegs,sizeof(std_linetype_nsegs));
      memcpy(reader->linetype_segs,std_linetype_segs,sizeof(std_linetype_segs));
      return 0;
    }

  if (hpgs_reader_read_int(reader,&itype)) return -1;

  if (itype < -8 || itype > 8)
    return hpgs_set_error(hpgs_i18n("Illegal linetype %d given."),itype);

  if (reader->eoc)
    {
      memcpy(reader->linetype_segs[8+itype],
	     std_linetype_segs[8+itype],sizeof(std_linetype_segs[0]));
      memcpy(reader->linetype_segs[8-itype],
	     std_linetype_segs[8-itype],sizeof(std_linetype_segs[0]));

      reader->linetype_nsegs[8+itype] = std_linetype_nsegs[8+itype];
      reader->linetype_nsegs[8+itype] = std_linetype_nsegs[8-itype];
      return 0;
    }

  nsegs = 0;
  
  while (!reader->eoc)
    {
      double gap;
      if (hpgs_reader_read_double(reader,&gap)) return -1;
      
      reader->linetype_segs[8+itype][nsegs] = gap;
      reader->linetype_segs[8-itype][nsegs] = gap;
      ++nsegs;
    }
  
  reader->linetype_nsegs[8+itype] = nsegs;
  reader->linetype_nsegs[8-itype] = nsegs;

  return 0;
}

/* 
  HPGL command MC (Merge Control)
*/
int hpgs_reader_do_MC (hpgs_reader *reader)
{
  int mode=0,rop=252;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&mode)) return -1;
 
  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&rop)) return -1;
 
  reader->rop3 = mode ? rop : 252;

  return hpgs_setrop3(reader->device,reader->rop3,
                      reader->src_transparency,
                      reader->pattern_transparency );
}

/* 
  HPGL command TR (Transparency Mode)
*/
int hpgs_reader_do_TR (hpgs_reader *reader)
{
  int n=1;

  if (!reader->eoc &&
      hpgs_reader_read_int(reader,&n)) return -1;
 
  reader->src_transparency = (n != 0);

  return hpgs_setrop3(reader->device,reader->rop3,
                      reader->src_transparency,
                      reader->pattern_transparency );
}
