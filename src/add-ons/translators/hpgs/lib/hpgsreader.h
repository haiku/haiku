/***********************************************************************
 *                                                                     *
 * $Id: hpgsreader.h 381 2007-02-20 09:06:38Z softadm $
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
 * Private subroutines used by the hpgs_reader.                        *
 *                                                                     *
 ***********************************************************************/

#ifndef __HPGS_READER_H__
#define __HPGS_READER_H__

#include <hpgs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! \file hpgsreader.h

   \brief The private interfaces for implementing the HPGL reader.

   A header file, which declares the private structures and functions
   used to implement the HPGL reader \c hpgs_reader.
*/

#define HPGS_MAX_PCL_PALETTES 20

#define MM_TO_PT (72.0 / 25.4)
#define HP_TO_PT (72.0 / (25.4 * 40.0))

#define MAKE_COMMAND(a,b) (((int)(a) << 8) + (b))

#define AA_CMD  MAKE_COMMAND('A','A')
#define AC_CMD  MAKE_COMMAND('A','C')
#define AD_CMD  MAKE_COMMAND('A','D')
#define AR_CMD  MAKE_COMMAND('A','R')
#define AT_CMD  MAKE_COMMAND('A','T')
#define BP_CMD  MAKE_COMMAND('B','P')
#define BR_CMD  MAKE_COMMAND('B','R')
#define BZ_CMD  MAKE_COMMAND('B','Z')
#define CI_CMD  MAKE_COMMAND('C','I')
#define CO_CMD  MAKE_COMMAND('C','O')
#define CP_CMD  MAKE_COMMAND('C','P')
#define CR_CMD  MAKE_COMMAND('C','R')
#define DI_CMD  MAKE_COMMAND('D','I')
#define DR_CMD  MAKE_COMMAND('D','R')
#define DT_CMD  MAKE_COMMAND('D','T')
#define DV_CMD  MAKE_COMMAND('D','V')
#define EA_CMD  MAKE_COMMAND('E','A')
#define EP_CMD  MAKE_COMMAND('E','P')
#define ER_CMD  MAKE_COMMAND('E','R')
#define ES_CMD  MAKE_COMMAND('E','S')
#define EW_CMD  MAKE_COMMAND('E','W')
#define FP_CMD  MAKE_COMMAND('F','P')
#define FR_CMD  MAKE_COMMAND('F','R')
#define FT_CMD  MAKE_COMMAND('F','T')
#define IN_CMD  MAKE_COMMAND('I','N')
#define IP_CMD  MAKE_COMMAND('I','P')
#define IR_CMD  MAKE_COMMAND('I','R')
#define IW_CMD  MAKE_COMMAND('I','W')
#define LA_CMD  MAKE_COMMAND('L','A')
#define LB_CMD  MAKE_COMMAND('L','B')
#define LO_CMD  MAKE_COMMAND('L','O')
#define LT_CMD  MAKE_COMMAND('L','T')
#define MC_CMD  MAKE_COMMAND('M','C')
#define MG_CMD  MAKE_COMMAND('M','G')
#define NP_CMD  MAKE_COMMAND('N','P')
#define PA_CMD  MAKE_COMMAND('P','A')
#define PC_CMD  MAKE_COMMAND('P','C')
#define PD_CMD  MAKE_COMMAND('P','D')
#define PE_CMD  MAKE_COMMAND('P','E')
#define PG_CMD  MAKE_COMMAND('P','G')
#define PM_CMD  MAKE_COMMAND('P','M')
#define PP_CMD  MAKE_COMMAND('P','P')
#define PR_CMD  MAKE_COMMAND('P','R')
#define PS_CMD  MAKE_COMMAND('P','S')
#define PU_CMD  MAKE_COMMAND('P','U')
#define PW_CMD  MAKE_COMMAND('P','W')
#define RA_CMD  MAKE_COMMAND('R','A')
#define RO_CMD  MAKE_COMMAND('R','O')
#define RR_CMD  MAKE_COMMAND('R','R')
#define RT_CMD  MAKE_COMMAND('R','T')
#define SA_CMD  MAKE_COMMAND('S','A')
#define SC_CMD  MAKE_COMMAND('S','C')
#define SD_CMD  MAKE_COMMAND('S','D')
#define SI_CMD  MAKE_COMMAND('S','I')
#define SL_CMD  MAKE_COMMAND('S','L')
#define SM_CMD  MAKE_COMMAND('S','M')
#define SP_CMD  MAKE_COMMAND('S','P')
#define SR_CMD  MAKE_COMMAND('S','R')
#define SS_CMD  MAKE_COMMAND('S','S')
#define TR_CMD  MAKE_COMMAND('T','R')
#define UL_CMD  MAKE_COMMAND('U','L')
#define WG_CMD  MAKE_COMMAND('W','G')
#define WU_CMD  MAKE_COMMAND('W','U')

/*! @addtogroup reader
 *  @{
 */

typedef struct hpgs_reader_poly_point_st hpgs_reader_poly_point;

/*! \brief A point in hte HPGL polygon buffer

   This structure holds a point in the polygon buffer of HPGL
   used to implement PM, EP and FP commands.
*/
struct hpgs_reader_poly_point_st
{
  hpgs_point p; //!< The coordinates of the point
  int flag;     //!< 0 moveto, 1 lineto, 2 curveto
};

typedef struct hpgs_reader_pcl_palette_st hpgs_reader_pcl_palette;

/*! \brief A PCL palette as used by PCL push/pop palette.

   This structure holds all properties which are saved/resoterd through PCL push/pop palette.
*/
struct hpgs_reader_pcl_palette_st
{
  hpgs_palette_color colors[256]; /*!< The palette colors. */
  hpgs_palette_color last_color; /*!< The PCL color currently being assembled. */

  int cid_space; /*!< The current PCL color space. */
  int cid_enc;   /*!< The current PCL pixel encoding scheme. */
  int cid_bpi;   /*!< PCL Bits per color index. */
  int cid_bpc[3];/*!< PCL Bits per color component. */ 
};

/*! \brief A HPGL interpreter.

   This structure holds all the states used during the interpretation
   of a HPGL stream. Users of the library usually don't have to cope
   with details of this structure.
*/
struct hpgs_reader_st
{
  hpgs_istream *in;    /*!< The current input stream. */
  hpgs_device *device; /*!< The current output device. */
  hpgs_device *plotsize_device; /*!< The current plotsize device. */

  int current_page; /*!< The number of the current page. -1...single page mode. */

  int verbosity; /*!< The verbosity level in effect. */

  double lw_factor;      /*!< The linewidth scaling factor. */
  /*@{ */
  /*! The paper size in PostScript pt (1/72 inch). */
  double x_size,y_size; /*@} */
  /*@{ */
  /*! The frame advance vector in PostScript pt (1/72 inch). */
  double frame_x,frame_y; /*@} */
  hpgs_point P1;  /*!< HPGL frame point P1 in PostScript pt (1/72 inch). */
  hpgs_point P2;  /*!< HPGL frame point P2 in PostScript pt (1/72 inch). */
  hpgs_point delta_P; /*!< The difference of P2-P1 as set by the IP command. */
  int rotation;       /*!< The current rotation angle (90/180/270/360). */

  /*@{ */
  /*! The current effective coordinate scaling of a SC command. */
  int sc_type;
  double sc_xmin;
  double sc_xmax;
  double sc_ymin;
  double sc_ymax;
  double sc_left;
  double sc_bottom;
  /*@} */

  int rop3;                       /*!< ROP3 operation in effect. */
  hpgs_bool src_transparency;     /*!< ROP3 source transparency. */
  hpgs_bool pattern_transparency; /*!< ROP3 pattern transparency. */
   
  hpgs_matrix world_matrix; /*!< transformation matrix for the transformation of HPGL to PostScript
                                 (world) coordinates usually given in points (1/72 inch). */

  double world_scale; /*!< sqrt(|det(world_matrix)|) */

  hpgs_matrix page_matrix; /*!< transformation matrix for the transformation of
                                PostScript (world) coordinates usually given in points (1/72 inch)
                                to user defined page-coorindates. */

  double page_scale; /*!< sqrt(|det(page_matrix)|) */

  hpgs_matrix total_matrix; /*!< The concatenation of page_matrix and world_matrix. */

  double total_scale; /*!< sqrt(|det(page_matrix)|) */

  int page_mode; /*!< 0...untransformed, 1...fixed page, 2...dynamic page */ 

  double page_width;  /*!< The page width or the maximal page width in points. */
  double page_height; /*!< The page height or the maximal page height in points. */
  double page_angle;  /*!< The rotation angle of the HPGL content on the page. */
  double page_border; /*!< The border of the HPGL border on the page. */

  hpgs_bbox page_bbox; /*!< The currently active page bounding box. */
  hpgs_bbox content_bbox; /*!< The bounding box of the HPGL content of the current page. */

  void *page_asset_ctxt; /*!< A callback for rendering additional page assets before showpage. */
  hpgs_reader_asset_func_t page_asset_func; /*!< A callback for rendering additional page assets before showpage. */

  void *frame_asset_ctxt; /*!< A callback for rendering additional frame assets before frame advance/showpage. */
  hpgs_reader_asset_func_t frame_asset_func; /*!< A callback for rendering additional frame asset before frame advance/showpage. */

  /*@{ */
  /*! linetype settings. (-8,...8) stored from 0...16 */
  int   linetype_nsegs[17];
  float linetype_segs[17][20];
  /*@} */

  /*@{ */
  /*! pen settings */
  int npens;
  double *pen_widths;
  hpgs_color *pen_colors;
  /*@} */

  hpgs_color min_color; /*!< The minimal RGB values. */
  hpgs_color max_color; /*!< The maximal RGB values. */

  /*@{ */
  /*! label terminator settings */
  char label_term;
  int  label_term_ignore;
  /*@} */

  int polygon_mode; /*!< Are we in polygon mode? */

  /*@{ */
  /*! the polygon buffer used in polygon mode. */
  hpgs_reader_poly_point *poly_buffer;
  int poly_buffer_size;
  int poly_buffer_alloc_size;
  /*@} */

  int polygon_open;       /*!< Is a polygon currently open? */
  int pen_width_relative; /*!< Are pen widths specified relative? */
  int pen_down;           /*!< Is the pen down? */
  int current_pen;        /*!< Number of the current pen. */
  int current_linetype;   /*!< Number of the current linetype. */
  int absolute_plotting;  /*!< Are PU and PD coordinates absoulte, because a PA statement is in effect? */
  int have_current_point; /*!< Do we have a current pen position, aka current_point is a valid position. */
  hpgs_point current_point;   /*!< The current pen position in PostScript pt (1/72 inch),
                                    if \c have_current_point is true. */
  hpgs_point first_path_point;/*!< The first point in a path, if \c polygon_open is true. */
  hpgs_point min_path_point;  /*!< The minimal x/y coordinates of all points in an open path. */
  hpgs_point max_path_point;  /*!< The maximal x/y coordinates of all points in an open path. */
  hpgs_point anchor_point;    /*!< The anchor point for fill patterns. */

  int current_ft;    /*!< The fill type currently in effect. */
  double ft3_angle;  /*!< The current pattern angle of fill type 3. */
  double ft3_spacing;/*!< The current pattern spacing of fill type 3. */
  double ft4_angle;  /*!< The current pattern angle of fill type 4. */
  double ft4_spacing;/*!< The current pattern spacing of fill type 4. */
  double ft10_level; /*!< The current color level for fill type 10. */

  /*@{ */
  /*! the text state for the standard font. */
  int default_encoding;
  int default_face;
  int default_spacing;
  double default_pitch;
  double default_height;
  int default_posture;
  int default_weight;
  /*@} */

  /*@{ */
  /*! the text state for the alternate font. */ 
  int alternate_encoding;
  int alternate_face;
  int alternate_spacing;
  double alternate_pitch;
  double alternate_height;
  int alternate_posture;
  int alternate_weight;
  /*@} */

  int alternate_font; /*!< do we use the alternate font? */

  /*@{ */
  /*! text attributes set through special commands. */
  hpgs_point cr_point;
  hpgs_point current_char_size;
  hpgs_point current_extra_space;
  double current_slant;
  int current_label_origin;
  int current_text_path;
  int current_text_line;
  double current_label_angle;
  /*@} */

  double pcl_scale; /*!< The factor from PCL units to PostScript pt (1/72 inch). */
  double pcl_hmi;   /*!< PCL horizontal motion index in pt. */
  double pcl_vmi;   /*!< PCL vertical motion index in pt. */
  hpgs_point pcl_point; /*!< PCL point position in pt. */

  hpgs_reader_pcl_palette *pcl_palettes[HPGS_MAX_PCL_PALETTES]; /*!< The PCL palette stack of palattes fo 256 colors .*/
  int pcl_i_palette;     /*!< The number of of the current PCL palette on the stack. */

  int pcl_raster_mode; /*!< The PCL ratser mode.
                           -1 no raster graphics, 0 horizontal graphics, 3 vertical graphics */
  int pcl_raster_presentation;/*!< The PCL raster presentation mode. */
  int pcl_raster_src_width;   /*!< The PCL raster image source width. */
  int pcl_raster_src_height;  /*!< The PCL raster image source height. */
  int pcl_raster_dest_width;  /*!< The PCL raster image destination width. */
  int pcl_raster_dest_height; /*!< The PCL raster image destination height. */
  int pcl_raster_res;         /*!< The PCL raster image resolution. */
  int pcl_raster_compression; /*!< The PCL raster compression in effect. */
  int pcl_raster_y_offset;    /*!< The PCL raster y offset in effect. */
  int pcl_raster_plane;       /*!< The current PCL raster plane for transfer data by plane. */
  int pcl_raster_line;        /*!< The number of the raster line currently transferred. */

  unsigned char *pcl_raster_data[8];/*!< The buffer for the data of the current raster. One pointer per plane. */
  int pcl_raster_data_size; /*!< The size of the raster data buffer. */
  int pcl_raster_planes;    /*!< The number of planes of the raster data currently being transferred. */

  
  hpgs_image *pcl_image; /*!< The image currently filled by pcl. */

  // The filename for dumped pcl images.
  int png_dump_count;      /*!< The number of PCL images dumped so far. */
  char *png_dump_filename; /*!< The base filename for dumped PCL images. */

  int clipsave_depth; /*!< how many clipsaves have been issued? */

  int last_byte;    /*!< The last byte extracted from the stream? */
  int bytes_ignored;/*!< A byte counter for various purposes. */
  int eoc;          /*!< Did we reach the end of a HPGL command? */
  hpgs_bool interrupted;  /*!< Did someone call \c hpgs_reader_interrupt ? */
};

HPGS_INTERNAL_API int hpgs_reader_check_param_end(hpgs_reader *reader);

/*! read an integer argument from the stream - PCL version.
   return values:
   \li -1 read error/EOF
   \li 0 no integer found.
   \li 1 integer found.
*/
HPGS_INTERNAL_API int hpgs_reader_read_pcl_int(hpgs_reader *reader, int *x, int *sign);

/*! read an integer argument from the stream .
   return values:
   \li -1 read error/EOF
   \li 0 success.
   \li 1 End of command.
*/
HPGS_INTERNAL_API int hpgs_reader_read_int(hpgs_reader *reader, int *x);

/*" read a double argument from the stream.
   return values:
   \li -1 read error/EOF
   \li 0 success.
   \li 1 End of command.
*/
HPGS_INTERNAL_API int hpgs_reader_read_double(hpgs_reader *reader, double *x);
HPGS_INTERNAL_API int hpgs_reader_read_point(hpgs_reader *reader, hpgs_point *p, int xform);

/* read a new string argument from the stream 
   return values:
   \li -1 read error/EOF
   \li 0 success.
   \li 1 End of command.
*/
HPGS_INTERNAL_API int hpgs_reader_read_new_string(hpgs_reader *reader, char *str);
HPGS_INTERNAL_API int hpgs_reader_read_label_string(hpgs_reader *reader, char *str);

HPGS_INTERNAL_API void hpgs_reader_set_page_matrix (hpgs_reader *reader, const hpgs_bbox *bb);
HPGS_INTERNAL_API void hpgs_reader_set_default_transformation (hpgs_reader *reader);
HPGS_INTERNAL_API void hpgs_reader_set_default_state (hpgs_reader *reader);
HPGS_INTERNAL_API void hpgs_reader_set_defaults (hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_set_plotsize (hpgs_reader *reader, double xs, double ys);
HPGS_INTERNAL_API int hpgs_reader_showpage (hpgs_reader *reader, int ipage);

HPGS_INTERNAL_API void hpgs_reader_set_std_pen_colors(hpgs_reader *reader,
                                                      int i0, int n);

HPGS_INTERNAL_API int hpgs_reader_checkpath(hpgs_reader *reader);

HPGS_INTERNAL_API int hpgs_reader_moveto(hpgs_reader *reader, hpgs_point *p);
HPGS_INTERNAL_API int hpgs_reader_lineto(hpgs_reader *reader, hpgs_point *p);
HPGS_INTERNAL_API int hpgs_reader_curveto(hpgs_reader *reader,
                                         hpgs_point *p1, hpgs_point *p2, hpgs_point *p3);
HPGS_INTERNAL_API int hpgs_reader_stroke(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_fill(hpgs_reader *reader, hpgs_bool winding);
HPGS_INTERNAL_API int hpgs_reader_closepath(hpgs_reader *reader);

HPGS_INTERNAL_API int hpgs_reader_do_setpen(hpgs_reader *reader, int pen);

HPGS_INTERNAL_API int hpgs_reader_label(hpgs_reader *reader,
                                        const char *str, int str_len,
                                        int face,
                                        int encoding,
                                        int posture,
                                        int weight,
                                        const hpgs_point *left_vec,
                                        const hpgs_point *up_vec,
                                        const hpgs_point *space_vec);

HPGS_INTERNAL_API int hpgs_device_label(hpgs_device *dev,
                                        hpgs_point *pos,
                                        const char *str, int str_len,
                                        int face,
                                        const char *encoding,
                                        int posture,
                                        int weight,
                                        const hpgs_point *left_vec,
                                        const hpgs_point *up_vec,
                                        const hpgs_point *space_vec);

typedef int (*hpgs_reader_hpglcmd_func_t)(hpgs_reader *reader); 

HPGS_INTERNAL_API int hpgs_reader_do_PCL(hpgs_reader *reader, hpgs_bool take_pos);
HPGS_INTERNAL_API int hpgs_reader_do_PJL(hpgs_reader *reader);

HPGS_INTERNAL_API int hpgs_reader_push_pcl_palette (hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_pop_pcl_palette (hpgs_reader *reader);

HPGS_INTERNAL_API int hpgs_reader_do_AA(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_AC(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_AD(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_AR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_AT(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_BP(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_BR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_BZ(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_CI(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_CO(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_CP(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_CR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_DI(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_DR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_DT(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_DV(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_EA(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_EP(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_ER(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_ES(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_EW(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_FP(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_FR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_FT(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_IN(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_IP(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_IR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_IW(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_LA(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_LB(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_LO(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_LT(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_MC(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_MG(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_NP(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PC(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PA(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PD(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PE(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PG(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PM(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PP(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PS(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PU(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_PW(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_RA(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_RO(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_RR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_RT(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_SA(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_SC(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_SD(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_SI(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_SL(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_SM(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_SP(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_SR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_SS(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_TR(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_UL(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_WG(hpgs_reader *reader);
HPGS_INTERNAL_API int hpgs_reader_do_WU(hpgs_reader *reader);

/*! @} */ /* end of group reader */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // ! __HPGS_READER_H__
