/*
 * "$Id: print-escp2.h,v 1.139 2010/12/19 02:51:37 rlk Exp $"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
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

#ifndef GUTENPRINT_INTERNAL_ESCP2_H
#define GUTENPRINT_INTERNAL_ESCP2_H

/*
 * Maximum number of channels in a printer.  If Epson comes out with an
 * 8-head printer, this needs to be increased.
 */
#define PHYSICAL_CHANNEL_LIMIT 8
#define MAX_DROP_SIZES 3

#define XCOLOR_R     (STP_NCOLORS + 0)
#define XCOLOR_B     (STP_NCOLORS + 1)
#define XCOLOR_GLOSS (STP_NCOLORS + 2)

/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long model_cap_t;
typedef unsigned long model_featureset_t;

/*
 ****************************************************************
 *                                                              *
 * PAPERS                                                       *
 *                                                              *
 ****************************************************************
 */

typedef enum
{
  PAPER_PLAIN         = 0x01,
  PAPER_GOOD          = 0x02,
  PAPER_PHOTO         = 0x04,
  PAPER_PREMIUM_PHOTO = 0x08,
  PAPER_TRANSPARENCY  = 0x10
} paper_class_t;

typedef struct
{
  const char *cname;
  const char *name;
  const char *text;
  paper_class_t paper_class;
  const char *preferred_ink_type;
  const char *preferred_ink_set;
  stp_vars_t *v;
} paper_t;


/*
 ****************************************************************
 *                                                              *
 * RESOLUTIONS                                                  *
 *                                                              *
 ****************************************************************
 */

/* Drop sizes are grouped under resolution because each resolution
   has different drop sizes. */
typedef struct
{
  short numdropsizes;
  double dropsizes[MAX_DROP_SIZES];
} escp2_dropsize_t;

typedef struct
{
  const char *name;
  const char *text;
  short hres;
  short vres;
  short printed_hres;
  short printed_vres;
  short vertical_passes;
  stp_raw_t *command;
  stp_vars_t *v;
} res_t;

typedef struct
{
  char *name;
  res_t *resolutions;
  size_t n_resolutions;
} resolution_list_t;

typedef struct
{
  char *name;
  char *text;
  short min_hres;
  short min_vres;
  short max_hres;
  short max_vres;
  short desired_hres;
  short desired_vres;
} quality_t;

typedef struct
{
  char *name;
  quality_t *qualities;
  size_t n_quals;
} quality_list_t;

typedef struct
{
  char *name;
  char *text;
  stp_raw_t *command;
} printer_weave_t;

typedef struct
{
  char *name;
  size_t n_printer_weaves;
  printer_weave_t *printer_weaves;
} printer_weave_list_t;


/*
 ****************************************************************
 *                                                              *
 * INKS                                                         *
 *                                                              *
 ****************************************************************
 */

typedef struct
{
  short color;
  short subchannel;
  short head_offset;
  short split_channel_count;
  const char *channel_density;
  const char *subchannel_transition;
  const char *subchannel_value;
  const char *subchannel_scale;
  const char *name;
  const char *text;
  short *split_channels;
} physical_subchannel_t;

typedef struct
{
  const char *name;
  short n_subchannels;
  physical_subchannel_t *subchannels;
  const char *hue_curve_name;
  stp_curve_t *hue_curve;
} ink_channel_t;

typedef enum
{
  INKSET_CMYK             = 0,
  INKSET_CcMmYK           = 1,
  INKSET_CcMmYyK          = 2,
  INKSET_CcMmYKk          = 3,
  INKSET_QUADTONE         = 4,
  INKSET_HEXTONE          = 5,
  INKSET_OTHER		  = 6,
  INKSET_EXTENDED	  = 7
} inkset_id_t;

typedef struct
{
  const char *name;
  const char *text;
  short channel_count;
  short aux_channel_count;
  inkset_id_t inkset;
  const stp_raw_t *init_sequence;
  const stp_raw_t *deinit_sequence;
  ink_channel_t *channels;
  ink_channel_t *aux_channels;
} inkname_t;

typedef struct
{
  int n_shades;
  double *shades;
} shade_t;

typedef struct
{
  const char *name;
  const char *text;
  short n_shades;
  short n_inks;
  const stp_raw_t *init_sequence;
  const stp_raw_t *deinit_sequence;
  shade_t *shades;
  inkname_t *inknames;
} inklist_t;

typedef struct
{
  const char *name;
  short n_inklists;
  inklist_t *inklists;
} inkgroup_t;
    

/*
 ****************************************************************
 *                                                              *
 * INPUT SLOTS                                                  *
 *                                                              *
 ****************************************************************
 */

#define ROLL_FEED_CUT_ALL (1)
#define ROLL_FEED_CUT_LAST (2)
#define ROLL_FEED_DONT_EJECT (4)

#define DUPLEX_NO_TUMBLE (1)
#define DUPLEX_TUMBLE (2)

typedef struct
{
  const char *name;
  const char *text;
  short is_cd;
  short is_roll_feed;
  short duplex;
  short extra_height;
  unsigned roll_feed_cut_flags;
  const stp_raw_t *init_sequence;
  const stp_raw_t *deinit_sequence;
} input_slot_t;

/*
 ****************************************************************
 *                                                              *
 * FLAGS                                                        *
 *                                                              *
 ****************************************************************
 */

#define MODEL_COMMAND_MASK	0xful /* What general command set does */
#define MODEL_COMMAND_1998	0x0ul /* Old (ESC .) printers */
#define MODEL_COMMAND_1999	0x1ul /* ESC i printers w/o extended ESC(c */
#define MODEL_COMMAND_2000	0x2ul /* ESC i printers with extended ESC(c */
#define MODEL_COMMAND_PRO	0x3ul /* Stylus Pro printers */

#define MODEL_ZEROMARGIN_MASK	0x70ul /* Does this printer support */
#define MODEL_ZEROMARGIN_NO	0x00ul /* zero margin mode? */
#define MODEL_ZEROMARGIN_YES	0x10ul /* (print beyond bottom of page) */
#define MODEL_ZEROMARGIN_FULL	0x20ul /* (do not print beyond bottom) */
#define MODEL_ZEROMARGIN_RESTR	0x30ul /* (no special treatment for vertical) */
#define MODEL_ZEROMARGIN_H_ONLY	0x40ul /* (borderless only horizontal) */

#define MODEL_VARIABLE_DOT_MASK	0x80ul /* Does this printer support var */
#define MODEL_VARIABLE_NO	0x00ul /* dot size printing? The newest */
#define MODEL_VARIABLE_YES	0x80ul /* printers support multiple modes */

#define MODEL_GRAYMODE_MASK	0x100ul /* Does this printer support special */
#define MODEL_GRAYMODE_NO	0x000ul /* fast black printing? */
#define MODEL_GRAYMODE_YES	0x100ul

#define MODEL_FAST_360_MASK	0x200ul
#define MODEL_FAST_360_NO	0x000ul
#define MODEL_FAST_360_YES	0x200ul

#define MODEL_SEND_ZERO_ADVANCE_MASK	0x400ul
#define MODEL_SEND_ZERO_ADVANCE_NO	0x000ul
#define MODEL_SEND_ZERO_ADVANCE_YES	0x400ul

#define MODEL_SUPPORTS_INK_CHANGE_MASK	0x800ul
#define MODEL_SUPPORTS_INK_CHANGE_NO	0x000ul
#define MODEL_SUPPORTS_INK_CHANGE_YES	0x800ul

#define MODEL_PACKET_MODE_MASK	0x1000ul
#define MODEL_PACKET_MODE_NO	0x0000ul
#define MODEL_PACKET_MODE_YES	0x1000ul

#define MODEL_INTERCHANGEABLE_INK_MASK	0x2000ul
#define MODEL_INTERCHANGEABLE_INK_NO	0x0000ul
#define MODEL_INTERCHANGEABLE_INK_YES	0x2000ul

#define MODEL_ENVELOPE_LANDSCAPE_MASK	0x4000ul
#define MODEL_ENVELOPE_LANDSCAPE_NO	0x0000ul
#define MODEL_ENVELOPE_LANDSCAPE_YES	0x4000ul

typedef enum
{
  MODEL_COMMAND,
  MODEL_ZEROMARGIN,
  MODEL_VARIABLE_DOT,
  MODEL_GRAYMODE,
  MODEL_FAST_360,
  MODEL_SEND_ZERO_ADVANCE,
  MODEL_SUPPORTS_INK_CHANGE,
  MODEL_PACKET_MODE,
  MODEL_INTERCHANGEABLE_INK,
  MODEL_ENVELOPE_LANDSCAPE,
  MODEL_LIMIT
} escp2_model_option_t;

typedef struct escp2_printer
{
  int		active;
/*****************************************************************************/
  model_cap_t	flags;		/* Bitmask of flags, see above */
/*****************************************************************************/
  /* Basic head configuration */
  short		nozzles;	/* Number of nozzles per color */
  short		min_nozzles;	/* Minimum number of nozzles per color */
  short		nozzle_start;	/* Starting usable nozzle */
  short		nozzle_separation; /* Separation between rows, in 1/360" */
  short		black_nozzles;	/* Number of black nozzles (may be extra) */
  short		min_black_nozzles;	/* # of black nozzles (may be extra) */
  short		black_nozzle_start;	/* Starting usable nozzle */
  short		black_nozzle_separation; /* Separation between rows */
  short		fast_nozzles;	/* Number of fast nozzles */
  short		min_fast_nozzles;	/* # of fast nozzles (may be extra) */
  short		fast_nozzle_start;	/* Starting usable nozzle */
  short		fast_nozzle_separation; /* Separation between rows */
  short		physical_channels; /* Number of ink channels */
/*****************************************************************************/
  /* Print head resolution */
  short		base_separation; /* Basic unit of row separation */
  short		resolution_scale;   /* Scaling factor for ESC(D command */
  short		max_black_resolution; /* Above this resolution, we */
				      /* must use color parameters */
				      /* rather than (faster) black */
				      /* only parameters*/
  short		max_hres;
  short		max_vres;
  short		min_hres;
  short		min_vres;
/*****************************************************************************/
  /* Miscellaneous printer-specific data */
  short		extra_feed;	/* Extra distance the paper can be spaced */
				/* beyond the bottom margin, in 1/360". */
				/* (maximum useful value is */
				/* nozzles * nozzle_separation) */
  short		separation_rows; /* Some printers require funky spacing */
				/* arguments in softweave mode. */
  short		pseudo_separation_rows;/* Some printers require funky */
				/* spacing arguments in printer_weave mode */

  short         zero_margin_offset;   /* Offset to use to achieve */
				      /* zero-margin printing */
  short		micro_left_margin; /* Precise left margin (base separation) */
  short		initial_vertical_offset;
  short		black_initial_vertical_offset;
  short		extra_720dpi_separation;
  short		min_horizontal_position_alignment; /* Horizontal alignment */
					       /* for good performance */
  short		base_horizontal_position_alignment; /* Horizontal alignment */
					       /* for good performance */
  int		bidirectional_upper_limit;     /* Highest total resolution */
					       /* for bidirectional printing */
					       /* in auto mode */
/*****************************************************************************/
  /* Paper size limits */
  int		max_paper_width; /* Maximum paper width, in points */
  int		max_paper_height; /* Maximum paper height, in points */
  int		min_paper_width; /* Maximum paper width, in points */
  int		min_paper_height; /* Maximum paper height, in points */
  int		max_imageable_width; /* Maximum imageable area, in points */
  int		max_imageable_height; /* Maximum imageable area, in points */
/*****************************************************************************/
  /* Borders */
				/* SHEET FED: */
				/* Softweave: */
  short		left_margin;	/* Left margin, points */
  short		right_margin;	/* Right margin, points */
  short		top_margin;	/* Absolute top margin, points */
  short		bottom_margin;	/* Absolute bottom margin, points */
				/* Printer weave: */
  short		m_left_margin;	/* Left margin, points */
  short		m_right_margin;	/* Right margin, points */
  short		m_top_margin;	/* Absolute top margin, points */
  short		m_bottom_margin;	/* Absolute bottom margin, points */
				/* ROLL FEED: */
				/* Softweave: */
  short		roll_left_margin;	/* Left margin, points */
  short		roll_right_margin;	/* Right margin, points */
  short		roll_top_margin;	/* Absolute top margin, points */
  short		roll_bottom_margin;	/* Absolute bottom margin, points */
				/* Printer weave: */
  short		m_roll_left_margin;	/* Left margin, points */
  short		m_roll_right_margin;	/* Right margin, points */
  short		m_roll_top_margin;	/* Absolute top margin, points */
  short		m_roll_bottom_margin;	/* Absolute bottom margin, points */
				/* Duplex margin limit (SHRT_MIN = no limit): */
  short		duplex_left_margin;	/* Left margin, points */
  short		duplex_right_margin;	/* Right margin, points */
  short		duplex_top_margin;	/* Absolute top margin, points */
  short		duplex_bottom_margin;	/* Absolute bottom margin, points */
				/* Print directly to CD */
  short		cd_x_offset;	/* Center of CD (horizontal offset) */
  short		cd_y_offset;	/* Center of CD (vertical offset) */
  short		cd_page_width;	/* Width of "page" when printing to CD */
  short		cd_page_height;	/* Height of "page" when printing to CD */
				/* Extra height for form factor command */
  short		paper_extra_bottom; /* Extra space on the bottom of the page */
/*****************************************************************************/
  /* Parameters for escputil */
  short		alignment_passes;
  short		alignment_choices;
  short		alternate_alignment_passes;
  short		alternate_alignment_choices;
/*****************************************************************************/
  stp_raw_t *preinit_sequence;
  stp_raw_t *preinit_remote_sequence;
  stp_raw_t *postinit_sequence;
  stp_raw_t *postinit_remote_sequence;
  stp_raw_t *vertical_borderless_sequence;
/*****************/
  stp_mxml_node_t *media;
  stp_list_t *media_cache;
  stp_string_list_t *papers;
/*****************/
  stp_mxml_node_t *slots;
  stp_list_t *slots_cache;
  stp_string_list_t *input_slots;
/*****************/
  stp_mxml_node_t *media_sizes;
/*****************/
  stp_string_list_t *channel_names;
/*****************/
  resolution_list_t *resolutions;
/*****************/
  printer_weave_list_t *printer_weaves;
/*****************/
  quality_list_t *quality_list;
/*****************/
  inkgroup_t *inkgroup;
} stpi_escp2_printer_t;

/* From escp2-channels.c: */

extern const inkname_t *stpi_escp2_get_default_black_inkset(void);
extern int stp_escp2_load_inkgroup(const stp_vars_t *v, const char *name);

/* From escp2-papers.c: */
extern int stp_escp2_load_media(const stp_vars_t *v, const char *name);
extern int stp_escp2_has_media_feature(const stp_vars_t *v, const char *name);
extern const paper_t *stp_escp2_get_default_media_type(const stp_vars_t *v);
extern const paper_t *stp_escp2_get_media_type(const stp_vars_t *v, int ignore_res);
extern int stp_escp2_printer_supports_rollfeed(const stp_vars_t *v);
extern int stp_escp2_printer_supports_print_to_cd(const stp_vars_t *v);
extern int stp_escp2_printer_supports_duplex(const stp_vars_t *v);

extern int stp_escp2_load_input_slots(const stp_vars_t *v, const char *name);
extern const input_slot_t *stp_escp2_get_input_slot(const stp_vars_t *v);

extern int stp_escp2_load_media_sizes(const stp_vars_t *v, const char *name);
extern void stp_escp2_set_media_size(stp_vars_t *v, const stp_vars_t *src);

/* From escp2-resolutions.c: */
extern int stp_escp2_load_resolutions(const stp_vars_t *v, const char *name);
extern int stp_escp2_load_resolutions_from_xml(const stp_vars_t *v, stp_mxml_node_t *node);
extern int stp_escp2_load_printer_weaves(const stp_vars_t *v, const char *name);
extern int stp_escp2_load_printer_weaves_from_xml(const stp_vars_t *v, stp_mxml_node_t *node);
extern int stp_escp2_load_quality_presets(const stp_vars_t *v, const char *name);
extern int stp_escp2_load_quality_presets_from_xml(const stp_vars_t *v, stp_mxml_node_t *node);

/* From print-escp2.c: */
extern const res_t *stp_escp2_find_resolution(const stp_vars_t *v);
extern const inklist_t *stp_escp2_inklist(const stp_vars_t *v);

/* From print-escp2-data.c: */
extern void stp_escp2_load_model(const stp_vars_t *v, int model);
extern stpi_escp2_printer_t *stp_escp2_get_printer(const stp_vars_t *v);
extern model_featureset_t stp_escp2_get_cap(const stp_vars_t *v,
					    escp2_model_option_t feature);
extern int stp_escp2_has_cap(const stp_vars_t *v, escp2_model_option_t feature,
			     model_featureset_t class);


typedef struct
{
  /* Basic print head parameters */
  int nozzles;			/* Number of nozzles */
  int min_nozzles;		/* Fewest nozzles we're allowed to use */
  int nozzle_separation;	/* Nozzle separation, in dots */
  int nozzle_start;		/* First usable nozzle */
  int *head_offset;		/* Head offset (for C80-type printers) */
  int max_head_offset;		/* Largest head offset */
  int page_management_units;	/* Page management units (dpi) */
  int vertical_units;		/* Vertical units (dpi) */
  int horizontal_units;		/* Horizontal units (dpi) */
  int micro_units;		/* Micro-units for horizontal positioning */
  int unit_scale;		/* Scale factor for units */
  int send_zero_pass_advance;	/* Send explicit command for zero advance */
  int zero_margin_offset;	/* Zero margin offset */
  int split_channel_count;	/* For split channels, like C120 */
  int split_channel_width;	/* Linewidth for split channels */
  short *split_channels;

  /* Ink parameters */
  int bitwidth;			/* Number of bits per ink drop */
  int drop_size;		/* ID of the drop size we're using */
  const inkname_t *inkname;	/* Description of the ink set */
  int use_aux_channels;		/* Use gloss channel */

  /* Ink channels */
  int logical_channels;		/* Number of logical ink channels (e.g.CMYK) */
  int physical_channels;	/* Number of physical channels (e.g. CcMmYK) */
  int channels_in_use;		/* Number of channels we're using
				   FIXME merge with physical_channels! */
  unsigned char **cols;		/* Output dithered data */
  const physical_subchannel_t **channels; /* Description of each channel */

  /* Miscellaneous printer control */
  int use_black_parameters;	/* Can we use (faster) black head parameters */
  int use_fast_360;		/* Can we use fast 360 DPI 4 color mode */
  int advanced_command_set;	/* Uses one of the advanced command sets */
  int use_extended_commands;	/* Do we use the extended commands? */
  const input_slot_t *input_slot; /* Input slot description */
  const paper_t *paper_type;	/* Paper type */
  stp_vars_t *media_settings;	/* Hardware media settings */
  const inkgroup_t *ink_group;	/* Which set of inks */
  const stp_raw_t *preinit_sequence; /* Initialization sequence */
  const stp_raw_t *preinit_remote_sequence; /* Initialization sequence */
  const stp_raw_t *deinit_sequence; /* De-initialization sequence */
  const stp_raw_t *deinit_remote_sequence; /* De-initialization sequence */
  const stp_raw_t *borderless_sequence; /* Vertical borderless sequence */
  model_featureset_t command_set; /* Which command set this printer supports */
  int variable_dots;		/* Print supports variable dot sizes */
  int has_graymode;		/* Printer supports fast grayscale mode */
  int base_separation;		/* Basic unit of separation */
  int resolution_scale;		/* Scale factor for ESC(D command */
  int separation_rows;		/* Row separation scaling */
  int pseudo_separation_rows;	/* Special row separation for some printers */
  int extra_720dpi_separation;	/* Special separation needed at 720 DPI */
  int bidirectional_upper_limit; /* Max total resolution for auto-bidi */
  int duplex;
  int extra_vertical_feed;	/* Extra vertical feed */

  /* weave parameters */
  int horizontal_passes;	/* Number of horizontal passes required
				   to print a complete row */
  int physical_xdpi;		/* Horizontal distance between dots in pass */
  const res_t *res;		/* Description of the printing resolution */
  const stp_raw_t *printer_weave; /* Printer weave parameters */
  int use_printer_weave;	/* Use the printer weaving mechanism */
  int extra_vertical_passes;	/* Quality enhancement */

  /* page parameters */		/* Indexed from top left */
  int page_left;		/* Left edge of page (points) */
  int page_right;		/* Right edge of page (points) */
  int page_top;			/* Top edge of page (points) */
  int page_bottom;		/* Bottom edge of page (points) */
  int page_width;		/* Page width (points) */
  int page_height;		/* Page height (points) */
  int page_true_height;		/* Physical page height (points) */
  int page_extra_height;	/* Extra height for set_form_factor (rows) */
  int paper_extra_bottom;	/* Extra bottom for set_page_size (rows) */
  int page_true_width;		/* Physical page height (points) */
  int cd_x_offset;		/* CD X offset (micro units) */
  int cd_y_offset;		/* CD Y offset (micro units) */
  int cd_outer_radius;		/* CD radius (micro units) */
  int cd_inner_radius;		/* CD radius (micro units) */

  /* Image parameters */	/* Indexed from top left */
  int image_height;		/* Height of printed region (points) */
  int image_width;		/* Width of printed region (points) */
  int image_top;		/* First printed row (points) */
  int image_left;		/* Left edge of image (points) */
  int image_scaled_width;	/* Width of physical printed region (dots) */
  int image_printed_width;	/* Width of printed region (dots) */
  int image_scaled_height;	/* Height of physical printed region (dots) */
  int image_printed_height;	/* Height of printed region (dots) */
  int image_left_position;	/* Left dot position of image */

  /* Transitory state */
  int printed_something;	/* Have we actually printed anything? */
  int initial_vertical_offset;	/* Vertical offset for C80-type printers */
  int printing_initial_vertical_offset;	/* Vertical offset, for print cmd */
  int last_color;		/* Last color we printed */
  int last_pass_offset;		/* Starting row of last pass we printed */
  int last_pass;		/* Last pass printed */
  unsigned char *comp_buf;	/* Compression buffer for C120-type printers */

} escp2_privdata_t;

extern void stpi_escp2_init_printer(stp_vars_t *v);
extern void stpi_escp2_deinit_printer(stp_vars_t *v);
extern void stpi_escp2_flush_pass(stp_vars_t *v, int passno,
				  int vertical_subpass);
extern void stpi_escp2_terminate_page(stp_vars_t *v);

#ifdef TEST_UNCOMPRESSED
#define COMPRESSION (0)
#define FILLFUNC stp_fill_uncompressed
#define COMPUTEFUNC stp_compute_uncompressed_linewidth
#define PACKFUNC stp_pack_uncompressed
#else
#define COMPRESSION (1)
#define FILLFUNC pd->split_channel_count > 0 ? stp_fill_uncompressed : stp_fill_tiff
#define COMPUTEFUNC pd->split_channel_count > 0 ? stp_compute_uncompressed_linewidth : stp_compute_tiff_linewidth
#define PACKFUNC pd->split_channel_count > 0 ? stp_pack_uncompressed : stp_pack_tiff
#endif

#endif /* GUTENPRINT_INTERNAL_ESCP2_H */
/*
 * End of "$Id: print-escp2.h,v 1.139 2010/12/19 02:51:37 rlk Exp $".
 */
