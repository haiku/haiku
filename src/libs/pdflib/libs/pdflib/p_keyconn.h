/*---------------------------------------------------------------------------*
 |              PDFlib - A library for generating PDF on the fly             |
 +---------------------------------------------------------------------------+
 | Copyright (c) 1997-2004 Thomas Merz and PDFlib GmbH. All rights reserved. |
 +---------------------------------------------------------------------------+
 |                                                                           |
 |    This software is subject to the PDFlib license. It is NOT in the       |
 |    public domain. Extended versions and commercial licenses are           |
 |    available, please check http://www.pdflib.com.                         |
 |                                                                           |
 *---------------------------------------------------------------------------*/

/* $Id: p_keyconn.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib shared keys connection lists
 *
 */

#ifndef P_KEYCONN_H
#define P_KEYCONN_H

#if defined(P_IMAGE_C) || defined(P_XGSTATE_C) || defined(P_PARAMS_C)
static const pdc_keyconn gs_renderingintents[] =
{
    {"Auto",                 AutoIntent},
    {"AbsoluteColorimetric", AbsoluteColorimetric},
    {"RelativeColorimetric", RelativeColorimetric},
    {"Saturation",           Saturation},
    {"Perceptual",           Perceptual},
    {NULL, 0}
};
#endif /* P_IMAGE_C || P_XGSTATE_C || P_PARAMS_C */

#if defined(P_XGSTATE_C)
static const pdc_keyconn gs_blendmodes[] =
{
    {"Normal",          BM_Normal},
    {"Multiply",        BM_Multiply},
    {"Screen",          BM_Screen},
    {"Overlay",         BM_Overlay},
    {"Darken",          BM_Darken},
    {"Lighten",         BM_Lighten},
    {"ColorDodge",      BM_ColorDodge},
    {"ColorBurn",       BM_ColorBurn},
    {"HardLight",       BM_HardLight},
    {"SoftLight",       BM_SoftLight},
    {"Difference",      BM_Difference},
    {"Exclusion",       BM_Exclusion},
    {"Hue",             BM_Hue},
    {"Saturation",      BM_Saturation},
    {"Color",           BM_Color},
    {"Luminosity",      BM_Luminosity},
    {NULL, 0}
};
#endif /* P_XGSTATE_C */

#if defined(P_PDI_C) || defined(P_PARAMS_C)
static const pdc_keyconn pdf_usebox_keylist[] =
{
    {"art",   use_art},
    {"bleed", use_bleed},
    {"crop",  use_crop},
    {"media", use_media},
    {"trim",  use_trim},
    {NULL, 0}
};
#endif /* P_PDI_C || P_PARAMS_C */

#if defined(P_TEXT_C) || defined(P_PARAMS_C) || defined(P_BLOCK_C)
static const pdc_keyconn pdf_textformat_keylist[] =
{
    {"auto",    pdc_auto},
    {"auto2",   pdc_auto2},
    {"bytes",   pdc_bytes},
    {"bytes2",  pdc_bytes2},
    {"utf8",    pdc_utf8},
    {"utf16",   pdc_utf16},
    {"utf16be", pdc_utf16be},
    {"utf16le", pdc_utf16le},
    {NULL, 0}
};
#endif /* P_TEXT_C || P_PARAMS_C || P_BLOCK_C */

#if defined(P_TEXT_C) || defined(P_IMAGE_C) || defined(P_BLOCK_C)
static const pdc_keyconn pdf_orientate_keylist[] =
{
    {"north",   0},
    {"west",   90},
    {"south", 180},
    {"east",  270},
    {NULL, 0}
};
#endif /* P_TEXT_C || P_IMAGE_C || P_BLOCK_C */

#if defined(P_TEXT_C) || defined(P_IMAGE_C) || defined(P_BLOCK_C)
static const pdc_keyconn pdf_fitmethod_keylist[] =
{
    {"nofit",       pdc_nofit},
    {"clip",        pdc_clip},
    {"slice",       pdc_slice},
    {"meet",        pdc_meet},
    {"entire",      pdc_entire},
    {"auto",        pdc_tauto},
    {NULL, 0}
};
#endif /* P_TEXT_C || P_IMAGE_C || P_BLOCK_C */

#if defined(P_FONT_C) || defined(P_HYPER_C) || defined(P_BLOCK_C)
static const pdc_keyconn pdf_fontstyle_keylist[] =
{
    {"normal",     pdc_Normal},
    {"bold",       pdc_Bold },
    {"italic",     pdc_Italic},
    {"bolditalic", pdc_BoldItalic},
    {NULL, 0}
};
#endif /* P_FONT_C || P_HYPER_C || P_BLOCK_C */

#if defined(P_COLOR_C) || defined(P_BLOCK_C)
static const pdc_keyconn pdf_colorspace_keylist[] =
{
    {"none",         NoColor},
    {"gray",         DeviceGray},
    {"rgb",          DeviceRGB },
    {"cmyk",         DeviceCMYK},
    {"spotname",     Separation},
    {"spot",         Separation},
    {"pattern",      PatternCS},
    {"iccbasedgray", ICCBased},
    {"iccbasedrgb",  ICCBased},
    {"iccbasedcmyk", ICCBased},
    {"lab",          Lab},
    {NULL, 0}
};
#endif /* P_COLOR_C || P_BLOCK_C */

#if defined(P_IMAGE_C) || defined(P_BLOCK_C)
typedef enum
{
    dpi_none = -999999,
    dpi_internal = 0
}
pdf_dpi_states;

static const pdc_keyconn pdf_dpi_keylist[] =
{
    {"none",     dpi_none},
    {"internal", dpi_internal},
    {NULL, 0}
};
#endif /* P_IMAGE_C || P_BLOCK_C */

#endif  /* P_KEYCONN_H */
