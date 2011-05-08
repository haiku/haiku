/*
 * "$Id: print-escp2.c,v 1.433 2010/12/11 22:04:07 rlk Exp $"
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include <gutenprint/gutenprint-intl-internal.h>
#include "gutenprint-internal.h"
#include <string.h>
#include <math.h>
#include <limits.h>
#include "print-escp2.h"

#ifdef __GNUC__
#define inline __inline__
#endif

#define OP_JOB_START 1
#define OP_JOB_PRINT 2
#define OP_JOB_END   4

#ifndef MAX
#  define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* !MAX */

typedef struct
{
  unsigned count;
  const char *name;
} channel_count_t;

static const channel_count_t escp2_channel_counts[] =
{
  { 1,  "1" },
  { 2,  "2" },
  { 3,  "3" },
  { 4,  "4" },
  { 5,  "5" },
  { 6,  "6" },
  { 7,  "7" },
  { 8,  "8" },
  { 9,  "9" },
  { 10, "10" },
  { 11, "11" },
  { 12, "12" },
  { 13, "13" },
  { 14, "14" },
  { 15, "15" },
  { 16, "16" },
  { 17, "17" },
  { 18, "18" },
  { 19, "19" },
  { 20, "20" },
  { 21, "21" },
  { 22, "22" },
  { 23, "23" },
  { 24, "24" },
  { 25, "25" },
  { 26, "26" },
  { 27, "27" },
  { 28, "28" },
  { 29, "29" },
  { 30, "30" },
  { 31, "31" },
  { 32, "32" },
};

static stp_curve_t *hue_curve_bounds = NULL;
static int escp2_channel_counts_count =
sizeof(escp2_channel_counts) / sizeof(channel_count_t);

static const double ink_darknesses[] =
{
  1.0, 0.31 / .4, 0.61 / .96, 0.08, 0.31 * 0.33 / .4, 0.61 * 0.33 / .96, 0.33, 1.0
};

#define INCH(x)		(72 * x)

#define PARAMETER_INT(s)					\
{								\
  "escp2_" #s, "escp2_" #s,					\
  "Color=Yes,Category=Advanced Printer Functionality", NULL,	\
  STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,		\
  STP_PARAMETER_LEVEL_INTERNAL, 0, 1, STP_CHANNEL_NONE, 1, 0	\
}

#define PARAMETER_INT_RO(s)					\
{								\
  "escp2_" #s, "escp2_" #s,					\
  "Color=Yes,Category=Advanced Printer Functionality", NULL,	\
  STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,		\
  STP_PARAMETER_LEVEL_INTERNAL, 0, 1, STP_CHANNEL_NONE, 1, 1	\
}

#define PARAMETER_RAW(s)					\
{								\
  "escp2_" #s, "escp2_" #s,					\
  "Color=Yes,Category=Advanced Printer Functionality", NULL,	\
  STP_PARAMETER_TYPE_RAW, STP_PARAMETER_CLASS_FEATURE,		\
  STP_PARAMETER_LEVEL_INTERNAL, 0, 1, STP_CHANNEL_NONE, 1, 0	\
}

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  int color_only;
} float_param_t;

typedef struct
{
  const stp_parameter_t param;
  int min;
  int max;
  int defval;
} int_param_t;

static const stp_parameter_t the_parameters[] =
{
#if 0
  {
    "AutoMode", N_("Automatic Printing Mode"), "Color=Yes,Category=Basic Output Adjustment",
    N_("Automatic printing mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
#endif
  /*
   * Don't check this parameter.  We may offer different settings for
   * different papers, but we need to be able to handle settings from PPD
   * files that don't have constraints set up.
   */
  {
    "Quality", N_("Print Quality"), "Color=Yes,Category=Basic Output Adjustment",
    N_("Print Quality"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 0, 0
  },
  {
    "PageSize", N_("Page Size"), "Color=No,Category=Basic Printer Setup",
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MediaType", N_("Media Type"), "Color=Yes,Category=Basic Printer Setup",
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "InputSlot", N_("Media Source"), "Color=No,Category=Basic Printer Setup",
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Duplex", N_("Double-Sided Printing"), "Color=No,Category=Basic Printer Setup",
    N_("Duplex/Tumble Setting"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDInnerRadius", N_("CD Hub Size"), "Color=No,Category=Basic Printer Setup",
    N_("Print only outside of the hub of the CD, or all the way to the hole"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDOuterDiameter", N_("CD Size (Custom)"), "Color=No,Category=Basic Printer Setup",
    N_("Variable adjustment for the outer diameter of CD"),
    STP_PARAMETER_TYPE_DIMENSION, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDInnerDiameter", N_("CD Hub Size (Custom)"), "Color=No,Category=Basic Printer Setup",
    N_("Variable adjustment to the inner hub of the CD"),
    STP_PARAMETER_TYPE_DIMENSION, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDXAdjustment", N_("CD Horizontal Fine Adjustment"), "Color=No,Category=Advanced Printer Setup",
    N_("Fine adjustment to horizontal position for CD printing"),
    STP_PARAMETER_TYPE_DIMENSION, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDYAdjustment", N_("CD Vertical Fine Adjustment"), "Color=No,Category=Advanced Printer Setup",
    N_("Fine adjustment to horizontal position for CD printing"),
    STP_PARAMETER_TYPE_DIMENSION, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDAllowOtherMedia", N_("CD Allow Other Media Sizes"), "Color=No,Category=Advanced Printer Setup",
    N_("Allow non-CD media sizes when printing to CD"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Resolution", N_("Resolution"), "Color=Yes,Category=Basic Printer Setup",
    N_("Resolution of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  /*
   * Don't check this parameter.  We may offer different settings for
   * different ink sets, but we need to be able to handle settings from PPD
   * files that don't have constraints set up.
   */
  {
    "InkType", N_("Ink Type"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED2, 1, 1, STP_CHANNEL_NONE, 0, 0
  },
  {
    "UseGloss", N_("Enhanced Gloss"), "Color=Yes,Category=Basic Printer Setup",
    N_("Add gloss enhancement"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 0, 0
  },
  {
    "InkSet", N_("Ink Set"), "Color=Yes,Category=Basic Printer Setup",
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "PrintingDirection", N_("Printing Direction"), "Color=Yes,Category=Advanced Output Adjustment",
    N_("Printing direction (unidirectional is higher quality, but slower)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "FullBleed", N_("Borderless"), "Color=No,Category=Basic Printer Setup",
    N_("Print without borders"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Weave", N_("Interleave Method"), "Color=Yes,Category=Advanced Output Adjustment",
    N_("Interleave pattern to use"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "OutputOrder", N_("Output Order"), "Color=No,Category=Basic Printer Setup",
    N_("Output Order"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "AlignmentPasses", N_("Alignment Passes"), "Color=No,Category=Advanced Printer Functionality",
    N_("Alignment Passes"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "AlignmentChoices", N_("Alignment Choices"), "Color=No,Category=Advanced Printer Functionality",
    N_("Alignment Choices"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "InkChange", N_("Ink change command"), "Color=No,Category=Advanced Printer Functionality",
    N_("Ink change command"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "AlternateAlignmentPasses", N_("Alternate Alignment Passes"), "Color=No,Category=Advanced Printer Functionality",
    N_("Alternate Alignment Passes"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "AlternateAlignmentChoices", N_("Alternate Alignment Choices"), "Color=No,Category=Advanced Printer Functionality",
    N_("Alternate Alignment Choices"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "SupportsPacketMode", N_("Supports Packet Mode"), "Color=No,Category=Advanced Printer Functionality",
    N_("Supports D4 Packet Mode"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "InterchangeableInk", N_("Has Interchangeable Ink Cartridges"), "Color=No,Category=Advanced Printer Functionality",
    N_("Has multiple choices of ink cartridges"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "InkChannels", N_("Ink Channels"), "Color=No,Category=Advanced Printer Functionality",
    N_("Ink Channels"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "RawChannelNames", N_("Raw Channel Names"), "Color=No,Category=Advanced Printer Functionality",
    N_("Raw Channel Names"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "ChannelNames", N_("Channel Names"), "Color=No,Category=Advanced Printer Functionality",
    N_("Channel Names"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "PrintingMode", N_("Printing Mode"), "Color=Yes,Category=Core Parameter",
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "RawChannels", N_("Raw Channels"), "Color=Yes,Category=Core Parameter",
    N_("Raw Channel Count"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 0, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CyanHueCurve", N_("Cyan Map"), "Color=Yes,Category=Advanced Output Control",
    N_("Adjust the cyan map"),
    STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 1, 1, 0
  },
  {
    "MagentaHueCurve", N_("Magenta Map"), "Color=Yes,Category=Advanced Output Control",
    N_("Adjust the magenta map"),
    STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 2, 1, 0
  },
  {
    "YellowHueCurve", N_("Yellow Map"), "Color=Yes,Category=Advanced Output Control",
    N_("Adjust the yellow map"),
    STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 3, 1, 0
  },
  {
    "BlueHueCurve", N_("Blue Map"), "Color=Yes,Category=Advanced Output Control",
    N_("Adjust the blue map"),
    STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 4, 1, 0
  },
  {
    "OrangeHueCurve", N_("Orange Map"), "Color=Yes,Category=Advanced Output Control",
    N_("Adjust the orange map"),
    STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 4, 1, 0
  },
  {
    "RedHueCurve", N_("Red Map"), "Color=Yes,Category=Advanced Output Control",
    N_("Adjust the red map"),
    STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 5, 1, 0
  },
  PARAMETER_INT(max_hres),
  PARAMETER_INT(max_vres),
  PARAMETER_INT(min_hres),
  PARAMETER_INT(min_vres),
  PARAMETER_INT(nozzles),
  PARAMETER_INT(black_nozzles),
  PARAMETER_INT(fast_nozzles),
  PARAMETER_INT(min_nozzles),
  PARAMETER_INT(min_black_nozzles),
  PARAMETER_INT(min_fast_nozzles),
  PARAMETER_INT(nozzle_start),
  PARAMETER_INT(black_nozzle_start),
  PARAMETER_INT(fast_nozzle_start),
  PARAMETER_INT(nozzle_separation),
  PARAMETER_INT(black_nozzle_separation),
  PARAMETER_INT(fast_nozzle_separation),
  PARAMETER_INT(separation_rows),
  PARAMETER_INT(max_paper_width),
  PARAMETER_INT(max_paper_height),
  PARAMETER_INT(min_paper_width),
  PARAMETER_INT(min_paper_height),
  PARAMETER_INT(max_imageable_width),
  PARAMETER_INT(max_imageable_height),
  PARAMETER_INT(extra_feed),
  PARAMETER_INT(pseudo_separation_rows),
  PARAMETER_INT(base_separation),
  PARAMETER_INT(resolution_scale),
  PARAMETER_INT(initial_vertical_offset),
  PARAMETER_INT(black_initial_vertical_offset),
  PARAMETER_INT(max_black_resolution),
  PARAMETER_INT(zero_margin_offset),
  PARAMETER_INT(extra_720dpi_separation),
  PARAMETER_INT(micro_left_margin),
  PARAMETER_INT(min_horizontal_position_alignment),
  PARAMETER_INT(base_horizontal_position_alignment),
  PARAMETER_INT(bidirectional_upper_limit),
  PARAMETER_INT(physical_channels),
  PARAMETER_INT(left_margin),
  PARAMETER_INT(right_margin),
  PARAMETER_INT(top_margin),
  PARAMETER_INT(bottom_margin),
  PARAMETER_INT(ink_type),
  PARAMETER_INT(bits),
  PARAMETER_INT(base_res),
  PARAMETER_INT_RO(alignment_passes),
  PARAMETER_INT_RO(alignment_choices),
  PARAMETER_INT_RO(alternate_alignment_passes),
  PARAMETER_INT_RO(alternate_alignment_choices),
  PARAMETER_INT(cd_x_offset),
  PARAMETER_INT(cd_y_offset),
  PARAMETER_INT(cd_page_width),
  PARAMETER_INT(cd_page_height),
  PARAMETER_INT(paper_extra_bottom),
  PARAMETER_RAW(preinit_sequence),
  PARAMETER_RAW(preinit_remote_sequence),
  PARAMETER_RAW(postinit_remote_sequence),
  PARAMETER_RAW(vertical_borderless_sequence)
};

static const int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

static const float_param_t float_parameters[] =
{
  {
    {
      "CyanDensity", N_("Cyan Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the cyan density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 1, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "MagentaDensity", N_("Magenta Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the magenta density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 2, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "YellowDensity", N_("Yellow Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the yellow density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 3, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlackDensity", N_("Black Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the black density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 0, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "RedDensity", N_("Red Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the red density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 4, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlueDensity", N_("Blue Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the blue density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 5, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "OrangeDensity", N_("Orange Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the orange density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 5, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "GlossLimit", N_("Gloss Level"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the gloss level"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 6, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "DropSize1", N_("Drop Size Small"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Drop Size 1 (small)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 1.0, 1.0, 1
  },
  {
    {
      "DropSize2", N_("Drop Size Medium"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Drop Size 2 (medium)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 1.0, 0.0, 1
  },
  {
    {
      "DropSize3", N_("Drop Size Large"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Drop Size 3 (large)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 1.0, 0.0, 1
  },
  {
    {
      "LightCyanValue", N_("Light Cyan Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Cyan Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightCyanTrans", N_("Light Cyan Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Cyan Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightCyanScale", N_("Light Cyan Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Cyan Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightMagentaValue", N_("Light Magenta Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Magenta Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightMagentaScale", N_("Light Magenta Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Magenta Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightMagentaTrans", N_("Light Magenta Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Magenta Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "DarkYellowValue", N_("Dark Yellow Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Dark Yellow Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "DarkYellowTrans", N_("Dark Yellow Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Dark Yellow Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "DarkYellowScale", N_("Dark Yellow Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Dark Yellow Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "GrayValue", N_("Gray Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Gray Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "GrayTrans", N_("Gray Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "GrayScale", N_("Gray Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Gray Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "DarkGrayValue", N_("Gray Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Gray Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "DarkGrayTrans", N_("Gray Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "DarkGrayScale", N_("Gray Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Gray Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightGrayValue", N_("Light Gray Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Gray Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightGrayTrans", N_("Light Gray Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightGrayScale", N_("Light Gray Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Gray Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray3Value", N_("Dark Gray Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Dark Gray Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray3Trans", N_("Dark Gray Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Dark Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray3Scale", N_("Dark Gray Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Dark Gray Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray2Value", N_("Mid Gray Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Medium Gray Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray2Trans", N_("Mid Gray Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Medium Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray2Scale", N_("Mid Gray Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Medium Gray Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray1Value", N_("Light Gray Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Gray Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray1Trans", N_("Light Gray Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray1Scale", N_("Light Gray Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Gray Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray5Value", N_("Hextone Gray 5 Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 5 (Darkest) Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray5Trans", N_("Hextone Gray 5 Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 5 (Darkest) Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray5Scale", N_("Hextone Gray 5 Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 5 (Darkest) Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray4Value", N_("Hextone Gray 4 Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 4 Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray4Trans", N_("Hextone Gray 4 Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 4 Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray4Scale", N_("Hextone Gray 4 Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 4 Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray3Value", N_("Hextone Gray 3 Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 3 Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray3Trans", N_("Hextone Gray 3 Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 3 Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray3Scale", N_("Hextone Gray 3 Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 3 Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray2Value", N_("Hextone Gray 2 Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 2 Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray2Trans", N_("Hextone Gray 2 Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 2 Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray2Scale", N_("Hextone Gray 2 Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 2 Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray1Value", N_("Hextone Gray 1 Value"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 1 (Lightest) Value"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray1Trans", N_("Hextone Gray 1 Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 1 (Lightest) Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "HGray1Scale", N_("Hextone Gray 1 Density Scale"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Hextone Gray 1 (Lightest) Density Scale"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "BlackTrans", N_("GCR Transition"), "Color=Yes,Category=Advanced Output Control",
      N_("Adjust the gray component transition rate"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0, 1, 0
    }, 0.0, 1.0, 1.0, 1
  },
  {
    {
      "GCRLower", N_("GCR Lower Bound"), "Color=Yes,Category=Advanced Output Control",
      N_("Lower bound of gray component reduction"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0, 1, 0
    }, 0.0, 1.0, 0.2, 1
  },
  {
    {
      "GCRUpper", N_("GCR Upper Bound"), "Color=Yes,Category=Advanced Output Control",
      N_("Upper bound of gray component reduction"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0, 1, 0
    }, 0.0, 5.0, 0.5, 1
  },
  {
    {
      "PageDryTime", N_("Drying Time Per Page"), "Color=No,Category=Advanced Printer Functionality",
      N_("Set drying time per page"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 60.0, 0.0, 1
  },
  {
    {
      "ScanDryTime", N_("Drying Time Per Scan"), "Color=No,Category=Advanced Printer Functionality",
      N_("Set drying time per scan"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 10.0, 0.0, 1
  },
  {
    {
      "ScanMinDryTime", N_("Minimum Drying Time Per Scan"), "Color=No,Category=Advanced Printer Functionality",
      N_("Set minimum drying time per scan"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 10.0, 0.0, 1
  },
};

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(const float_param_t);


static const int_param_t int_parameters[] =
{
  {
    {
      "BandEnhancement", N_("Quality Enhancement"), "Color=No,Category=Advanced Printer Functionality",
      N_("Enhance print quality by additional passes"),
      STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 4, 0
  },
  {
    {
      "PaperThickness", N_("Paper Thickness"), "Color=No,Category=Advanced Printer Functionality",
      N_("Set printer paper thickness"),
      STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 255, 0
  },
  {
    {
      "VacuumIntensity", N_("Vacuum Intensity"), "Color=No,Category=Advanced Printer Functionality",
      N_("Set vacuum intensity (printer-specific)"),
      STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 255, 0
  },
  {
    {
      "FeedSequence", N_("Feed Sequence"), "Color=No,Category=Advanced Printer Functionality",
      N_("Set paper feed sequence (printer-specific)"),
      STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 255, 0
  },
  {
    {
      "PrintMethod", N_("Print Method"), "Color=No,Category=Advanced Printer Functionality",
      N_("Set print method (printer-specific)"),
      STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 255, 0
  },
  {
    {
      "PlatenGap", N_("Platen Gap"), "Color=No,Category=Advanced Printer Functionality",
      N_("Set platen gap (printer-specific)"),
      STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 255, 0
  },
  {
    {
      "FeedAdjustment", N_("Feed Adjustment"), "Color=No,Category=Advanced Printer Functionality",
      /* xgettext:no-c-format */
      N_("Set paper feed adjustment (0.01% units)"),
      STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0, 255, 0
  },
};

static const int int_parameter_count =
sizeof(int_parameters) / sizeof(const int_param_t);


static escp2_privdata_t *
get_privdata(stp_vars_t *v)
{
  return (escp2_privdata_t *) stp_get_component_data(v, "Driver");
}

#define DEF_SIMPLE_ACCESSOR(f, t)					\
static inline t								\
escp2_##f(const stp_vars_t *v)						\
{									\
  if (stp_check_int_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	\
    return stp_get_int_parameter(v, "escp2_" #f);			\
  else									\
    {									\
      stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);	\
      return printdef->f;						\
    }									\
}

#define DEF_RAW_ACCESSOR(f, t)						\
static inline t								\
escp2_##f(const stp_vars_t *v)						\
{									\
  if (stp_check_raw_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	\
    return stp_get_raw_parameter(v, "escp2_" #f);			\
  else									\
    {									\
      stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);	\
      return printdef->f;						\
    }									\
}

#define DEF_ROLL_ACCESSOR(f, t)						\
static inline t								\
escp2_##f(const stp_vars_t *v, int rollfeed)				\
{									\
  if (stp_check_int_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	\
    return stp_get_int_parameter(v, "escp2_" #f);			\
  else									\
    {									\
      stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);	\
      const res_t *res = stp_escp2_find_resolution(v);			\
      if (res && res->command)						\
	{								\
	  if (rollfeed)							\
	    return (printdef->m_roll_##f);				\
	  else								\
	    return (printdef->m_##f);					\
	}								\
      else								\
	{								\
	  if (rollfeed)							\
	    return (printdef->roll_##f);				\
	  else								\
	    return (printdef->f);					\
	}								\
    }									\
}

DEF_SIMPLE_ACCESSOR(max_hres, int)
DEF_SIMPLE_ACCESSOR(max_vres, int)
DEF_SIMPLE_ACCESSOR(min_hres, int)
DEF_SIMPLE_ACCESSOR(min_vres, int)
DEF_SIMPLE_ACCESSOR(nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(black_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(fast_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(min_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(min_black_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(min_fast_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(nozzle_start, int)
DEF_SIMPLE_ACCESSOR(black_nozzle_start, int)
DEF_SIMPLE_ACCESSOR(fast_nozzle_start, int)
DEF_SIMPLE_ACCESSOR(nozzle_separation, unsigned)
DEF_SIMPLE_ACCESSOR(black_nozzle_separation, unsigned)
DEF_SIMPLE_ACCESSOR(fast_nozzle_separation, unsigned)
DEF_SIMPLE_ACCESSOR(separation_rows, unsigned)
DEF_SIMPLE_ACCESSOR(max_paper_width, unsigned)
DEF_SIMPLE_ACCESSOR(max_paper_height, unsigned)
DEF_SIMPLE_ACCESSOR(min_paper_width, unsigned)
DEF_SIMPLE_ACCESSOR(min_paper_height, unsigned)
DEF_SIMPLE_ACCESSOR(max_imageable_width, unsigned)
DEF_SIMPLE_ACCESSOR(max_imageable_height, unsigned)
DEF_SIMPLE_ACCESSOR(cd_x_offset, int)
DEF_SIMPLE_ACCESSOR(cd_y_offset, int)
DEF_SIMPLE_ACCESSOR(cd_page_width, int)
DEF_SIMPLE_ACCESSOR(cd_page_height, int)
DEF_SIMPLE_ACCESSOR(paper_extra_bottom, int)
DEF_SIMPLE_ACCESSOR(extra_feed, unsigned)
DEF_SIMPLE_ACCESSOR(pseudo_separation_rows, int)
DEF_SIMPLE_ACCESSOR(base_separation, int)
DEF_SIMPLE_ACCESSOR(resolution_scale, int)
DEF_SIMPLE_ACCESSOR(initial_vertical_offset, int)
DEF_SIMPLE_ACCESSOR(black_initial_vertical_offset, int)
DEF_SIMPLE_ACCESSOR(max_black_resolution, int)
DEF_SIMPLE_ACCESSOR(zero_margin_offset, int)
DEF_SIMPLE_ACCESSOR(extra_720dpi_separation, int)
DEF_SIMPLE_ACCESSOR(micro_left_margin, int)
DEF_SIMPLE_ACCESSOR(min_horizontal_position_alignment, unsigned)
DEF_SIMPLE_ACCESSOR(base_horizontal_position_alignment, unsigned)
DEF_SIMPLE_ACCESSOR(bidirectional_upper_limit, int)
DEF_SIMPLE_ACCESSOR(physical_channels, int)
DEF_SIMPLE_ACCESSOR(alignment_passes, int)
DEF_SIMPLE_ACCESSOR(alignment_choices, int)
DEF_SIMPLE_ACCESSOR(alternate_alignment_passes, int)
DEF_SIMPLE_ACCESSOR(alternate_alignment_choices, int)

DEF_ROLL_ACCESSOR(left_margin, unsigned)
DEF_ROLL_ACCESSOR(right_margin, unsigned)
DEF_ROLL_ACCESSOR(top_margin, unsigned)
DEF_ROLL_ACCESSOR(bottom_margin, unsigned)

DEF_RAW_ACCESSOR(preinit_sequence, const stp_raw_t *)
DEF_RAW_ACCESSOR(preinit_remote_sequence, const stp_raw_t *)
DEF_RAW_ACCESSOR(postinit_remote_sequence, const stp_raw_t *)

DEF_RAW_ACCESSOR(vertical_borderless_sequence, const stp_raw_t *)

static const resolution_list_t *
escp2_reslist(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->resolutions;
}

static inline const printer_weave_list_t *
escp2_printer_weaves(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->printer_weaves;
}

static inline const stp_string_list_t *
escp2_channel_names(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return (printdef->channel_names);
}

static inline const inkgroup_t *
escp2_inkgroup(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return (printdef->inkgroup);
}

static inline const quality_list_t *
escp2_quality_list(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->quality_list;
}

static short
escp2_duplex_left_margin(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->duplex_left_margin;
}

static short
escp2_duplex_right_margin(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->duplex_right_margin;
}

static short
escp2_duplex_top_margin(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->duplex_top_margin;
}

static short
escp2_duplex_bottom_margin(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->duplex_bottom_margin;
}

static const channel_count_t *
get_channel_count_by_name(const char *name)
{
  int i;
  for (i = 0; i < escp2_channel_counts_count; i++)
    if (strcmp(name, escp2_channel_counts[i].name) == 0)
      return &(escp2_channel_counts[i]);
  return NULL;
}

static const channel_count_t *
get_channel_count_by_number(unsigned count)
{
  int i;
  for (i = 0; i < escp2_channel_counts_count; i++)
    if (count == escp2_channel_counts[i].count)
      return &(escp2_channel_counts[i]);
  return NULL;
}

static int
escp2_res_param(const stp_vars_t *v, const char *param, const res_t *res)
{
  if (res)
    {
      if (res->v &&
	  stp_check_int_parameter(res->v, param, STP_PARAMETER_ACTIVE))
	return stp_get_int_parameter(res->v, param);
      else
	return -1;
    }
  if (stp_check_int_parameter(v, param, STP_PARAMETER_ACTIVE))
    return stp_get_int_parameter(v, param);
  else
    {
      const res_t *res1 = stp_escp2_find_resolution(v);
      if (res1->v &&
	  stp_check_int_parameter(res1->v, param, STP_PARAMETER_ACTIVE))
	return stp_get_int_parameter(res1->v, param);
    }
  return -1;
}

static int
escp2_ink_type(const stp_vars_t *v)
{
  return escp2_res_param(v, "escp2_ink_type", NULL);
}

static double
escp2_density(const stp_vars_t *v)
{
  if (stp_check_float_parameter(v, "escp2_density", STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, "escp2_density");
  else
    {
      const res_t *res1 = stp_escp2_find_resolution(v);
      if (res1->v &&
	  stp_check_float_parameter(res1->v, "escp2_density", STP_PARAMETER_ACTIVE))
	return stp_get_float_parameter(res1->v, "escp2_density");
    }
  return 0;
}

static int
escp2_bits(const stp_vars_t *v)
{
  return escp2_res_param(v, "escp2_bits", NULL);
}

static int
escp2_base_res(const stp_vars_t *v)
{
  return escp2_res_param(v, "escp2_base_res", NULL);
}

static int
escp2_ink_type_by_res(const stp_vars_t *v, const res_t *res)
{
  return escp2_res_param(v, "escp2_ink_type", res);
}

static double
escp2_density_by_res(const stp_vars_t *v, const res_t *res)
{
  if (res)
    {
      if (res->v &&
	  stp_check_float_parameter(res->v, "escp2_density", STP_PARAMETER_ACTIVE))
	return stp_get_float_parameter(res->v, "escp2_density");
    }
  return 0.0;
}

static int
escp2_bits_by_res(const stp_vars_t *v, const res_t *res)
{
  return escp2_res_param(v, "escp2_bits", res);
}

static int
escp2_base_res_by_res(const stp_vars_t *v, const res_t *res)
{
  return escp2_res_param(v, "escp2_base_res", res);
}

static escp2_dropsize_t *
escp2_copy_dropsizes(const stp_vars_t *v)
{
  const res_t *res = stp_escp2_find_resolution(v);
  escp2_dropsize_t *ndrops;
  if (!res || !(res->v))
    return NULL;
  ndrops = stp_zalloc(sizeof(escp2_dropsize_t));
  if (! ndrops)
    return NULL;
  if (stp_check_float_parameter(res->v, "DropSize1", STP_PARAMETER_ACTIVE))
    {
      ndrops->numdropsizes = 1;
      ndrops->dropsizes[0] = stp_get_float_parameter(res->v, "DropSize1");
    }
  if (stp_check_float_parameter(res->v, "DropSize2", STP_PARAMETER_ACTIVE))
    {
      ndrops->numdropsizes = 2;
      ndrops->dropsizes[1] = stp_get_float_parameter(res->v, "DropSize2");
    }
  if (stp_check_float_parameter(res->v, "DropSize3", STP_PARAMETER_ACTIVE))
    {
      ndrops->numdropsizes = 3;
      ndrops->dropsizes[2] = stp_get_float_parameter(res->v, "DropSize3");
    }
  return ndrops;
}

static void
escp2_free_dropsizes(escp2_dropsize_t *drops)
{
  if (drops)
    stp_free(drops);
}

const inklist_t *
stp_escp2_inklist(const stp_vars_t *v)
{
  int i;
  const char *ink_list_name = NULL;
  const inkgroup_t *inkgroup = escp2_inkgroup(v);

  if (stp_check_string_parameter(v, "InkSet", STP_PARAMETER_ACTIVE))
    ink_list_name = stp_get_string_parameter(v, "InkSet");
  if (ink_list_name)
    {
      for (i = 0; i < inkgroup->n_inklists; i++)
	{
	  if (strcmp(ink_list_name, inkgroup->inklists[i].name) == 0)
	    return &(inkgroup->inklists[i]);
	}
    }
  STPI_ASSERT(inkgroup, v);
  return &(inkgroup->inklists[0]);
}

static const shade_t *
escp2_shades(const stp_vars_t *v, int channel)
{
  const inklist_t *inklist = stp_escp2_inklist(v);
  return &(inklist->shades[channel]);
}

static shade_t *
escp2_copy_shades(const stp_vars_t *v, int channel)
{
  int i;
  shade_t *nshades;
  const inklist_t *inklist = stp_escp2_inklist(v);
  if (!inklist)
    return NULL;
  nshades = stp_zalloc(sizeof(shade_t));
  nshades->n_shades = inklist->shades[channel].n_shades;
  nshades->shades = stp_zalloc(sizeof(double) * inklist->shades[channel].n_shades);
  for (i = 0; i < inklist->shades[channel].n_shades; i++)
    nshades->shades[i] = inklist->shades[channel].shades[i];
  return nshades;
}

static void
escp2_free_shades(shade_t *shades)
{
  if (shades)
    {
      if (shades->shades)
	stp_free(shades->shades);
      stp_free(shades);
    }
}

static const stp_string_list_t *
escp2_paperlist(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->papers;
}

static const stp_string_list_t *
escp2_slotlist(const stp_vars_t *v)
{
  stpi_escp2_printer_t *printdef = stp_escp2_get_printer(v);
  return printdef->input_slots;
}

static int
supports_borderless(const stp_vars_t *v)
{
  return (stp_escp2_has_cap(v, MODEL_ZEROMARGIN, MODEL_ZEROMARGIN_YES) ||
	  stp_escp2_has_cap(v, MODEL_ZEROMARGIN, MODEL_ZEROMARGIN_FULL) ||
	  stp_escp2_has_cap(v, MODEL_ZEROMARGIN, MODEL_ZEROMARGIN_H_ONLY) ||
	  stp_escp2_has_cap(v, MODEL_ZEROMARGIN, MODEL_ZEROMARGIN_RESTR));
}

static int
max_nozzle_span(const stp_vars_t *v)
{
  int nozzle_count = escp2_nozzles(v);
  int nozzle_separation = escp2_nozzle_separation(v);
  int black_nozzle_count = escp2_black_nozzles(v);
  int black_nozzle_separation = escp2_black_nozzle_separation(v);
  int nozzle_span = nozzle_count * nozzle_separation;
  int black_nozzle_span = black_nozzle_count * black_nozzle_separation;
  if (black_nozzle_span > nozzle_span)
    return black_nozzle_span;
  else
    return nozzle_span;
}

static const stp_raw_t *
get_printer_weave(const stp_vars_t *v)
{
  int i;
  const res_t *res = stp_escp2_find_resolution(v);
  const printer_weave_list_t *p = escp2_printer_weaves(v);
  if (p)
    {
      const char *name = stp_get_string_parameter(v, "Weave");
      int printer_weave_count = p->n_printer_weaves;
      if (name)
	{
	  for (i = 0; i < printer_weave_count; i++)
	    {
	      if (!strcmp(name, p->printer_weaves[i].name))
		return p->printer_weaves[i].command;
	    }
	}
    }
  if (res)
    return res->command;
  return NULL;
}

static int
use_printer_weave(const stp_vars_t *v)
{
  const res_t *res = stp_escp2_find_resolution(v);
  return (!res || res->command);
}

static void
get_resolution_bounds_by_paper_type(const stp_vars_t *v,
				    unsigned *max_x, unsigned *max_y,
				    unsigned *min_x, unsigned *min_y)
{
  const paper_t *paper = stp_escp2_get_media_type(v, 1);
  *min_x = 0;
  *min_y = 0;
  *max_x = 0;
  *max_y = 0;
  if (paper)
    {
      switch (paper->paper_class)
	{
	case PAPER_PLAIN:
	  *min_x = 0;
	  *min_y = 0;
	  *max_x = 1440;
	  *max_y = 720;
	  break;
	case PAPER_GOOD:
	  *min_x = 360;
	  *min_y = 360;
	  *max_x = 1440;
	  *max_y = 1440;
	  break;
	case PAPER_PHOTO:
	  *min_x = 720;
	  *min_y = 360;
	  *max_x = 2880;
	  *max_y = 1440;
	  if (*min_x >= escp2_max_hres(v))
	    *min_x = escp2_max_hres(v);
	  break;
	case PAPER_PREMIUM_PHOTO:
	  *min_x = 720;
	  *min_y = 720;
	  *max_x = 0;
	  *max_y = 0;
	  if (*min_x >= escp2_max_hres(v))
	    *min_x = escp2_max_hres(v);
	  break;
	case PAPER_TRANSPARENCY:
	  *min_x = 360;
	  *min_y = 360;
	  *max_x = 720;
	  *max_y = 720;
	  break;
	}
      stp_dprintf(STP_DBG_ESCP2, v,
		  "Paper %s class %d: min_x %d min_y %d max_x %d max_y %d\n",
		  paper->text, paper->paper_class, *min_x, *min_y,
		  *max_x, *max_y);
    }
}

static int
verify_resolution_by_paper_type(const stp_vars_t *v, const res_t *res)
{
  unsigned min_x = 0;
  unsigned min_y = 0;
  unsigned max_x = 0;
  unsigned max_y = 0;
  get_resolution_bounds_by_paper_type(v, &max_x, &max_y, &min_x, &min_y);
  if ((max_x == 0 || res->printed_hres <= max_x) &&
      (max_y == 0 || res->printed_vres <= max_y) &&
      (min_x == 0 || res->printed_hres >= min_x) &&
      (min_y == 0 || res->printed_vres >= min_y))
    {
      stp_dprintf(STP_DBG_ESCP2, v,
		  "Resolution %s (%d, %d) GOOD (%d, %d, %d, %d)\n",
		  res->name, res->printed_hres, res->printed_vres,
		  min_x, min_y, max_x, max_y);
      return 1;
    }
  else
    {
      stp_dprintf(STP_DBG_ESCP2, v,
		  "Resolution %s (%d, %d) BAD (%d, %d, %d, %d)\n",
		  res->name, res->printed_hres, res->printed_vres,
		  min_x, min_y, max_x, max_y);
      return 0;
    }
}

static int
verify_resolution(const stp_vars_t *v, const res_t *res)
{
  int nozzle_width =
    (escp2_base_separation(v) / escp2_nozzle_separation(v));
  int nozzles = escp2_nozzles(v);
  if (escp2_ink_type_by_res(v, res) != -1 &&
      res->vres <= escp2_max_vres(v) &&
      res->hres <= escp2_max_hres(v) &&
      res->vres >= escp2_min_vres(v) &&
      res->hres >= escp2_min_hres(v) &&
      (nozzles == 1 ||
       ((res->vres / nozzle_width) * nozzle_width) == res->vres))
    {
      int xdpi = res->hres;
      int physical_xdpi = escp2_base_res_by_res(v, res);
      int horizontal_passes, oversample;
      if (physical_xdpi > xdpi)
	physical_xdpi = xdpi;
      horizontal_passes = xdpi / physical_xdpi;
      oversample = horizontal_passes * res->vertical_passes;
      if (horizontal_passes < 1)
	horizontal_passes = 1;
      if (oversample < 1)
	oversample = 1;
      if (((horizontal_passes * res->vertical_passes) <= STP_MAX_WEAVE) &&
	  (res->command || (nozzles > 1 && nozzles > oversample)))
	return 1;
    }
  return 0;
}

static void
get_printer_resolution_bounds(const stp_vars_t *v,
			      unsigned *max_x, unsigned *max_y,
			      unsigned *min_x, unsigned *min_y)
{
  int i = 0;
  const resolution_list_t *resolutions = escp2_reslist(v);
  *max_x = 0;
  *max_y = 0;
  *min_x = 0;
  *min_y = 0;
  for (i = 0; i < resolutions->n_resolutions; i++)
    {
      res_t *res = &(resolutions->resolutions[i]);
      if (verify_resolution(v, res))
	{
	  if (res->printed_hres * res->vertical_passes > *max_x)
	    *max_x = res->printed_hres * res->vertical_passes;
	  if (res->printed_vres > *max_y)
	    *max_y = res->printed_vres;
	  if (*min_x == 0 ||
	      res->printed_hres * res->vertical_passes < *min_x)
	    *min_x = res->printed_hres * res->vertical_passes;
	  if (*min_y == 0 || res->printed_vres < *min_y)
	    *min_y = res->printed_vres;
	}
    }
  stp_dprintf(STP_DBG_ESCP2, v,
	      "Printer bounds: %d %d %d %d\n", *min_x, *min_y, *max_x, *max_y);
}

static int
verify_papersize(const stp_vars_t *v, const stp_papersize_t *pt)
{
  unsigned int height_limit, width_limit;
  unsigned int min_height_limit, min_width_limit;
  unsigned int envelope_landscape =
    stp_escp2_has_cap(v, MODEL_ENVELOPE_LANDSCAPE, MODEL_ENVELOPE_LANDSCAPE_YES);
  width_limit = escp2_max_paper_width(v);
  height_limit = escp2_max_paper_height(v);
  min_width_limit = escp2_min_paper_width(v);
  min_height_limit = escp2_min_paper_height(v);
  if (strlen(pt->name) > 0 &&
      (pt->paper_size_type != PAPERSIZE_TYPE_ENVELOPE ||
       envelope_landscape || pt->height > pt->width) &&
      pt->width <= width_limit && pt->height <= height_limit &&
      (pt->height >= min_height_limit || pt->height == 0) &&
      (pt->width >= min_width_limit || pt->width == 0) &&
      (pt->width == 0 || pt->height > 0 ||
       stp_escp2_printer_supports_rollfeed(v)))
    return 1;
  else
    return 0;
}

static int
verify_inktype(const stp_vars_t *v, const inkname_t *inks)
{
  if (inks->inkset == INKSET_EXTENDED)
    return 0;
  else
    return 1;
}

static const char *
get_default_inktype(const stp_vars_t *v)
{
  const inklist_t *ink_list = stp_escp2_inklist(v);
  const paper_t *paper_type;
  if (!ink_list)
    return NULL;
  paper_type = stp_escp2_get_media_type(v, 0);
  if (!paper_type)
    paper_type = stp_escp2_get_default_media_type(v);
  if (paper_type && paper_type->preferred_ink_type)
    return paper_type->preferred_ink_type;
  else if (stp_escp2_has_cap(v, MODEL_FAST_360, MODEL_FAST_360_YES) &&
	   stp_check_string_parameter(v, "Resolution", STP_PARAMETER_ACTIVE))
    {
      const res_t *res = stp_escp2_find_resolution(v);
      if (res)
	{
	  if (res->vres == 360 && res->hres == escp2_base_res(v))
	    {
	      int i;
	      for (i = 0; i < ink_list->n_inks; i++)
		if (strcmp(ink_list->inknames[i].name, "CMYK") == 0)
		  return ink_list->inknames[i].name;
	    }
	}
    }
  return ink_list->inknames[0].name;
}


static const inkname_t *
get_inktype(const stp_vars_t *v)
{
  const char	*ink_type = stp_get_string_parameter(v, "InkType");
  const inklist_t *ink_list = stp_escp2_inklist(v);
  int i;

  if (!ink_type || strcmp(ink_type, "None") == 0 ||
      (ink_list && ink_list->n_inks == 1))
    ink_type = get_default_inktype(v);

  if (ink_type && ink_list)
    {
      for (i = 0; i < ink_list->n_inks; i++)
	{
	  if (strcmp(ink_type, ink_list->inknames[i].name) == 0)
	    return &(ink_list->inknames[i]);
	}
    }
  /*
   * If we couldn't find anything, try again with the default ink type.
   * This may mean duplicate work, but that's cheap enough.
   */
  ink_type = get_default_inktype(v);
  for (i = 0; i < ink_list->n_inks; i++)
    {
      if (strcmp(ink_type, ink_list->inknames[i].name) == 0)
	return &(ink_list->inknames[i]);
    }
  /*
   * If even *that* doesn't work, try using the first ink type on the list.
   */
  return &(ink_list->inknames[0]);
}

static const inkname_t *
get_inktype_only(const stp_vars_t *v)
{
  const char	*ink_type = stp_get_string_parameter(v, "InkType");

  if (!ink_type)
    return NULL;
  else
    return get_inktype(v);
}

static int
printer_supports_inkset(const stp_vars_t *v, inkset_id_t inkset)
{
  const inkgroup_t *ink_group = escp2_inkgroup(v);
  int i;
  for (i = 0; i < ink_group->n_inklists; i++)
    {
      const inklist_t *ink_list = &(ink_group->inklists[i]);
      if (ink_list)
	{
	  int j;
	  for (j = 0; j < ink_list->n_inks; j++)
	    {
	      if (ink_list->inknames[j].inkset == inkset)
		{
		  return 1;
		}
	    }
	}
    }
  return 0;
}

static const stp_vars_t *
get_media_adjustment(const stp_vars_t *v)
{
  const paper_t *pt = stp_escp2_get_media_type(v, 0);
  if (pt)
    return pt->v;
  else
    return NULL;
}

/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
escp2_list_parameters(const stp_vars_t *v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  for (i = 0; i < int_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(int_parameters[i].param));
  return ret;
}

static void
set_density_parameter(const stp_vars_t *v,
		      stp_parameter_t *description,
		      const char *name)
{
  const inkname_t *ink_name = get_inktype(v);
  description->is_active = 0;
  if (ink_name && stp_get_string_parameter(v, "PrintingMode") &&
      strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") != 0)
    {
      int i, j;
      for (i = 0; i < ink_name->channel_count; i++)
	{
	  ink_channel_t *ich = &(ink_name->channels[i]);
	  if (ich)
	    {
	      for (j = 0; j < ich->n_subchannels; j++)
		{
		  physical_subchannel_t *sch = &(ich->subchannels[j]);
		  if (sch && sch->channel_density &&
		      !strcmp(name, sch->channel_density))
		    {
		      description->is_active = 1;
		      description->bounds.dbl.lower = 0;
		      description->bounds.dbl.upper = 2.0;
		      description->deflt.dbl = 1.0;
		    }
		}
	    }
	}
    }
}

static void
set_hue_map_parameter(const stp_vars_t *v,
		      stp_parameter_t *description,
		      const char *name)
{
  const inkname_t *ink_name = get_inktype(v);
  description->is_active = 0;
  description->deflt.curve = hue_curve_bounds;
  description->bounds.curve = stp_curve_create_copy(hue_curve_bounds);
  if (ink_name && stp_get_string_parameter(v, "PrintingMode") &&
      strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") != 0)
    {
      int i;
      for (i = 0; i < ink_name->channel_count; i++)
	{
	  ink_channel_t *ich = &(ink_name->channels[i]);
	  if (ich && ich->hue_curve && !strcmp(name, ich->hue_curve_name))
	    {
	      description->deflt.curve = ich->hue_curve;
	      description->is_active = 1;
	    }
	}
    }
}

static void
fill_value_parameters(const stp_vars_t *v,
		      stp_parameter_t *description,
		      int color)
{
  const shade_t *shades = escp2_shades(v, color);
  const inkname_t *ink_name = get_inktype(v);
  description->is_active = 1;
  description->bounds.dbl.lower = 0;
  description->bounds.dbl.upper = 1.0;
  description->deflt.dbl = 1.0;
  if (shades && ink_name)
    {
      const ink_channel_t *channel = &(ink_name->channels[color]);
      int i;
      for (i = 0; i < channel->n_subchannels; i++)
	{
	  if (channel->subchannels[i].subchannel_value &&
	      strcmp(description->name,
		     channel->subchannels[i].subchannel_value) == 0)
	    {
	      description->deflt.dbl = shades->shades[i];
	      return;
	    }
	}
    }
}

static void
set_color_value_parameter(const stp_vars_t *v,
			  stp_parameter_t *description,
			  int color)
{
  description->is_active = 0;
  if (stp_get_string_parameter(v, "PrintingMode") &&
      strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") != 0)
    {
      const inkname_t *ink_name = get_inktype(v);
      if (ink_name &&
	  ink_name->channel_count == 4 &&
	  ink_name->channels[color].n_subchannels == 2)
	fill_value_parameters(v, description, color);
    }
}

static void
set_gray_value_parameter(const stp_vars_t *v,
			 stp_parameter_t *description,
			 int expected_channels)
{
  const inkname_t *ink_name = get_inktype_only(v);
  description->is_active = 0;
  if (!ink_name &&
      ((expected_channels == 4 && printer_supports_inkset(v, INKSET_QUADTONE)) ||
       (expected_channels == 6 && printer_supports_inkset(v, INKSET_HEXTONE))))
    fill_value_parameters(v, description, STP_ECOLOR_K);
  else if (ink_name && 
      (ink_name->channels[STP_ECOLOR_K].n_subchannels ==
       expected_channels))
    fill_value_parameters(v, description, STP_ECOLOR_K);
  else
    set_color_value_parameter(v, description, STP_ECOLOR_K);
}

static void
fill_transition_parameters(const stp_vars_t *v,
			   stp_parameter_t *description,
			   int color)
{
  const stp_vars_t *paper_adj = get_media_adjustment(v);
  description->is_active = 1;
  description->bounds.dbl.lower = 0;
  description->bounds.dbl.upper = 1.0;
  if (paper_adj && stp_check_float_parameter(paper_adj, "SubchannelCutoff", STP_PARAMETER_ACTIVE))
    description->deflt.dbl = stp_get_float_parameter(paper_adj, "SubchannelCutoff");
  else
    description->deflt.dbl = 1.0;
}

static void
set_color_transition_parameter(const stp_vars_t *v,
			       stp_parameter_t *description,
			       int color)
{
  description->is_active = 0;
  if (stp_get_string_parameter(v, "PrintingMode") &&
      strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") != 0)
    {
      const inkname_t *ink_name = get_inktype(v);
      if (ink_name &&
	  ink_name->channel_count == 4 &&
	  ink_name->channels[color].n_subchannels == 2)
	fill_transition_parameters(v, description, color);
    }
}

static void
set_gray_transition_parameter(const stp_vars_t *v,
			      stp_parameter_t *description,
			      int expected_channels)
{
  const inkname_t *ink_name = get_inktype_only(v);
  description->is_active = 0;
  if (!ink_name &&
      ((expected_channels == 4 && printer_supports_inkset(v, INKSET_QUADTONE)) ||
       (expected_channels == 6 && printer_supports_inkset(v, INKSET_HEXTONE))))
    fill_transition_parameters(v, description, STP_ECOLOR_K);
  if (ink_name && 
      (ink_name->channels[STP_ECOLOR_K].n_subchannels ==
       expected_channels))
    fill_transition_parameters(v, description, STP_ECOLOR_K);
  else
    set_color_transition_parameter(v, description, STP_ECOLOR_K);
}

static void
fill_scale_parameters(stp_parameter_t *description)
{
  description->is_active = 1;
  description->bounds.dbl.lower = 0;
  description->bounds.dbl.upper = 5.0;
  description->deflt.dbl = 1.0;
}

static void
set_color_scale_parameter(const stp_vars_t *v,
			       stp_parameter_t *description,
			       int color)
{
  description->is_active = 0;
  if (stp_get_string_parameter(v, "PrintingMode") &&
      strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") != 0)
    {
      const inkname_t *ink_name = get_inktype(v);
      if (ink_name &&
	  ink_name->channel_count == 4 &&
	  ink_name->channels[color].n_subchannels == 2)
	fill_scale_parameters(description);
    }
}

static void
set_gray_scale_parameter(const stp_vars_t *v,
			      stp_parameter_t *description,
			      int expected_channels)
{
  const inkname_t *ink_name = get_inktype_only(v);
  description->is_active = 0;
  if (!ink_name &&
      ((expected_channels == 4 && printer_supports_inkset(v, INKSET_QUADTONE)) ||
       (expected_channels == 6 && printer_supports_inkset(v, INKSET_HEXTONE))))
    fill_transition_parameters(v, description, STP_ECOLOR_K);
  if (ink_name &&
      (ink_name->channels[STP_ECOLOR_K].n_subchannels ==
       expected_channels))
    fill_scale_parameters(description);
  else
    set_color_scale_parameter(v, description, STP_ECOLOR_K);
}

static const res_t *
find_default_resolution(const stp_vars_t *v, const quality_t *q, int strict)
{
  const resolution_list_t *resolutions = escp2_reslist(v);
  int i = 0;
  stp_dprintf(STP_DBG_ESCP2, v, "Quality %s: min %d %d max %d %d, des %d %d\n",
	      q->name, q->min_hres, q->min_vres, q->max_hres, q->max_vres,
	      q->desired_hres, q->desired_vres);
  if (q->desired_hres < 0 || q->desired_vres < 0)
    {
      for (i = resolutions->n_resolutions - 1; i >= 0; i--)
	{
	  const res_t *res = &(resolutions->resolutions[i]);
	  stp_dprintf(STP_DBG_ESCP2, v, "  Checking resolution %s %d...\n",
		      res->name, i);
	  if ((q->max_hres <= 0 || res->printed_hres <= q->max_hres) &&
	      (q->max_vres <= 0 || res->printed_vres <= q->max_vres) &&
	      q->min_hres <= res->printed_hres &&
	      q->min_vres <= res->printed_vres &&
	      verify_resolution(v, res) &&
	      verify_resolution_by_paper_type(v, res))
	    return res;
	}
    }
  if (!strict)
    {
      unsigned max_x, max_y, min_x, min_y;
      unsigned desired_hres = q->desired_hres;
      unsigned desired_vres = q->desired_vres;
      get_resolution_bounds_by_paper_type(v, &max_x, &max_y, &min_x, &min_y);
      stp_dprintf(STP_DBG_ESCP2, v, "  Comparing hres %d to %d, %d\n",
		  desired_hres, min_x, max_x);
      stp_dprintf(STP_DBG_ESCP2, v, "  Comparing vres %d to %d, %d\n",
		  desired_vres, min_y, max_y);
      if (max_x > 0 && desired_hres > max_x)
	{
	  stp_dprintf(STP_DBG_ESCP2, v, "  Decreasing hres from %d to %d\n",
		      desired_hres, max_x);
	  desired_hres = max_x;
	}
      else if (desired_hres < min_x)
	{
	  stp_dprintf(STP_DBG_ESCP2, v, "  Increasing hres from %d to %d\n",
		      desired_hres, min_x);
	  desired_hres = min_x;
	}
      if (max_y > 0 && desired_vres > max_y)
	{
	  stp_dprintf(STP_DBG_ESCP2, v, "  Decreasing vres from %d to %d\n",
		      desired_vres, max_y);
	  desired_vres = max_y;
	}
      else if (desired_vres < min_y)
	{
	  stp_dprintf(STP_DBG_ESCP2, v, "  Increasing vres from %d to %d\n",
		      desired_vres, min_y);
	  desired_vres = min_y;
	}
      for (i = 0; i < resolutions->n_resolutions; i++)
	{
	  res_t *res = &(resolutions->resolutions[i]);
	  if (verify_resolution(v, res) &&
	      res->printed_vres == desired_vres &&
	      res->printed_hres == desired_hres)
	    {
	      stp_dprintf(STP_DBG_ESCP2, v,
			  "  Found desired resolution w/o oversample: %s %d: %d * %d, %d\n",
			  res->name, i, res->printed_hres,
			  res->vertical_passes, res->printed_vres);
	      return res;
	    }
	}
      for (i = 0; i < resolutions->n_resolutions; i++)
	{
	  res_t *res = &(resolutions->resolutions[i]);
	  if (verify_resolution(v, res) &&
	      res->printed_vres == desired_vres &&
	      res->printed_hres * res->vertical_passes == desired_hres)
	    {
	      stp_dprintf(STP_DBG_ESCP2, v,
			  "  Found desired resolution: %s %d: %d * %d, %d\n",
			  res->name, i, res->printed_hres,
			  res->vertical_passes, res->printed_vres);
	      return res;
	    }
	}
      for (i = 0; i < resolutions->n_resolutions; i++)
	{
	  res_t *res = &(resolutions->resolutions[i]);
	  if (verify_resolution(v, res) &&
	      (q->min_vres == 0 || res->printed_vres >= q->min_vres) &&
	      (q->max_vres == 0 || res->printed_vres <= q->max_vres) &&
	      (q->min_hres == 0 ||
	       res->printed_hres * res->vertical_passes >=q->min_hres) &&
	      (q->max_hres == 0 ||
	       res->printed_hres * res->vertical_passes <= q->max_hres))
	    {
	      stp_dprintf(STP_DBG_ESCP2, v,
			  "  Found acceptable resolution: %s %d: %d * %d, %d\n",
			  res->name, i, res->printed_hres,
			  res->vertical_passes, res->printed_vres);
	      return res;
	    }
	}
    }
#if 0
  if (!strict)			/* Try again to find a match */
    {
      for (i = 0; i < resolutions->n_resolutions; i++)
	{
	  res_t *res = &(resolutions->resolutions[i]);
	  if (verify_resolution(v, res) &&
	      res->printed_vres >= desired_vres &&
	      res->printed_hres * res->vertical_passes >= desired_hres &&
	      res->printed_vres <= 2 * desired_vres &&
	      res->printed_hres * res->vertical_passes <= 2 * desired_hres)
	    return res;
	}
    }
#endif
  return NULL;
}

static int
verify_quality(const stp_vars_t *v, const quality_t *q)
{
  unsigned max_x, max_y, min_x, min_y;
  get_printer_resolution_bounds(v, &max_x, &max_y, &min_x, &min_y);
  if ((q->max_vres == 0 || min_y <= q->max_vres) &&
      (q->min_vres == 0 || max_y >= q->min_vres) &&
      (q->max_hres == 0 || min_x <= q->max_hres) &&
      (q->min_hres == 0 || max_x >= q->min_hres))
    {
      stp_dprintf(STP_DBG_ESCP2, v, "Quality %s OK: %d %d %d %d\n",
		  q->text, q->min_hres, q->min_vres, q->max_hres, q->max_vres);
      return 1;
    }
  else
    {
      stp_dprintf(STP_DBG_ESCP2, v, "Quality %s not OK: %d %d %d %d\n",
		  q->text, q->min_hres, q->min_vres, q->max_hres, q->max_vres);
      return 0;
    }
}

static const res_t *
find_resolution_from_quality(const stp_vars_t *v, const char *quality,
			     int strict)
{
  int i;
  const quality_list_t *quals = escp2_quality_list(v);
  /* This is a rather gross hack... */
  if (strcmp(quality, "None") == 0)
    quality = "Standard";
  for (i = 0; i < quals->n_quals; i++)
    {
      const quality_t *q = &(quals->qualities[i]);
      if (strcmp(quality, q->name) == 0 && verify_quality(v, q))
	return find_default_resolution(v, q, strict);
    }
  return NULL;
}

static const inkname_t *
get_raw_inktype(const stp_vars_t *v)
{
  if (strcmp(stp_get_string_parameter(v, "InputImageType"), "Raw") == 0)
    {
      const inklist_t *inks = stp_escp2_inklist(v);
      int ninktypes = inks->n_inks;
      int i;
      const char *channel_name = stp_get_string_parameter(v, "RawChannels");
      const channel_count_t *count;
      if (!channel_name)
	goto none;
      count = get_channel_count_by_name(channel_name);
      if (!count)
	goto none;
      for (i = 0; i < ninktypes; i++)
	if (inks->inknames[i].inkset == INKSET_EXTENDED &&
	    (inks->inknames[i].channel_count == count->count))
	  return &(inks->inknames[i]);
    }
 none:
  return get_inktype(v);
}

static void
escp2_parameters(const stp_vars_t *v, const char *name,
		 stp_parameter_t *description)
{
  int		i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  for (i = 0; i < float_parameter_count; i++)
    if (strcmp(name, float_parameters[i].param.name) == 0)
      {
	stp_fill_parameter_settings(description,
				     &(float_parameters[i].param));
	description->deflt.dbl = float_parameters[i].defval;
	description->bounds.dbl.upper = float_parameters[i].max;
	description->bounds.dbl.lower = float_parameters[i].min;
	break;
      }
  for (i = 0; i < int_parameter_count; i++)
    if (strcmp(name, int_parameters[i].param.name) == 0)
      {
	stp_fill_parameter_settings(description,
				     &(int_parameters[i].param));
	description->deflt.integer = int_parameters[i].defval;
	description->bounds.integer.upper = int_parameters[i].max;
	description->bounds.integer.lower = int_parameters[i].min;
	break;
      }

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	if (description->p_type == STP_PARAMETER_TYPE_INT)
	  {
	    description->deflt.integer = 0;
	    description->bounds.integer.upper = INT_MAX;
	    description->bounds.integer.lower = INT_MIN;
	  }
	break;
      }

  description->deflt.str = NULL;
  if (strcmp(name, "AutoMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string(description->bounds.str, "None",
				 _("Full Manual Control"));
      stp_string_list_add_string(description->bounds.str, "Auto",
				 _("Automatic Setting Control"));
      description->deflt.str = "None"; /* so CUPS and Foomatic don't break */
    }
  else if (strcmp(name, "PageSize") == 0)
    {
      int papersizes = stp_known_papersizes();
      const input_slot_t *slot = stp_escp2_get_input_slot(v);
      description->bounds.str = stp_string_list_create();
      if (slot && slot->is_cd &&
	  !stp_get_boolean_parameter(v, "CDAllowOtherMedia"))
	{
	  stp_string_list_add_string
	    (description->bounds.str, "CD5Inch", _("CD - 5 inch"));
	  stp_string_list_add_string
	    (description->bounds.str, "CD3Inch", _("CD - 3 inch"));
	  stp_string_list_add_string
	    (description->bounds.str, "CDCustom", _("CD - Custom"));
	}
      else
	{
	  for (i = 0; i < papersizes; i++)
	    {
	      const stp_papersize_t *pt = stp_get_papersize_by_index(i);
	      if (verify_papersize(v, pt))
		stp_string_list_add_string(description->bounds.str,
					   pt->name, gettext(pt->text));
	    }
	}
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "CDAllowOtherMedia") == 0)
    {
      const input_slot_t *slot = stp_escp2_get_input_slot(v);
      if (stp_escp2_printer_supports_print_to_cd(v) &&
	  (!slot || slot->is_cd))
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "CDInnerRadius") == 0 )
    {
      const input_slot_t *slot = stp_escp2_get_input_slot(v);
      description->bounds.str = stp_string_list_create();
      if (stp_escp2_printer_supports_print_to_cd(v) &&
	  (!slot || slot->is_cd) &&
	  (!stp_get_string_parameter(v, "PageSize") ||
	   strcmp(stp_get_string_parameter(v, "PageSize"), "CDCustom") != 0))
	{
	  stp_string_list_add_string
	    (description->bounds.str, "None", _("Normal"));
	  stp_string_list_add_string
	    (description->bounds.str, "Small", _("Print To Hub"));
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "CDInnerDiameter") == 0 )
    {
      const input_slot_t *slot = stp_escp2_get_input_slot(v);
      description->bounds.dimension.lower = 16 * 10 * 72 / 254;
      description->bounds.dimension.upper = 43 * 10 * 72 / 254;
      description->deflt.dimension = 43 * 10 * 72 / 254;
      if (stp_escp2_printer_supports_print_to_cd(v) &&
	  (!slot || slot->is_cd) &&
	  (!stp_get_string_parameter(v, "PageSize") ||
	   strcmp(stp_get_string_parameter(v, "PageSize"), "CDCustom") == 0))
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "CDOuterDiameter") == 0 )
    {
      const input_slot_t *slot = stp_escp2_get_input_slot(v);
      description->bounds.dimension.lower = 65 * 10 * 72 / 254;
      description->bounds.dimension.upper = 120 * 10 * 72 / 254;
      description->deflt.dimension = 329;
      if (stp_escp2_printer_supports_print_to_cd(v) &&
	  (!slot || slot->is_cd) &&
	  (!stp_get_string_parameter(v, "PageSize") ||
	   strcmp(stp_get_string_parameter(v, "PageSize"), "CDCustom") == 0))
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "CDXAdjustment") == 0 ||
	   strcmp(name, "CDYAdjustment") == 0)
    {
      const input_slot_t *slot = stp_escp2_get_input_slot(v);
      description->bounds.dimension.lower = -15;
      description->bounds.dimension.upper = 15;
      description->deflt.dimension = 0;
      if (stp_escp2_printer_supports_print_to_cd(v) && (!slot || slot->is_cd))
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "Quality") == 0)
    {
      const quality_list_t *quals = escp2_quality_list(v);
      int has_standard_quality = 0;
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string(description->bounds.str, "None",
				 _("Manual Control"));
      for (i = 0; i < quals->n_quals; i++)
	{
	  const quality_t *q = &(quals->qualities[i]);
	  if (verify_quality(v, q))
	    stp_string_list_add_string(description->bounds.str, q->name,
				       gettext(q->text));
	  if (strcmp(q->name, "Standard") == 0)
	    has_standard_quality = 1;
	}
      if (has_standard_quality)
	description->deflt.str = "Standard";
      else
	description->deflt.str = "None";
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      const resolution_list_t *resolutions = escp2_reslist(v);
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string(description->bounds.str, "None",
				 _("Default"));
      description->deflt.str = "None";
      for (i = 0; i < resolutions->n_resolutions; i++)
	{
	  res_t *res = &(resolutions->resolutions[i]);
	  if (verify_resolution(v, res))
	    stp_string_list_add_string(description->bounds.str,
				       res->name, gettext(res->text));
	}
    }
  else if (strcmp(name, "InkType") == 0)
    {
      const inklist_t *inks = stp_escp2_inklist(v);
      int ninktypes = inks->n_inks;
      int verified_inktypes = 0;
      for (i = 0; i < ninktypes; i++)
	if (verify_inktype(v, &(inks->inknames[i])))
	  verified_inktypes++;
      description->bounds.str = stp_string_list_create();
      if (verified_inktypes > 1)
	{
	  stp_string_list_add_string(description->bounds.str, "None",
				     _("Standard"));
	  for (i = 0; i < ninktypes; i++)
	    if (verify_inktype(v, &(inks->inknames[i])))
	      stp_string_list_add_string(description->bounds.str,
					 inks->inknames[i].name,
					 gettext(inks->inknames[i].text));
	  description->deflt.str = "None";
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "InkSet") == 0)
    {
      const inkgroup_t *inks = escp2_inkgroup(v);
      int ninklists = inks->n_inklists;
      description->bounds.str = stp_string_list_create();
      if (ninklists > 1)
	{
	  int has_default_choice = 0;
	  for (i = 0; i < ninklists; i++)
	    {
	      stp_string_list_add_string(description->bounds.str,
					 inks->inklists[i].name,
					 gettext(inks->inklists[i].text));
	      if (strcmp(inks->inklists[i].name, "None") == 0)
		has_default_choice = 1;
	    }
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      const stp_string_list_t *p = escp2_paperlist(v);
      description->is_active = 0;
      if (p)
	{
	  int nmediatypes = stp_string_list_count(p);
	  description->bounds.str = stp_string_list_create();
	  if (nmediatypes)
	    {
	      description->is_active = 1;
	      for (i = 0; i < nmediatypes; i++)
		stp_string_list_add_string(description->bounds.str,
					   stp_string_list_param(p, i)->name,
					   gettext(stp_string_list_param(p, i)->text));
	      description->deflt.str =
		stp_string_list_param(description->bounds.str, 0)->name;
	    }
	}
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      const stp_string_list_t *p = escp2_slotlist(v);
      description->is_active = 0;
      if (p)
	{
	  int nslots = stp_string_list_count(p);
	  description->bounds.str = stp_string_list_create();
	  if (nslots)
	    {
	      description->is_active = 1;
	      for (i = 0; i < nslots; i++)
		stp_string_list_add_string(description->bounds.str,
					   stp_string_list_param(p, i)->name,
					   gettext(stp_string_list_param(p, i)->text));
	      description->deflt.str =
		stp_string_list_param(description->bounds.str, 0)->name;
	    }
	}
    }
  else if (strcmp(name, "PrintingDirection") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string
	(description->bounds.str, "None", _("Automatic"));
      stp_string_list_add_string
	(description->bounds.str, "Bidirectional", _("Bidirectional"));
      stp_string_list_add_string
	(description->bounds.str, "Unidirectional", _("Unidirectional"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "Weave") == 0)
    {
      description->bounds.str = stp_string_list_create();
      if (stp_escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO))
	{
	  const res_t *res = stp_escp2_find_resolution(v);
	  const printer_weave_list_t *printer_weaves = escp2_printer_weaves(v);
	  int nprinter_weaves = 0;
	  if (printer_weaves && use_printer_weave(v) && (!res || res->command))
	    nprinter_weaves = printer_weaves->n_printer_weaves;
	  if (nprinter_weaves)
	    {
	      stp_string_list_add_string(description->bounds.str, "None",
					 _("Standard"));
	      for (i = 0; i < nprinter_weaves; i++)
		stp_string_list_add_string(description->bounds.str,
					   printer_weaves->printer_weaves[i].name,
					   gettext(printer_weaves->printer_weaves[i].text));
	    }
	  else
	    description->is_active = 0;
	}
      else
	{
	  stp_string_list_add_string
	    (description->bounds.str, "None", _("Standard"));
	  stp_string_list_add_string
	    (description->bounds.str, "Alternate", _("Alternate Fill"));
	  stp_string_list_add_string
	    (description->bounds.str, "Ascending", _("Ascending Fill"));
	  stp_string_list_add_string
	    (description->bounds.str, "Descending", _("Descending Fill"));
	  stp_string_list_add_string
	    (description->bounds.str, "Ascending2X", _("Ascending Double"));
	  stp_string_list_add_string
	    (description->bounds.str, "Staggered", _("Nearest Neighbor Avoidance"));
	}
      if (description->is_active)
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "OutputOrder") == 0)
    {
      description->bounds.str = stp_string_list_create();
      description->deflt.str = "Reverse";
    }
  else if (strcmp(name, "FullBleed") == 0)
    {
      const input_slot_t *slot = stp_escp2_get_input_slot(v);
      if (slot && slot->is_cd)
	description->is_active = 0;
      else if (supports_borderless(v))
	description->deflt.boolean = 0;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "Duplex") == 0)
    {
      if (stp_escp2_printer_supports_duplex(v))
	{
	  const input_slot_t *slot = stp_escp2_get_input_slot(v);
	  if (slot && !slot->duplex)
	    description->is_active = 0;
	  else
	    {
	      description->bounds.str = stp_string_list_create();
	      stp_string_list_add_string
		(description->bounds.str, "None", _("Off"));
	      stp_string_list_add_string
		(description->bounds.str, "DuplexNoTumble", _("Long Edge (Standard)"));
	      stp_string_list_add_string
		(description->bounds.str, "DuplexTumble", _("Short Edge(Flip)"));
	      description->deflt.str = "None";
	    }
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "CyanDensity") == 0 ||
	   strcmp(name, "MagentaDensity") == 0 ||
	   strcmp(name, "YellowDensity") == 0 ||
	   strcmp(name, "BlackDensity") == 0 ||
	   strcmp(name, "RedDensity") == 0 ||
	   strcmp(name, "BlueDensity") == 0 ||
	   strcmp(name, "OrangeDensity") == 0)
    set_density_parameter(v, description, name);
  else if (strcmp(name, "CyanHueCurve") == 0 ||
	   strcmp(name, "MagentaHueCurve") == 0 ||
	   strcmp(name, "YellowHueCurve") == 0 ||
	   strcmp(name, "RedHueCurve") == 0 ||
	   strcmp(name, "BlueHueCurve") == 0 ||
	   strcmp(name, "OrangeHueCurve") == 0)
    set_hue_map_parameter(v, description, name);
  else if (strcmp(name, "UseGloss") == 0)
    {
      const inkname_t *ink_name = get_inktype(v);
      if (ink_name && ink_name->aux_channel_count > 0)
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "GlossLimit") == 0)
    {
      const inkname_t *ink_name = get_inktype(v);
      if (ink_name && ink_name->aux_channel_count > 0)
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "DropSize1") == 0 ||
	   strcmp(name, "DropSize2") == 0 ||
	   strcmp(name, "DropSize3") == 0)
    {
      if (stp_escp2_has_cap(v, MODEL_VARIABLE_DOT, MODEL_VARIABLE_YES))
	{
	  const res_t *res = stp_escp2_find_resolution(v);
	  if (res && res->v &&
	      stp_check_float_parameter(v, name, STP_PARAMETER_ACTIVE))
	    description->deflt.dbl = stp_get_float_parameter(v, name);
	  description->is_active = 1;
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "BlackTrans") == 0 ||
	   strcmp(name, "GCRLower") == 0 ||
	   strcmp(name, "GCRUpper") == 0)
    {
      const stp_vars_t *paper_adj = get_media_adjustment(v);
      if (paper_adj &&
	  stp_get_string_parameter(v, "PrintingMode") &&
	  strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") != 0)
	{
	  if (paper_adj && stp_check_float_parameter(paper_adj, name, STP_PARAMETER_ACTIVE))
	    description->deflt.dbl = stp_get_float_parameter(paper_adj, name);
	  else
	    description->p_type = STP_PARAMETER_TYPE_INVALID;
	}
      else
	description->p_type = STP_PARAMETER_TYPE_INVALID;
    }
  else if (strcmp(name, "GrayValue") == 0)
    set_gray_value_parameter(v, description, 2);
  else if (strcmp(name, "DarkGrayValue") == 0 ||
	   strcmp(name, "LightGrayValue") == 0)
    set_gray_value_parameter(v, description, 3);
  else if (strcmp(name, "Gray1Value") == 0 ||
	   strcmp(name, "Gray2Value") == 0 ||
	   strcmp(name, "Gray3Value") == 0)
    set_gray_value_parameter(v, description, 4);
  else if (strcmp(name, "LightCyanValue") == 0)
    set_color_value_parameter(v, description, STP_ECOLOR_C);
  else if (strcmp(name, "LightMagentaValue") == 0)
    set_color_value_parameter(v, description, STP_ECOLOR_M);
  else if (strcmp(name, "DarkYellowValue") == 0)
    set_color_value_parameter(v, description, STP_ECOLOR_Y);
  else if (strcmp(name, "GrayTrans") == 0)
    set_gray_transition_parameter(v, description, 2);
  else if (strcmp(name, "DarkGrayTrans") == 0 ||
	   strcmp(name, "LightGrayTrans") == 0)
    set_gray_transition_parameter(v, description, 3);
  else if (strcmp(name, "Gray1Trans") == 0 ||
	   strcmp(name, "Gray2Trans") == 0 ||
	   strcmp(name, "Gray3Trans") == 0)
    set_gray_transition_parameter(v, description, 4);
  else if (strcmp(name, "LightCyanTrans") == 0)
    set_color_transition_parameter(v, description, STP_ECOLOR_C);
  else if (strcmp(name, "LightMagentaTrans") == 0)
    set_color_transition_parameter(v, description, STP_ECOLOR_M);
  else if (strcmp(name, "DarkYellowTrans") == 0)
    set_color_transition_parameter(v, description, STP_ECOLOR_Y);
  else if (strcmp(name, "GrayScale") == 0)
    set_gray_scale_parameter(v, description, 2);
  else if (strcmp(name, "DarkGrayScale") == 0 ||
	   strcmp(name, "LightGrayScale") == 0)
    set_gray_scale_parameter(v, description, 3);
  else if (strcmp(name, "Gray1Scale") == 0 ||
	   strcmp(name, "Gray2Scale") == 0 ||
	   strcmp(name, "Gray3Scale") == 0)
    set_gray_scale_parameter(v, description, 4);
  else if (strcmp(name, "LightCyanScale") == 0)
    set_color_scale_parameter(v, description, STP_ECOLOR_C);
  else if (strcmp(name, "LightMagentaScale") == 0)
    set_color_scale_parameter(v, description, STP_ECOLOR_M);
  else if (strcmp(name, "DarkYellowScale") == 0)
    set_color_scale_parameter(v, description, STP_ECOLOR_Y);
  else if (strcmp(name, "AlignmentPasses") == 0)
    {
      description->deflt.integer = escp2_alignment_passes(v);
    }
  else if (strcmp(name, "AlignmentChoices") == 0)
    {
      description->deflt.integer = escp2_alignment_choices(v);
    }
  else if (strcmp(name, "SupportsInkChange") == 0)
    {
      description->deflt.integer =
	stp_escp2_has_cap(v, MODEL_SUPPORTS_INK_CHANGE,
		      MODEL_SUPPORTS_INK_CHANGE_YES);
    }
  else if (strcmp(name, "AlternateAlignmentPasses") == 0)
    {
      description->deflt.integer = escp2_alternate_alignment_passes(v);
    }
  else if (strcmp(name, "AlternateAlignmentChoices") == 0)
    {
      description->deflt.integer = escp2_alternate_alignment_choices(v);
    }
  else if (strcmp(name, "InkChannels") == 0)
    {
      description->deflt.integer = escp2_physical_channels(v);
    }
  else if (strcmp(name, "ChannelNames") == 0)
    {
      const stp_string_list_t *channel_names = escp2_channel_names(v);
      if (channel_names)
	{
	  description->bounds.str = stp_string_list_create_copy(channel_names);
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	}
      else
	description->p_type = STP_PARAMETER_TYPE_INVALID;
    }
  else if (strcmp(name, "SupportsPacketMode") == 0)
    {
      description->deflt.boolean =
	stp_escp2_has_cap(v, MODEL_PACKET_MODE, MODEL_PACKET_MODE_YES);
    }
  else if (strcmp(name, "PrintingMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string
	(description->bounds.str, "Color", _("Color"));
      stp_string_list_add_string
	(description->bounds.str, "BW", _("Black and White"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "RawChannels") == 0)
    {
      const inklist_t *inks = stp_escp2_inklist(v);
      int ninktypes = inks->n_inks;
      description->bounds.str = stp_string_list_create();
      if (ninktypes >= 1)
	{
	  stp_string_list_add_string(description->bounds.str, "None", "None");
	  for (i = 0; i < ninktypes; i++)
	    if (inks->inknames[i].inkset == INKSET_EXTENDED)
	      {
		const channel_count_t *ch =
		  (get_channel_count_by_number
		   (inks->inknames[i].channel_count));
		stp_string_list_add_string(description->bounds.str,
					   ch->name, ch->name);
	      }
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "RawChannelNames") == 0)
    {
      const inkname_t *ink_name = get_raw_inktype(v);
      if (ink_name)
	{
	  description->bounds.str = stp_string_list_create();
	  for (i = 0; i < ink_name->channel_count; i++)
	    {
	      int j;
	      const ink_channel_t *ic = &(ink_name->channels[i]);
	      if (ic)
		for (j = 0; j < ic->n_subchannels; j++)
		  if (ic->subchannels[j].name)
		    stp_string_list_add_string(description->bounds.str,
					       ic->subchannels[j].name,
					       gettext(ic->subchannels[j].text));
	    }
	  for (i = 0; i < ink_name->aux_channel_count; i++)
	    {
	      int j;
	      const ink_channel_t *ic = &(ink_name->aux_channels[i]);
	      if (ic)
		for (j = 0; j < ic->n_subchannels; j++)
		  if (ic->subchannels[j].name)
		    stp_string_list_add_string(description->bounds.str,
					       ic->subchannels[j].name,
					       gettext(ic->subchannels[j].text));
	    }
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	}
    }
  else if (strcmp(name, "MultiChannelLimit") == 0)
    {
      description->is_active = 0;
      if (stp_get_string_parameter(v, "PrintingMode") &&
	  strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") != 0)
	{
	  const inkname_t *ink_name = get_inktype(v);
	  if (ink_name && ink_name->inkset == INKSET_OTHER)
	    description->is_active = 1;
	}
    }
  else if (strcmp(name, "PageDryTime") == 0 ||
	   strcmp(name, "ScanDryTime") == 0 ||
	   strcmp(name, "ScanMinDryTime") == 0 ||
	   strcmp(name, "FeedAdjustment") == 0 ||
	   strcmp(name, "PaperThickness") == 0 ||
	   strcmp(name, "VacuumIntensity") == 0 ||
	   strcmp(name, "FeedSequence") == 0 ||
	   strcmp(name, "PrintMethod") == 0 ||
	   strcmp(name, "PaperMedia") == 0 ||
	   strcmp(name, "PaperMediaSize") == 0 ||
	   strcmp(name, "PlatenGap") == 0)
    {
      description->is_active = 0;
      if (stp_escp2_has_media_feature(v, name))
	description->is_active = 1;
    }
  else if (strcmp(name, "BandEnhancement") == 0)
    {
      description->is_active = 1;
    }
}

const res_t *
stp_escp2_find_resolution(const stp_vars_t *v)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  if (resolution)
    {
      const resolution_list_t *resolutions = escp2_reslist(v);
      int i;
      for (i = 0; i < resolutions->n_resolutions; i++)
	{
	  const res_t *res = &(resolutions->resolutions[i]);
	  if (!strcmp(resolution, res->name))
	    return res;
	  else if (!strcmp(res->name, ""))
	    return NULL;
	}
    }
  if (stp_check_string_parameter(v, "Quality", STP_PARAMETER_ACTIVE))
    {
      const res_t *default_res =
	find_resolution_from_quality(v, stp_get_string_parameter(v, "Quality"),
				     0);
      if (default_res)
	{
	  stp_dprintf(STP_DBG_ESCP2, v,
		      "Setting resolution to %s from quality %s\n",
		      default_res->name,
		      stp_get_string_parameter(v, "Quality"));
	  return default_res;
	}
      else
	stp_dprintf(STP_DBG_ESCP2, v, "Unable to map quality %s\n",
		    stp_get_string_parameter(v, "Quality"));
    }
  return NULL;
}

static inline int
imax(int a, int b)
{
  if (a > b)
    return a;
  else
    return b;
}

static void
escp2_media_size(const stp_vars_t *v,	/* I */
		 int  *width,		/* O - Width in points */
		 int  *height) 		/* O - Height in points */
{
  if (stp_get_page_width(v) > 0 && stp_get_page_height(v) > 0)
    {
      *width = stp_get_page_width(v);
      *height = stp_get_page_height(v);
    }
  else
    {
      const char *page_size = stp_get_string_parameter(v, "PageSize");
      const stp_papersize_t *papersize = NULL;
      if (page_size)
	papersize = stp_get_papersize_by_name(page_size);
      if (!papersize)
	{
	  *width = 1;
	  *height = 1;
	}
      else
	{
	  *width = papersize->width;
	  *height = papersize->height;
	}
      if (*width == 0 || *height == 0)
	{
	  const input_slot_t *slot = stp_escp2_get_input_slot(v);
	  if (slot && slot->is_cd)
	    {
	      papersize = stp_get_papersize_by_name("CDCustom");
	      if (papersize)
		{
		  if (*width == 0)
		    *width = papersize->width;
		  if (*height == 0)
		    *height = papersize->height;
		}
	    }
	  else
	    {
	      int papersizes = stp_known_papersizes();
	      int i;
	      for (i = 0; i < papersizes; i++)
		{
		  papersize = stp_get_papersize_by_index(i);
		  if (verify_papersize(v, papersize))
		    {
		      if (*width == 0)
			*width = papersize->width;
		      if (*height == 0)
			*height = papersize->height;
		      break;
		    }
		}
	    }
	}
      if (*width == 0)
	*width = 612;
      if (*height == 0)
	*height = 792;
    }
}

static void
internal_imageable_area(const stp_vars_t *v, int use_paper_margins,
			int use_maximum_area,
			int *left, int *right, int *bottom, int *top)
{
  int	width, height;			/* Size of page */
  int	rollfeed = 0;			/* Roll feed selected */
  int	cd = 0;			/* CD selected */
  const char *media_size = stp_get_string_parameter(v, "PageSize");
  const char *duplex = stp_get_string_parameter(v, "Duplex");
  int left_margin = 0;
  int right_margin = 0;
  int bottom_margin = 0;
  int top_margin = 0;
  const stp_papersize_t *pt = NULL;
  const input_slot_t *input_slot = NULL;

  if (media_size)
    pt = stp_get_papersize_by_name(media_size);

  input_slot = stp_escp2_get_input_slot(v);
  if (input_slot)
    {
      cd = input_slot->is_cd;
      rollfeed = input_slot->is_roll_feed;
    }

  escp2_media_size(v, &width, &height);
  if (cd)
    {
      if (pt)
	{
	  left_margin = pt->left;
	  right_margin = pt->right;
	  bottom_margin = pt->bottom;
	  top_margin = pt->top;
	}
      else
	{
	  left_margin = 0;
	  right_margin = 0;
	  bottom_margin = 0;
	  top_margin = 0;
	}
    }
  else
    {
      if (pt && use_paper_margins)
	{
	  left_margin = pt->left;
	  right_margin = pt->right;
	  bottom_margin = pt->bottom;
	  top_margin = pt->top;
	}

      left_margin = imax(left_margin, escp2_left_margin(v, rollfeed));
      right_margin = imax(right_margin, escp2_right_margin(v, rollfeed));
      bottom_margin = imax(bottom_margin, escp2_bottom_margin(v, rollfeed));
      top_margin = imax(top_margin, escp2_top_margin(v, rollfeed));
    }
  if (supports_borderless(v) &&
      (use_maximum_area ||
       (!cd && stp_get_boolean_parameter(v, "FullBleed"))))
    {
      if (pt)
	{
	  if (pt->left <= 0 && pt->right <= 0 && pt->top <= 0 &&
	      pt->bottom <= 0)
	    {
	      if (use_paper_margins)
		{
		  unsigned width_limit = escp2_max_paper_width(v);
		  int offset = escp2_zero_margin_offset(v);
		  int margin = escp2_micro_left_margin(v);
		  int sep = escp2_base_separation(v);
		  int delta = -((offset - margin) * 72 / sep);
		  left_margin = delta; /* Allow some overlap if paper isn't */
		  right_margin = delta; /* positioned correctly */
		  if (width - right_margin - 3 > width_limit)
		    right_margin = width - width_limit - 3;
		  if (! stp_escp2_has_cap(v, MODEL_ZEROMARGIN,
					  MODEL_ZEROMARGIN_H_ONLY))
		    {
		      top_margin = -7;
		      bottom_margin = -7;
		    }
		}
	      else
		{
		  left_margin = 0;
		  right_margin = 0;
		  if (! stp_escp2_has_cap(v, MODEL_ZEROMARGIN,
					  MODEL_ZEROMARGIN_H_ONLY))
		    {
		      top_margin = 0;
		      bottom_margin = 0;
		    }
		}
	    }
	}
    }
  if (!use_maximum_area && duplex && strcmp(duplex, "None") != 0)
    {
      left_margin = imax(left_margin, escp2_duplex_left_margin(v));
      right_margin = imax(right_margin, escp2_duplex_right_margin(v));
      bottom_margin = imax(bottom_margin, escp2_duplex_bottom_margin(v));
      top_margin = imax(top_margin, escp2_duplex_top_margin(v));
    }

  if (width > escp2_max_imageable_width(v))
    width = escp2_max_imageable_width(v);
  if (height > escp2_max_imageable_height(v))
    height = escp2_max_imageable_height(v);
  *left =	left_margin;
  *right =	width - right_margin;
  *top =	top_margin;
  *bottom =	height - bottom_margin;
}

/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

static void
escp2_imageable_area(const stp_vars_t *v,   /* I */
		     int  *left,	/* O - Left position in points */
		     int  *right,	/* O - Right position in points */
		     int  *bottom,	/* O - Bottom position in points */
		     int  *top)		/* O - Top position in points */
{
  internal_imageable_area(v, 1, 0, left, right, bottom, top);
}

static void
escp2_maximum_imageable_area(const stp_vars_t *v,   /* I */
			     int  *left,   /* O - Left position in points */
			     int  *right,  /* O - Right position in points */
			     int  *bottom, /* O - Bottom position in points */
			     int  *top)    /* O - Top position in points */
{
  internal_imageable_area(v, 1, 1, left, right, bottom, top);
}

static void
escp2_limit(const stp_vars_t *v,			/* I */
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  *width =	escp2_max_paper_width(v);
  *height =	escp2_max_paper_height(v);
  *min_width =	escp2_min_paper_width(v);
  *min_height =	escp2_min_paper_height(v);
}

static void
escp2_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
  const res_t *res = stp_escp2_find_resolution(v);
  if (res && verify_resolution(v, res))
    {
      *x = res->printed_hres;
      *y = res->printed_vres;
      return;
    }
  *x = -1;
  *y = -1;
}

static const char *
escp2_describe_output(const stp_vars_t *v)
{
  const char *printing_mode = stp_get_string_parameter(v, "PrintingMode");
  const char *input_image_type = stp_get_string_parameter(v, "InputImageType");
  if (input_image_type && strcmp(input_image_type, "Raw") == 0)
    return "Raw";
  else if (printing_mode && strcmp(printing_mode, "BW") == 0)
    return "Grayscale";
  else
    {
      const inkname_t *ink_type = get_inktype(v);
      if (ink_type)
	{
	  switch (ink_type->inkset)
	    {
	    case INKSET_QUADTONE:
	    case INKSET_HEXTONE:
	      return "Grayscale";
	    case INKSET_OTHER:
	    case INKSET_CMYK:
	    case INKSET_CcMmYK:
	    case INKSET_CcMmYyK:
	    case INKSET_CcMmYKk:
	    default:
	      if (ink_type->channels[0].n_subchannels > 0)
		return "KCMY";
	      else
		return "CMY";
	      break;
	    }
	}
      else
	return "CMYK";
    }
}

static int
escp2_has_advanced_command_set(const stp_vars_t *v)
{
  return (stp_escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO) ||
	  stp_escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_1999) ||
	  stp_escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_2000));
}

static int
escp2_use_extended_commands(const stp_vars_t *v, int use_softweave)
{
  return (stp_escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO) ||
	  (stp_escp2_has_cap(v, MODEL_VARIABLE_DOT, MODEL_VARIABLE_YES) &&
	   use_softweave));
}

static int
set_raw_ink_type(stp_vars_t *v)
{
  const inklist_t *inks = stp_escp2_inklist(v);
  int ninktypes = inks->n_inks;
  int i;
  const char *channel_name = stp_get_string_parameter(v, "RawChannels");
  const channel_count_t *count;
  if (!channel_name)
    return 0;
  count = get_channel_count_by_name(channel_name);
  if (!count)
    return 0;

  /*
   * If we're using raw printer output, we dummy up the appropriate inkset.
   */
  for (i = 0; i < ninktypes; i++)
    if (inks->inknames[i].inkset == INKSET_EXTENDED &&
	(inks->inknames[i].channel_count == count->count))
      {
	stp_dprintf(STP_DBG_INK, v, "Changing ink type from %s to %s\n",
		    stp_get_string_parameter(v, "InkType") ?
		    stp_get_string_parameter(v, "InkType") : "NULL",
		    inks->inknames[i].name);
	stp_set_string_parameter(v, "InkType", inks->inknames[i].name);
	stp_set_int_parameter(v, "STPIRawChannels", count->count);
	return 1;
      }
  stp_eprintf
    (v, _("This printer does not support raw printer output at depth %d\n"),
     count->count);
  return 0;
}

static void
adjust_density_and_ink_type(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  const stp_vars_t *pv = pd->paper_type->v;
  double paper_density = .8;

  if (pv && stp_check_float_parameter(pv, "Density", STP_PARAMETER_ACTIVE))
    paper_density = stp_get_float_parameter(pv, "Density");

  if (!stp_check_float_parameter(v, "Density", STP_PARAMETER_DEFAULTED))
    {
      stp_set_float_parameter_active(v, "Density", STP_PARAMETER_ACTIVE);
      stp_set_float_parameter(v, "Density", 1.0);
    }
  stp_scale_float_parameter(v, "Density", paper_density * escp2_density(v));
  pd->drop_size = escp2_ink_type(v);

  if (stp_get_float_parameter(v, "Density") > 1.0)
    stp_set_float_parameter(v, "Density", 1.0);
}

static void
adjust_print_quality(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  const stp_vars_t *pv = pd->paper_type->v;
  double k_upper = 1.0;
  double k_lower = 0;
  double k_transition = 1.0;

  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */

  if (pv)
    {
      int i;
      stp_string_list_t *slist;
      stp_set_default_float_parameter(v, "BlackDensity", 1.0);
      stp_set_default_float_parameter(v, "Saturation", 1.0);
      stp_set_default_float_parameter(v, "Gamma", 1.0);
      slist = stp_list_parameters(pv, STP_PARAMETER_TYPE_STRING_LIST);
      if (slist)
	{
	  int len = stp_string_list_count(slist);
	  for (i = 0; i < len; i++)
	    {
	      const char *name = stp_string_list_param(slist, i)->name;
	      if (!stp_check_string_parameter(v, name, STP_PARAMETER_ACTIVE))
		stp_set_string_parameter(v, name,
					 stp_get_string_parameter(pv, name));
	    }
	  stp_string_list_destroy(slist);
	}
      slist = stp_list_parameters(pv, STP_PARAMETER_TYPE_FILE);
      if (slist)
	{
	  int len = stp_string_list_count(slist);
	  for (i = 0; i < len; i++)
	    {
	      const char *name = stp_string_list_param(slist, i)->name;
	      if (!stp_check_file_parameter(v, name, STP_PARAMETER_ACTIVE))
		stp_set_file_parameter(v, name,
				       stp_get_file_parameter(pv, name));
	    }
	  stp_string_list_destroy(slist);
	}
      slist = stp_list_parameters(pv, STP_PARAMETER_TYPE_INT);
      if (slist)
	{
	  int len = stp_string_list_count(slist);
	  for (i = 0; i < len; i++)
	    {
	      const char *name = stp_string_list_param(slist, i)->name;
	      if (!stp_check_int_parameter(v, name, STP_PARAMETER_ACTIVE))
		stp_set_int_parameter(v, name,
				      stp_get_int_parameter(pv, name));
	    }
	  stp_string_list_destroy(slist);
	}
      slist = stp_list_parameters(pv, STP_PARAMETER_TYPE_DIMENSION);
      if (slist)
	{
	  int len = stp_string_list_count(slist);
	  for (i = 0; i < len; i++)
	    {
	      const char *name = stp_string_list_param(slist, i)->name;
	      if (!stp_check_dimension_parameter(v, name, STP_PARAMETER_ACTIVE))
		stp_set_dimension_parameter(v, name,
					    stp_get_dimension_parameter(pv, name));
	    }
	  stp_string_list_destroy(slist);
	}
      slist = stp_list_parameters(pv, STP_PARAMETER_TYPE_BOOLEAN);
      if (slist)
	{
	  int len = stp_string_list_count(slist);
	  for (i = 0; i < len; i++)
	    {
	      const char *name = stp_string_list_param(slist, i)->name;
	      if (!stp_check_boolean_parameter(v, name, STP_PARAMETER_ACTIVE))
		stp_set_boolean_parameter(v, name,
					  stp_get_boolean_parameter(pv, name));
	    }
	  stp_string_list_destroy(slist);
	}
      slist = stp_list_parameters(pv, STP_PARAMETER_TYPE_CURVE);
      if (slist)
	{
	  int len = stp_string_list_count(slist);
	  for (i = 0; i < len; i++)
	    {
	      const char *name = stp_string_list_param(slist, i)->name;
	      if (!stp_check_curve_parameter(v, name, STP_PARAMETER_ACTIVE))
		stp_set_curve_parameter(v, name,
					stp_get_curve_parameter(pv, name));
	    }
	  stp_string_list_destroy(slist);
	}
      slist = stp_list_parameters(pv, STP_PARAMETER_TYPE_ARRAY);
      if (slist)
	{
	  int len = stp_string_list_count(slist);
	  for (i = 0; i < len; i++)
	    {
	      const char *name = stp_string_list_param(slist, i)->name;
	      if (!stp_check_array_parameter(v, name, STP_PARAMETER_ACTIVE))
		stp_set_array_parameter(v, name,
					stp_get_array_parameter(pv, name));
	    }
	  stp_string_list_destroy(slist);
	}
      slist = stp_list_parameters(pv, STP_PARAMETER_TYPE_RAW);
      if (slist)
	{
	  int len = stp_string_list_count(slist);
	  for (i = 0; i < len; i++)
	    {
	      const char *name = stp_string_list_param(slist, i)->name;
	      if (!stp_check_raw_parameter(v, name, STP_PARAMETER_ACTIVE))
		{
		  const stp_raw_t *r = stp_get_raw_parameter(pv, name);
		  stp_set_raw_parameter(v, name, r->data, r->bytes);
		}
	    }
	  stp_string_list_destroy(slist);
	}
      slist = stp_list_parameters(pv, STP_PARAMETER_TYPE_DOUBLE);
      if (slist)
	{
	  int len = stp_string_list_count(slist);
	  for (i = 0; i < len; i++)
	    {
	      const char *name = stp_string_list_param(slist, i)->name;
	      if (strcmp(name, "BlackDensity") == 0 ||
		  strcmp(name, "Saturation") == 0 ||
		  strcmp(name, "Gamma") == 0)
		stp_scale_float_parameter(v, name,
					  stp_get_float_parameter(pv, name));
	      else if (strcmp(name, "GCRLower") == 0)
		k_lower = stp_get_float_parameter(pv, "GCRLower");
	      else if (strcmp(name, "GCRUpper") == 0)
		k_upper = stp_get_float_parameter(pv, "GCRUpper");
	      else if (strcmp(name, "BlackTrans") == 0)
		k_transition = stp_get_float_parameter(pv, "BlackTrans");
	      else if (!stp_check_float_parameter(v, name, STP_PARAMETER_ACTIVE))
		stp_set_float_parameter(v, name,
					stp_get_float_parameter(pv, name));
	    }
	  stp_string_list_destroy(slist);
	}
    }

  if (!stp_check_float_parameter(v, "GCRLower", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRLower", k_lower);
  if (!stp_check_float_parameter(v, "GCRUpper", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRUpper", k_upper);
  if (!stp_check_float_parameter(v, "BlackTrans", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "BlackTrans", k_transition);


}

static int
count_channels(const inkname_t *inks, int use_aux_channels)
{
  int answer = 0;
  int i;
  for (i = 0; i < inks->channel_count; i++)
    if (inks->channels[i].n_subchannels > 0)
      answer += inks->channels[i].n_subchannels;
  if (use_aux_channels)
    for (i = 0; i < inks->aux_channel_count; i++)
      if (inks->aux_channels[i].n_subchannels > 0)
	answer += inks->aux_channels[i].n_subchannels;
  return answer;
}

static int
compute_channel_count(const inkname_t *ink_type, int channel_limit,
		      int use_aux_channels)
{
  int i;
  int physical_channels = 0;
  for (i = 0; i < channel_limit; i++)
    {
      const ink_channel_t *channel = &(ink_type->channels[i]);
      if (channel)
	physical_channels += channel->n_subchannels;
    }
  if (use_aux_channels)
    for (i = 0; i < ink_type->aux_channel_count; i++)
      if (ink_type->aux_channels[i].n_subchannels > 0)
	physical_channels += ink_type->aux_channels[i].n_subchannels;
  return physical_channels;
}

static double
get_double_param(const stp_vars_t *v, const char *param)
{
  if (param && stp_check_float_parameter(v, param, STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, param);
  else
    return 1.0;
}

static void
setup_inks(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int i, j;
  escp2_dropsize_t *drops;
  const inkname_t *ink_type = pd->inkname;
  const stp_vars_t *pv = pd->paper_type->v;
  int gloss_channel = -1;
  double gloss_scale = get_double_param(v, "Density");

  drops = escp2_copy_dropsizes(v);
  stp_init_debug_messages(v);
  if (stp_check_float_parameter(v, "DropSize1", STP_PARAMETER_ACTIVE))
    {
      drops->dropsizes[0] = stp_get_float_parameter(v, "DropSize1");
      if (drops->dropsizes[0] > 0 && drops->numdropsizes < 1)
	drops->numdropsizes = 1;
    }
  if (stp_check_float_parameter(v, "DropSize2", STP_PARAMETER_ACTIVE))
    {
      drops->dropsizes[1] = stp_get_float_parameter(v, "DropSize2");
      if (drops->dropsizes[1] > 0 && drops->numdropsizes < 2)
	drops->numdropsizes = 2;
    }
  if (stp_check_float_parameter(v, "DropSize3", STP_PARAMETER_ACTIVE))
    {
      drops->dropsizes[2] = stp_get_float_parameter(v, "DropSize3");
      if (drops->dropsizes[2] > 0 && drops->numdropsizes < 3)
	drops->numdropsizes = 3;
    }
  for (i = drops->numdropsizes - 1; i >= 0; i--)
    {
      if (drops->dropsizes[i] > 0)
	break;
      drops->numdropsizes--;
    }
  for (i = 0; i < pd->logical_channels; i++)
    {
      const ink_channel_t *channel = &(ink_type->channels[i]);
      if (channel && channel->n_subchannels > 0)
	{
	  int hue_curve_found = 0;
	  const char *param = channel->subchannels[0].channel_density;
	  shade_t *shades = escp2_copy_shades(v, i);
	  double userval = get_double_param(v, param);
	  STPI_ASSERT(shades->n_shades >= channel->n_subchannels, v);
	  if (ink_type->inkset != INKSET_EXTENDED)
	    {
	      if (strcmp(param, "BlackDensity") == 0)
		stp_channel_set_black_channel(v, i);
	      else if (strcmp(param, "GlossDensity") == 0)
		{
		  gloss_scale *= get_double_param(v, param);
		  gloss_channel = i;
		}
	    }
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      const char *subparam =
		channel->subchannels[j].subchannel_value;
	      if (subparam &&
		  stp_check_float_parameter(v, subparam, STP_PARAMETER_ACTIVE))
		shades->shades[j] = stp_get_float_parameter(v, subparam);
	    }
	  stp_dither_set_inks(v, i, 1.0, ink_darknesses[i % 8],
			      channel->n_subchannels, shades->shades,
			      drops->numdropsizes, drops->dropsizes);
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      const char *subparam =
		channel->subchannels[j].subchannel_scale;
	      double scale = userval * get_double_param(v, subparam);
	      scale *= get_double_param(v, "Density");
	      stp_channel_set_density_adjustment(v, i, j, scale);
	      subparam =
		channel->subchannels[j].subchannel_transition;
	      if (subparam &&
		  stp_check_float_parameter(v, subparam, STP_PARAMETER_ACTIVE))
		stp_channel_set_cutoff_adjustment
		  (v, i, j, stp_get_float_parameter(v, subparam));
	      else if (pv)
		{
		  if (subparam &&
		      stp_check_float_parameter(pv, subparam, STP_PARAMETER_ACTIVE))
		    stp_channel_set_cutoff_adjustment
		      (v, i, j, stp_get_float_parameter(pv, subparam));
		  else if (stp_check_float_parameter(pv, "SubchannelCutoff", STP_PARAMETER_ACTIVE))
		    stp_channel_set_cutoff_adjustment
		      (v, i, j, stp_get_float_parameter(pv, "SubchannelCutoff"));
		}
	    }
	  if (ink_type->inkset != INKSET_EXTENDED)
	    {
	      if (channel->hue_curve_name)
		{
		  const stp_curve_t *curve = NULL;
		  curve = stp_get_curve_parameter(v, channel->hue_curve_name);
		  if (curve)
		    {
		      stp_channel_set_curve(v, i, curve);
		      hue_curve_found = 1;
		    }
		}
	      if (channel->hue_curve && !hue_curve_found)
		stp_channel_set_curve(v, i, channel->hue_curve);
	    }
	  escp2_free_shades(shades);
	}
    }
  if (pd->use_aux_channels)
    {
      int base_count = pd->logical_channels;
      for (i = 0; i < ink_type->aux_channel_count; i++)
	{
	  const ink_channel_t *channel = &(ink_type->aux_channels[i]);
	  if (channel && channel->n_subchannels > 0)
	    {
	      int ch = i + base_count;
	      const char *param = channel->subchannels[0].channel_density;
	      shade_t *shades = escp2_copy_shades(v, ch);
	      double userval = get_double_param(v, param);
	      STPI_ASSERT(shades->n_shades >= channel->n_subchannels, v);
	      if (strcmp(param, "GlossDensity") == 0)
		{
		  gloss_scale *= get_double_param(v, param);
		  stp_channel_set_gloss_channel(v, ch);
		  stp_channel_set_gloss_limit(v, gloss_scale);
		}
	      for (j = 0; j < channel->n_subchannels; j++)
		{
		  const char *subparam =
		    channel->subchannels[j].subchannel_value;
		  if (subparam &&
		      stp_check_float_parameter(v, subparam, STP_PARAMETER_ACTIVE))
		    shades->shades[j] = stp_get_float_parameter(v, subparam);
		}
	      stp_dither_set_inks(v, ch, 1.0, ink_darknesses[ch % 8],
				  channel->n_subchannels, shades->shades,
				  drops->numdropsizes, drops->dropsizes);
	      for (j = 0; j < channel->n_subchannels; j++)
		{
		  const char *subparam =
		    channel->subchannels[j].subchannel_scale;
		  double scale = userval * get_double_param(v, subparam);
		  scale *= get_double_param(v, "Density");
		  stp_channel_set_density_adjustment(v, ch, j, scale);
		  subparam =
		    channel->subchannels[j].subchannel_transition;
		  if (subparam &&
		      stp_check_float_parameter(v, subparam, STP_PARAMETER_ACTIVE))
		    stp_channel_set_cutoff_adjustment
		      (v, ch, j, stp_get_float_parameter(v, subparam));
		  else if (pv)
		    {
		      if (subparam &&
			  stp_check_float_parameter(pv, subparam, STP_PARAMETER_ACTIVE))
			stp_channel_set_cutoff_adjustment
			  (v, ch, j, stp_get_float_parameter(pv, subparam));
		      else if (stp_check_float_parameter(pv, "SubchannelCutoff", STP_PARAMETER_ACTIVE))
			stp_channel_set_cutoff_adjustment
			  (v, ch, j, stp_get_float_parameter(pv, "SubchannelCutoff"));
		    }
		}
	      if (channel->hue_curve)
		{
		  stp_curve_t *curve_tmp =
		    stp_curve_create_copy(channel->hue_curve);
		  (void) stp_curve_rescale(curve_tmp,
					   sqrt(1.0 / stp_get_float_parameter(v, "Gamma")),
					   STP_CURVE_COMPOSE_EXPONENTIATE,
					   STP_CURVE_BOUNDS_RESCALE);
		  stp_channel_set_curve(v, ch, curve_tmp);
		  stp_curve_destroy(curve_tmp);
		}
	      escp2_free_shades(shades);
	    }
	}
    }
  escp2_free_dropsizes(drops);
  stp_flush_debug_messages(v);
}

static void
setup_head_offset(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int i;
  int channel_id = 0;
  int channel_limit = pd->logical_channels;
  const inkname_t *ink_type = pd->inkname;
  if (pd->channels_in_use > pd->logical_channels)
    channel_limit = pd->channels_in_use;
  pd->head_offset = stp_zalloc(sizeof(int) * channel_limit);
  for (i = 0; i < pd->logical_channels; i++)
    {
      const ink_channel_t *channel = &(ink_type->channels[i]);
      if (channel)
	{
	  int j;
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      pd->head_offset[channel_id] =
		channel->subchannels[j].head_offset;
	      channel_id++;
	    }
	}
    }
  if (pd->use_aux_channels)
    {
      for (i = 0; i < ink_type->aux_channel_count; i++)
	{
	  const ink_channel_t *channel = &(ink_type->aux_channels[i]);
	  if (channel)
	    {
	      int j;
	      for (j = 0; j < channel->n_subchannels; j++)
		{
		  pd->head_offset[channel_id] =
		    channel->subchannels[j].head_offset;
		  channel_id++;
		}
	    }
	}
    }
  if (pd->physical_channels == 1)
    pd->head_offset[0] = 0;
  pd->max_head_offset = 0;
  if (pd->physical_channels > 1)
    for (i = 0; i < pd->channels_in_use; i++)
      {
	pd->head_offset[i] = pd->head_offset[i] * pd->res->vres /
	  escp2_base_separation(v);
	if (pd->head_offset[i] > pd->max_head_offset)
	  pd->max_head_offset = pd->head_offset[i];
      }
}

static int
supports_split_channels(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int i;
  int split_channel_count = -1;
  for (i = 0; i < pd->logical_channels; i++)
    {
      int j;
      if (pd->inkname->channels[i].n_subchannels > 0)
	{
	  for (j = 0; j < pd->inkname->channels[i].n_subchannels; j++)
	    {
	      int split_count = pd->inkname->channels[i].subchannels[j].split_channel_count;
	      if (split_count == 0)
		return 0;
	      else if (split_channel_count >= 0 && split_count != split_channel_count)
		return 0;
	      else
		split_channel_count = split_count;
	    }
	}
    }
  if (split_channel_count > 0)
    return split_channel_count;
  else
    return 0;
}


static void
setup_split_channels(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  /*
   * Set up the output channels
   */
  pd->split_channel_count = supports_split_channels(v);
  if (pd->split_channel_count)
    {
      if (pd->res->vres <
	  (escp2_base_separation(v) / escp2_black_nozzle_separation(v)))
	{
	  int incr =
	    (escp2_base_separation(v) / escp2_black_nozzle_separation(v)) /
	    pd->res->vres;
	  pd->split_channel_count /= incr;
	  pd->nozzle_separation *= incr;
	  pd->nozzles /= incr;
	  pd->min_nozzles /= incr;
	}
    }
}

static void
setup_basic(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  pd->advanced_command_set = escp2_has_advanced_command_set(v);
  pd->command_set = stp_escp2_get_cap(v, MODEL_COMMAND);
  pd->variable_dots = stp_escp2_has_cap(v, MODEL_VARIABLE_DOT, MODEL_VARIABLE_YES);
  pd->has_graymode = stp_escp2_has_cap(v, MODEL_GRAYMODE, MODEL_GRAYMODE_YES);
  pd->preinit_sequence = escp2_preinit_sequence(v);
  pd->preinit_remote_sequence = escp2_preinit_remote_sequence(v);
  pd->deinit_remote_sequence = escp2_postinit_remote_sequence(v);
  pd->borderless_sequence = escp2_vertical_borderless_sequence(v);
  pd->base_separation = escp2_base_separation(v);
  pd->resolution_scale = escp2_resolution_scale(v);
}

static void
setup_misc(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  pd->input_slot = stp_escp2_get_input_slot(v);
  pd->paper_type = stp_escp2_get_media_type(v, 0);
  pd->ink_group = escp2_inkgroup(v);
  pd->media_settings = stp_vars_create_copy(pd->paper_type->v);
  stp_escp2_set_media_size(pd->media_settings, v);
  if (stp_check_float_parameter(v, "PageDryTime", STP_PARAMETER_ACTIVE))
    stp_set_float_parameter(pd->media_settings, "PageDryTime",
			    stp_get_float_parameter(v, "PageDryTime"));
  if (stp_check_float_parameter(v, "ScanDryTime", STP_PARAMETER_ACTIVE))
    stp_set_float_parameter(pd->media_settings, "ScanDryTime",
			    stp_get_float_parameter(v, "ScanDryTime"));
  if (stp_check_float_parameter(v, "ScanMinDryTime", STP_PARAMETER_ACTIVE))
    stp_set_float_parameter(pd->media_settings, "ScanMinDryTime",
			    stp_get_float_parameter(v, "ScanMinDryTime"));
  if (stp_check_int_parameter(v, "FeedAdjustment", STP_PARAMETER_ACTIVE))
    stp_set_int_parameter(pd->media_settings, "FeedAdjustment",
			  stp_get_int_parameter(v, "FeedAdjustment"));
  if (stp_check_int_parameter(v, "PaperThickness", STP_PARAMETER_ACTIVE))
    stp_set_int_parameter(pd->media_settings, "PaperThickness",
			  stp_get_int_parameter(v, "PaperThickness"));
  if (stp_check_int_parameter(v, "VacuumIntensity", STP_PARAMETER_ACTIVE))
    stp_set_int_parameter(pd->media_settings, "VacuumIntensity",
			  stp_get_int_parameter(v, "VacuumIntensity"));
  if (stp_check_int_parameter(v, "FeedSequence", STP_PARAMETER_ACTIVE))
    stp_set_int_parameter(pd->media_settings, "FeedSequence",
			  stp_get_int_parameter(v, "FeedSequence"));
  if (stp_check_int_parameter(v, "PrintMethod", STP_PARAMETER_ACTIVE))
    stp_set_int_parameter(pd->media_settings, "PrintMethod",
			  stp_get_int_parameter(v, "PrintMethod"));
  if (stp_check_int_parameter(v, "PlatenGap", STP_PARAMETER_ACTIVE))
    stp_set_int_parameter(pd->media_settings, "PlatenGap",
			  stp_get_int_parameter(v, "PlatenGap"));
}

static void
allocate_channels(stp_vars_t *v, int line_length)
{
  escp2_privdata_t *pd = get_privdata(v);
  const inkname_t *ink_type = pd->inkname;
  int i, j, k;
  int channel_id = 0;
  int split_id = 0;

  pd->cols = stp_zalloc(sizeof(unsigned char *) * pd->channels_in_use);
  pd->channels =
    stp_zalloc(sizeof(physical_subchannel_t *) * pd->channels_in_use);
  if (pd->split_channel_count)
    pd->split_channels = stp_zalloc(sizeof(short) * pd->channels_in_use *
				    pd->split_channel_count);

  for (i = 0; i < pd->logical_channels; i++)
    {
      const ink_channel_t *channel = &(ink_type->channels[i]);
      if (channel)
	{
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      const physical_subchannel_t *sc = &(channel->subchannels[j]);
	      pd->cols[channel_id] = stp_zalloc(line_length);
	      pd->channels[channel_id] = sc;
	      stp_dither_add_channel(v, pd->cols[channel_id], i, j);
	      if (pd->split_channel_count)
		{
		  for (k = 0; k < pd->split_channel_count; k++)
		    pd->split_channels[split_id++] = sc->split_channels[k];
		}
	      channel_id++;
	    }
	}
    }
  if (pd->use_aux_channels && ink_type->aux_channel_count > 0)
    {
      for (i = 0; i < ink_type->aux_channel_count; i++)
	{
	  const ink_channel_t *channel = &(ink_type->aux_channels[i]);
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      const physical_subchannel_t *sc = &(channel->subchannels[j]);
	      pd->cols[channel_id] = stp_zalloc(line_length);
	      pd->channels[channel_id] = sc;
	      stp_dither_add_channel(v, pd->cols[channel_id],
				     i + pd->logical_channels, j);
	      if (pd->split_channel_count)
		{
		  for (k = 0; k < pd->split_channel_count; k++)
		    pd->split_channels[split_id++] = sc->split_channels[k];
		}
	      channel_id++;
	    }
	}
    }
  stp_set_string_parameter(v, "STPIOutputType", escp2_describe_output(v));
}

static unsigned
gcd(unsigned a, unsigned b)
{
  unsigned tmp;
  if (b > a)
    {
      tmp = a;
      a = b;
      b = tmp;
    }
  while (1)
    {
      tmp = a % b;
      if (tmp == 0)
	return b;
      a = b;
      b = tmp;
    }
}

static unsigned
lcm(unsigned a, unsigned b)
{
  if (a == b)
    return a;
  else
    return a * b / gcd(a, b);
}

static int
adjusted_vertical_resolution(const res_t *res)
{
  if (res->vres >= 720)
    return res->vres;
  else if (res->hres >= 720)	/* Special case 720x360 */
    return 720;
  else if (res->vres % 90 == 0)
    return res->vres;
  else
    return lcm(res->hres, res->vres);
}

static int
adjusted_horizontal_resolution(const res_t *res)
{
  if (res->vres % 90 == 0)
    return res->hres;
  else
    return lcm(res->hres, res->vres);
}

static void
setup_resolution(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  const res_t *res = stp_escp2_find_resolution(v);

  int vertical = adjusted_vertical_resolution(res);
  int horizontal = adjusted_horizontal_resolution(res);

  pd->res = res;
  pd->use_extended_commands =
    escp2_use_extended_commands(v, !(pd->res->command));
  pd->physical_xdpi = escp2_base_res(v);
  if (pd->physical_xdpi > pd->res->hres)
    pd->physical_xdpi = pd->res->hres;

  if (pd->use_extended_commands)
    {
      pd->unit_scale = MAX(escp2_max_hres(v), escp2_max_vres(v));
      pd->horizontal_units = horizontal;
      pd->micro_units = horizontal;
    }
  else
    {
      pd->unit_scale = 3600;
      if (pd->res->hres <= 720)
	pd->micro_units = vertical;
      else
	pd->micro_units = horizontal;
      pd->horizontal_units = vertical;
    }
  /* Note hard-coded 1440 -- from Epson manuals */
  if (stp_escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_1999) &&
      stp_escp2_has_cap(v, MODEL_VARIABLE_DOT, MODEL_VARIABLE_NO))
    pd->micro_units = 1440;
  pd->vertical_units = vertical;
  pd->page_management_units = vertical;
}

static void
setup_softweave_parameters(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  pd->horizontal_passes = pd->res->printed_hres / pd->physical_xdpi;
  if (pd->physical_channels == 1 &&
      (pd->inkname->channels[0].subchannels->split_channel_count > 1 ||
       (pd->res->vres >=
	(escp2_base_separation(v) / escp2_black_nozzle_separation(v)))) &&
      (escp2_max_black_resolution(v) < 0 ||
       pd->res->vres <= escp2_max_black_resolution(v)) &&
      escp2_black_nozzles(v))
    pd->use_black_parameters = 1;
  else
    pd->use_black_parameters = 0;
  if (pd->use_fast_360)
    {
      pd->nozzles = escp2_fast_nozzles(v);
      pd->nozzle_separation = escp2_fast_nozzle_separation(v);
      pd->nozzle_start = escp2_fast_nozzle_start(v);
      pd->min_nozzles = escp2_min_fast_nozzles(v);
    }
  else if (pd->use_black_parameters)
    {
      pd->nozzles = escp2_black_nozzles(v);
      pd->nozzle_separation = escp2_black_nozzle_separation(v);
      pd->nozzle_start = escp2_black_nozzle_start(v);
      pd->min_nozzles = escp2_min_black_nozzles(v);
    }
  else
    {
      pd->nozzles = escp2_nozzles(v);
      pd->nozzle_separation = escp2_nozzle_separation(v);
      pd->nozzle_start = escp2_nozzle_start(v);
      pd->min_nozzles = escp2_min_nozzles(v);
    }
}

static void
setup_printer_weave_parameters(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  pd->horizontal_passes = 1;
  pd->nozzles = 1;
  pd->nozzle_separation = 1;
  pd->nozzle_start = 0;
  pd->min_nozzles = 1;
  if (pd->physical_channels == 1)
    pd->use_black_parameters = 1;
  else
    pd->use_black_parameters = 0;
  pd->extra_vertical_passes = 1;
}

static void
setup_head_parameters(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  /*
   * Set up the output channels
   */
  if (strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") == 0)
    pd->logical_channels = 1;
  else
    pd->logical_channels = pd->inkname->channel_count;

  pd->physical_channels =
    compute_channel_count(pd->inkname, pd->logical_channels,
			  pd->use_aux_channels);
  if (pd->physical_channels == 0)
    {
      pd->inkname = stpi_escp2_get_default_black_inkset();
      pd->physical_channels =
	compute_channel_count(pd->inkname, pd->logical_channels,
			      pd->use_aux_channels);
    }

  pd->printer_weave = get_printer_weave(v);

  pd->extra_vertical_passes = 1;
  if (stp_check_int_parameter(v, "BandEnhancement", STP_PARAMETER_ACTIVE))
    pd->extra_vertical_passes =
      1 << stp_get_int_parameter(v, "BandEnhancement");
  if (stp_escp2_has_cap(v, MODEL_FAST_360, MODEL_FAST_360_YES) &&
      (pd->inkname->inkset == INKSET_CMYK || pd->physical_channels == 1) &&
      pd->res->hres == pd->physical_xdpi && pd->res->vres == 360)
    pd->use_fast_360 = 1;
  else
    pd->use_fast_360 = 0;

  /*
   * Set up the printer-specific parameters (weaving)
   */
  pd->use_printer_weave = use_printer_weave(v);
  if (pd->use_printer_weave)
    setup_printer_weave_parameters(v);
  else
    setup_softweave_parameters(v);
  pd->separation_rows = escp2_separation_rows(v);
  pd->pseudo_separation_rows = escp2_pseudo_separation_rows(v);
  pd->extra_720dpi_separation = escp2_extra_720dpi_separation(v);
  pd->bidirectional_upper_limit = escp2_bidirectional_upper_limit(v);

  if (pd->horizontal_passes == 0)
    pd->horizontal_passes = 1;

  setup_head_offset(v);
  setup_split_channels(v);

  if (stp_check_string_parameter(v, "Duplex", STP_PARAMETER_ACTIVE))
    {
      const char *duplex = stp_get_string_parameter(v, "Duplex");
      if (strcmp(duplex, "DuplexTumble") == 0)
	pd->duplex = DUPLEX_TUMBLE;
      else if (strcmp(duplex, "DuplexNoTumble") == 0)
	pd->duplex = DUPLEX_NO_TUMBLE;
      else
	pd->duplex = 0;
    }
  if (pd->duplex)
    pd->extra_vertical_feed = 0;
  else
    pd->extra_vertical_feed = escp2_extra_feed(v);

  if (strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") == 0 &&
      pd->physical_channels == 1)
    {
      if (pd->use_black_parameters)
	pd->initial_vertical_offset =
	  (escp2_black_initial_vertical_offset(v) -
	   (escp2_black_nozzle_start(v) * (int) escp2_black_nozzle_separation(v))) *
	  pd->page_management_units /
	  escp2_base_separation(v);
      else
	pd->initial_vertical_offset = pd->head_offset[0] +
	  ((escp2_initial_vertical_offset(v) -
	    (escp2_nozzle_start(v) * (int) escp2_nozzle_separation(v))) *
	   pd->page_management_units / escp2_base_separation(v));
    }
  else
    pd->initial_vertical_offset =
      (escp2_initial_vertical_offset(v) -
       (escp2_nozzle_start(v) * (int) escp2_nozzle_separation(v))) *
      pd->page_management_units /
      escp2_base_separation(v);

  pd->printing_initial_vertical_offset = 0;
  pd->bitwidth = escp2_bits(v);
}

static void
setup_page(stp_vars_t *v)
{
  escp2_privdata_t *pd = get_privdata(v);
  const input_slot_t *input_slot = stp_escp2_get_input_slot(v);
  int extra_left = 0;
  int extra_top = 0;
  int hub_size = 0;
  int min_horizontal_alignment = escp2_min_horizontal_position_alignment(v);
  int base_horizontal_alignment =
    pd->res->hres / escp2_base_horizontal_position_alignment(v);
  int required_horizontal_alignment =
    MAX(min_horizontal_alignment, base_horizontal_alignment);

  const char *cd_type = stp_get_string_parameter(v, "PageSize");
  if (cd_type && (strcmp(cd_type, "CDCustom") == 0 ))
     {
	int outer_diameter = stp_get_dimension_parameter(v, "CDOuterDiameter");
	stp_set_page_width(v, outer_diameter);
	stp_set_page_height(v, outer_diameter);
	stp_set_width(v, outer_diameter);
	stp_set_height(v, outer_diameter);
	hub_size = stp_get_dimension_parameter(v, "CDInnerDiameter");
     }
 else
    {
	const char *inner_radius_name = stp_get_string_parameter(v, "CDInnerRadius");
  	hub_size = 43 * 10 * 72 / 254;	/* 43 mm standard CD hub */

  	if (inner_radius_name && strcmp(inner_radius_name, "Small") == 0)
   	  hub_size = 16 * 10 * 72 / 254;	/* 15 mm prints to the hole - play it
				   safe and print 16 mm */
    }

  escp2_media_size(v, &(pd->page_true_width), &(pd->page_true_height));
  /* Don't use full bleed mode if the paper itself has a margin */
  if (pd->page_left > 0 || pd->page_top > 0)
    stp_set_boolean_parameter(v, "FullBleed", 0);
  if (stp_escp2_has_cap(v, MODEL_ZEROMARGIN, MODEL_ZEROMARGIN_FULL) &&
      ((!input_slot || !(input_slot->is_cd))))
    {
      pd->page_extra_height =
	max_nozzle_span(v) * pd->page_management_units /
	escp2_base_separation(v);
      if (stp_get_boolean_parameter(v, "FullBleed"))
	pd->paper_extra_bottom = 0;
      else
	pd->paper_extra_bottom = escp2_paper_extra_bottom(v);
    }
  else if (stp_escp2_has_cap(v, MODEL_ZEROMARGIN, MODEL_ZEROMARGIN_YES) &&
	   (stp_get_boolean_parameter(v, "FullBleed")) &&
	   ((!input_slot || !(input_slot->is_cd))))
    {
      pd->paper_extra_bottom = 0;
      pd->page_extra_height =
	escp2_zero_margin_offset(v) * pd->page_management_units /
	escp2_base_separation(v);
    }
  else if (stp_escp2_has_cap(v, MODEL_ZEROMARGIN, MODEL_ZEROMARGIN_RESTR) &&
	   (stp_get_boolean_parameter(v, "FullBleed")) &&
	   ((!input_slot || !(input_slot->is_cd))))
    {
      pd->paper_extra_bottom = 0;
      pd->page_extra_height = 0;
    }
  else if (stp_escp2_printer_supports_duplex(v) && !pd->duplex)
    {
      pd->paper_extra_bottom = escp2_paper_extra_bottom(v);
      pd->page_extra_height =
	max_nozzle_span(v) * pd->page_management_units /
	escp2_base_separation(v);
    }
  else
    {
      if (input_slot)
	pd->page_extra_height = input_slot->extra_height *
	  pd->page_management_units / escp2_base_separation(v);
      else
	pd->page_extra_height = 0;
      pd->paper_extra_bottom = escp2_paper_extra_bottom(v);
    }
  internal_imageable_area(v, 0, 0, &pd->page_left, &pd->page_right,
			  &pd->page_bottom, &pd->page_top);

  if (input_slot && input_slot->is_cd && escp2_cd_x_offset(v) > 0)
    {
      int left_center = escp2_cd_x_offset(v) +
	stp_get_dimension_parameter(v, "CDXAdjustment");
      int top_center = escp2_cd_y_offset(v) +
	stp_get_dimension_parameter(v, "CDYAdjustment");
      pd->page_true_height = pd->page_bottom - pd->page_top;
      pd->page_true_width = pd->page_right - pd->page_left;
      pd->paper_extra_bottom = 0;
      pd->page_extra_height = 0;
      stp_set_left(v, stp_get_left(v) - pd->page_left);
      stp_set_top(v, stp_get_top(v) - pd->page_top);
      pd->page_right -= pd->page_left;
      pd->page_bottom -= pd->page_top;
      pd->page_top = 0;
      pd->page_left = 0;
      extra_top = top_center - (pd->page_bottom / 2);
      extra_left = left_center - (pd->page_right / 2);
      pd->cd_inner_radius = hub_size * pd->micro_units / 72 / 2;
      pd->cd_outer_radius = pd->page_right * pd->micro_units / 72 / 2;
      pd->cd_x_offset =
	((pd->page_right / 2) - stp_get_left(v)) * pd->micro_units / 72;
      pd->cd_y_offset = stp_get_top(v) * pd->res->printed_vres / 72;
      if (escp2_cd_page_height(v))
	{
	  pd->page_right = escp2_cd_page_width(v);
	  pd->page_bottom = escp2_cd_page_height(v);
	  pd->page_true_height = escp2_cd_page_height(v);
	  pd->page_true_width = escp2_cd_page_width(v);
	}
    }

  pd->page_right += extra_left + 1;
  pd->page_width = pd->page_right - pd->page_left;
  pd->image_left = stp_get_left(v) - pd->page_left + extra_left;
  pd->image_width = stp_get_width(v);
  pd->image_scaled_width = pd->image_width * pd->res->hres / 72;
  pd->image_printed_width = pd->image_width * pd->res->printed_hres / 72;
  if (pd->split_channel_count >= 1)
    {
      pd->split_channel_width =
	((pd->image_printed_width + pd->horizontal_passes - 1) /
	 pd->horizontal_passes);
      pd->split_channel_width = (pd->split_channel_width + 7) / 8;
      pd->split_channel_width *= pd->bitwidth;
      if (!(stp_get_debug_level() & STP_DBG_NO_COMPRESSION))
	{
	  pd->comp_buf =
	    stp_malloc(stp_compute_tiff_linewidth(v, pd->split_channel_width));
	}
    }
  pd->image_left_position = pd->image_left * pd->micro_units / 72;
  pd->zero_margin_offset = escp2_zero_margin_offset(v);
  if (supports_borderless(v) &&
      pd->advanced_command_set &&
      ((!input_slot || !(input_slot->is_cd)) &&
       stp_get_boolean_parameter(v, "FullBleed")))
    {
      int margin = escp2_micro_left_margin(v);
      int sep = escp2_base_separation(v);
      pd->image_left_position +=
	(pd->zero_margin_offset - margin) * pd->micro_units / sep;
    }
  /*
   * Many printers print extremely slowly if the starting position
   * is not aligned to 1/180"
   */
  if (required_horizontal_alignment > 1)
    pd->image_left_position =
      (pd->image_left_position / required_horizontal_alignment) *
      required_horizontal_alignment;


  pd->page_bottom += extra_top + 1;
  pd->page_height = pd->page_bottom - pd->page_top;
  pd->image_top = stp_get_top(v) - pd->page_top + extra_top;
  pd->image_height = stp_get_height(v);
  pd->image_scaled_height = pd->image_height * pd->res->vres / 72;
  pd->image_printed_height = pd->image_height * pd->res->printed_vres / 72;

  if (input_slot && input_slot->roll_feed_cut_flags)
    {
      pd->page_true_height += 4; /* Empirically-determined constants */
      pd->page_top += 2;
      pd->page_bottom += 2;
      pd->image_top += 2;
      pd->page_height += 2;
    }
}

static void
set_mask(unsigned char *cd_mask, int x_center, int scaled_x_where,
	 int limit, int expansion, int invert)
{
  int clear_val = invert ? 255 : 0;
  int set_val = invert ? 0 : 255;
  int bytesize = 8 / expansion;
  int byteextra = bytesize - 1;
  int first_x_on = x_center - scaled_x_where;
  int first_x_off = x_center + scaled_x_where;
  if (first_x_on < 0)
    first_x_on = 0;
  if (first_x_on > limit)
    first_x_on = limit;
  if (first_x_off < 0)
    first_x_off = 0;
  if (first_x_off > limit)
    first_x_off = limit;
  first_x_on += byteextra;
  if (first_x_off > (first_x_on - byteextra))
    {
      int first_x_on_byte = first_x_on / bytesize;
      int first_x_on_mod = expansion * (byteextra - (first_x_on % bytesize));
      int first_x_on_extra = ((1 << first_x_on_mod) - 1) ^ clear_val;
      int first_x_off_byte = first_x_off / bytesize;
      int first_x_off_mod = expansion * (byteextra - (first_x_off % bytesize));
      int first_x_off_extra = ((1 << 8) - (1 << first_x_off_mod)) ^ clear_val;
      if (first_x_off_byte < first_x_on_byte)
	{
	  /* This can happen, if 6 or fewer points are turned on */
	  cd_mask[first_x_on_byte] = first_x_on_extra & first_x_off_extra;
	}
      else
	{
	  if (first_x_on_extra != clear_val)
	    cd_mask[first_x_on_byte - 1] = first_x_on_extra;
	  if (first_x_off_byte > first_x_on_byte)
	    memset(cd_mask + first_x_on_byte, set_val,
		   first_x_off_byte - first_x_on_byte);
	  if (first_x_off_extra != clear_val)
	    cd_mask[first_x_off_byte] = first_x_off_extra;
	}
    }
}

static int
escp2_print_data(stp_vars_t *v, stp_image_t *image)
{
  escp2_privdata_t *pd = get_privdata(v);
  int errdiv  = stp_image_height(image) / pd->image_printed_height;
  int errmod  = stp_image_height(image) % pd->image_printed_height;
  int errval  = 0;
  int errlast = -1;
  int errline  = 0;
  int y;
  double outer_r_sq = 0;
  double inner_r_sq = 0;
  int x_center = pd->cd_x_offset * pd->res->printed_hres / pd->micro_units;
  unsigned char *cd_mask = NULL;
  if (pd->cd_outer_radius > 0)
    {
      cd_mask = stp_malloc(1 + (pd->image_printed_width + 7) / 8);
      outer_r_sq = (double) pd->cd_outer_radius * (double) pd->cd_outer_radius;
      inner_r_sq = (double) pd->cd_inner_radius * (double) pd->cd_inner_radius;
    }

  for (y = 0; y < pd->image_printed_height; y ++)
    {
      int duplicate_line = 1;
      unsigned zero_mask;

      if (errline != errlast)
	{
	  errlast = errline;
	  duplicate_line = 0;
	  if (stp_color_get_row(v, image, errline, &zero_mask))
	    return 2;
	}

      if (cd_mask)
	{
	  int y_distance_from_center =
	    pd->cd_outer_radius -
	    ((y + pd->cd_y_offset) * pd->micro_units / pd->res->printed_vres);
	  if (y_distance_from_center < 0)
	    y_distance_from_center = -y_distance_from_center;
	  memset(cd_mask, 0, (pd->image_printed_width + 7) / 8);
	  if (y_distance_from_center < pd->cd_outer_radius)
	    {
	      double y_sq = (double) y_distance_from_center *
		(double) y_distance_from_center;
	      int x_where = sqrt(outer_r_sq - y_sq) + .5;
	      int scaled_x_where = x_where * pd->res->printed_hres / pd->micro_units;
	      set_mask(cd_mask, x_center, scaled_x_where,
		       pd->image_printed_width, 1, 0);
	      if (y_distance_from_center < pd->cd_inner_radius)
		{
		  x_where = sqrt(inner_r_sq - y_sq) + .5;
		  scaled_x_where = x_where * pd->res->printed_hres / pd->micro_units;
		  set_mask(cd_mask, x_center, scaled_x_where,
			   pd->image_printed_width, 1, 1);
		}
	    }
	}

      stp_dither(v, y, duplicate_line, zero_mask, cd_mask);

      stp_write_weave(v, pd->cols);
      errval += errmod;
      errline += errdiv;
      if (errval >= pd->image_printed_height)
	{
	  errval -= pd->image_printed_height;
	  errline ++;
	}
    }
  if (cd_mask)
    stp_free(cd_mask);
  return 1;
}

static int
escp2_print_page(stp_vars_t *v, stp_image_t *image)
{
  int status;
  escp2_privdata_t *pd = get_privdata(v);
  int out_channels;		/* Output bytes per pixel */
  int line_width = (pd->image_printed_width + 7) / 8 * pd->bitwidth;
  int weave_pattern = STP_WEAVE_ZIGZAG;
  if (stp_check_string_parameter(v, "Weave", STP_PARAMETER_ACTIVE))
    {
      const char *weave = stp_get_string_parameter(v, "Weave");
      if (strcmp(weave, "Alternate") == 0)
	weave_pattern = STP_WEAVE_ZIGZAG;
      else if (strcmp(weave, "Ascending") == 0)
	weave_pattern = STP_WEAVE_ASCENDING;
      else if (strcmp(weave, "Descending") == 0)
	weave_pattern = STP_WEAVE_DESCENDING;
      else if (strcmp(weave, "Ascending2X") == 0)
	weave_pattern = STP_WEAVE_ASCENDING_2X;
      else if (strcmp(weave, "Staggered") == 0)
	weave_pattern = STP_WEAVE_STAGGERED;
    }

  stp_initialize_weave
    (v,
     pd->nozzles,
     pd->nozzle_separation * pd->res->vres / escp2_base_separation(v),
     pd->horizontal_passes,
     pd->res->vertical_passes * pd->extra_vertical_passes,
     1,
     pd->channels_in_use,
     pd->bitwidth,
     pd->image_printed_width,
     pd->image_printed_height,
     ((pd->page_extra_height * pd->res->vres / pd->vertical_units) +
      (pd->image_top * pd->res->vres / 72)),
     (pd->page_extra_height +
      (pd->page_height + pd->extra_vertical_feed) * pd->res->vres / 72),
     pd->head_offset,
     weave_pattern,
     stpi_escp2_flush_pass,
     (((stp_get_debug_level() & STP_DBG_NO_COMPRESSION) ||
       pd->split_channel_count > 0) ?
      stp_fill_uncompressed : stp_fill_tiff),
     (((stp_get_debug_level() & STP_DBG_NO_COMPRESSION) ||
       pd->split_channel_count > 0) ?
      stp_pack_uncompressed : stp_pack_tiff),
     (((stp_get_debug_level() & STP_DBG_NO_COMPRESSION) ||
       pd->split_channel_count > 0) ?
      stp_compute_uncompressed_linewidth : stp_compute_tiff_linewidth));

  stp_dither_init(v, image, pd->image_printed_width, pd->res->printed_hres,
		  pd->res->printed_vres);
  allocate_channels(v, line_width);
  adjust_print_quality(v);
  out_channels = stp_color_init(v, image, 65536);

/*  stpi_dither_set_expansion(v, pd->res->hres / pd->res->printed_hres); */

  setup_inks(v);

  status = escp2_print_data(v, image);
  stp_flush_all(v);
  stpi_escp2_terminate_page(v);
  return status;
}

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
static int
escp2_do_print(stp_vars_t *v, stp_image_t *image, int print_op)
{
  int status = 1;
  int i;

  escp2_privdata_t *pd;
  int page_number = stp_get_int_parameter(v, "PageNumber");

  if (!stp_verify(v))
    {
      stp_eprintf(v, _("Print options not verified; cannot print.\n"));
      return 0;
    }

  if (strcmp(stp_get_string_parameter(v, "InputImageType"), "Raw") == 0 &&
      !set_raw_ink_type(v))
    return 0;

  pd = (escp2_privdata_t *) stp_zalloc(sizeof(escp2_privdata_t));

  pd->printed_something = 0;
  pd->last_color = -1;
  pd->last_pass_offset = 0;
  pd->last_pass = -1;
  pd->send_zero_pass_advance =
    stp_escp2_has_cap(v, MODEL_SEND_ZERO_ADVANCE, MODEL_SEND_ZERO_ADVANCE_YES);
  stp_allocate_component_data(v, "Driver", NULL, NULL, pd);

  pd->inkname = get_inktype(v);
  if (pd->inkname->inkset != INKSET_EXTENDED &&
      stp_check_boolean_parameter(v, "UseGloss", STP_PARAMETER_ACTIVE) &&
      stp_get_boolean_parameter(v, "UseGloss"))
    pd->use_aux_channels = 1;
  else
    pd->use_aux_channels = 0;
  if (pd->inkname && pd->inkname->inkset == INKSET_QUADTONE &&
      strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") != 0)
    {
      stp_eprintf(v, "Warning: Quadtone inkset only available in MONO\n");
      stp_set_string_parameter(v, "PrintingMode", "BW");
    }
  if (pd->inkname && pd->inkname->inkset == INKSET_HEXTONE &&
      strcmp(stp_get_string_parameter(v, "PrintingMode"), "BW") != 0)
    {
      stp_eprintf(v, "Warning: Hextone inkset only available in MONO\n");
      stp_set_string_parameter(v, "PrintingMode", "BW");
    }
  pd->channels_in_use = count_channels(pd->inkname, pd->use_aux_channels);

  setup_basic(v);
  setup_resolution(v);
  setup_head_parameters(v);
  setup_page(v);
  setup_misc(v);

  adjust_density_and_ink_type(v);
  if (print_op & OP_JOB_START)
    stpi_escp2_init_printer(v);
  if (print_op & OP_JOB_PRINT)
    {
      stp_image_init(image);
      if ((page_number & 1) && pd->duplex &&
	  ((pd->duplex & pd->input_slot->duplex) == 0))
	  /* If the hardware can't do the duplex operation, we need to
	     emulate it in software */
	image = stpi_buffer_image(image, BUFFER_FLAG_FLIP_X | BUFFER_FLAG_FLIP_Y);
      status = escp2_print_page(v, image);
      stp_image_conclude(image);
    }
  if (print_op & OP_JOB_END)
    stpi_escp2_deinit_printer(v);

  if (pd->head_offset)
    stp_free(pd->head_offset);

  /*
   * Cleanup...
   */
  if (pd->cols)
    {
      for (i = 0; i < pd->channels_in_use; i++)
	if (pd->cols[i])
	  stp_free(pd->cols[i]);
      stp_free(pd->cols);
    }
  if (pd->media_settings)
    stp_vars_destroy(pd->media_settings);
  if (pd->channels)
    stp_free(pd->channels);
  if (pd->split_channels)
    stp_free(pd->split_channels);
  if (pd->comp_buf)
    stp_free(pd->comp_buf);
  stp_free(pd);

  return status;
}

static int
escp2_print(const stp_vars_t *v, stp_image_t *image)
{
  stp_vars_t *nv = stp_vars_create_copy(v);
  int op = OP_JOB_PRINT;
  int status;
  if (!stp_get_string_parameter(v, "JobMode") ||
      strcmp(stp_get_string_parameter(v, "JobMode"), "Page") == 0)
    op = OP_JOB_START | OP_JOB_PRINT | OP_JOB_END;
  stp_prune_inactive_options(nv);
  status = escp2_do_print(nv, image, op);
  stp_vars_destroy(nv);
  return status;
}

static int
escp2_job_start(const stp_vars_t *v, stp_image_t *image)
{
  stp_vars_t *nv = stp_vars_create_copy(v);
  int status;
  stp_prune_inactive_options(nv);
  status = escp2_do_print(nv, image, OP_JOB_START);
  stp_vars_destroy(nv);
  return status;
}

static int
escp2_job_end(const stp_vars_t *v, stp_image_t *image)
{
  stp_vars_t *nv = stp_vars_create_copy(v);
  int status;
  stp_prune_inactive_options(nv);
  status = escp2_do_print(nv, image, OP_JOB_END);
  stp_vars_destroy(nv);
  return status;
}

static const stp_printfuncs_t print_escp2_printfuncs =
{
  escp2_list_parameters,
  escp2_parameters,
  escp2_media_size,
  escp2_imageable_area,
  escp2_maximum_imageable_area,
  escp2_limit,
  escp2_print,
  escp2_describe_resolution,
  escp2_describe_output,
  stp_verify_printer_params,
  escp2_job_start,
  escp2_job_end,
  NULL
};

static stp_family_t print_escp2_module_data =
  {
    &print_escp2_printfuncs,
    NULL
  };


static int
print_escp2_module_init(void)
{
  hue_curve_bounds = stp_curve_create_from_string
    ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
     "<gutenprint>\n"
     "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
     "<sequence count=\"2\" lower-bound=\"0\" upper-bound=\"1\">\n"
     "1 1\n"
     "</sequence>\n"
     "</curve>\n"
     "</gutenprint>");
  return stp_family_register(print_escp2_module_data.printer_list);
}


static int
print_escp2_module_exit(void)
{
  return stp_family_unregister(print_escp2_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_escp2_LTX_stp_module_version
#define stp_module_data print_escp2_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "escp2",
    VERSION,
    "Epson family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_escp2_module_init,
    print_escp2_module_exit,
    (void *) &print_escp2_module_data
  };

