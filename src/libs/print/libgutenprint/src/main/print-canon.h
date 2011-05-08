/*
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *      Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
 *   Copyright (c) 2006 Sascha Sommer
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

#ifndef GUTENPRINT_INTERNAL_CANON_H
#define GUTENPRINT_INTERNAL_CANON_H



/* Codes for possible ink-tank combinations.
 * Each combo is represented by the colors that can be used with
 * the installed ink-tank(s)
 * Combinations of the codes represent the combinations allowed for a model
 * Note that only preferrable combinations should be used
 */
#define CANON_INK_K           1
#define CANON_INK_CMY         2
#define CANON_INK_CMYK        4
#define CANON_INK_CcMmYK      8
#define CANON_INK_CcMmYyK    16

#define CANON_INK_CcMmYyKk_MASK (CANON_INK_CcMmYK|CANON_INK_CcMmYyK)     /* Ink is CcMmYyKk */
#define CANON_INK_CMYK_MASK     (CANON_INK_CMYK|CANON_INK_CcMmYyKk_MASK) /* Ink is CMYK */
#define CANON_INK_CMY_MASK      (CANON_INK_CMY|CANON_INK_CMYK_MASK)      /* Ink is CMY */
#define CANON_INK_K_MASK        (CANON_INK_K|CANON_INK_CMYK_MASK)        /* Ink is K */



/* FIXME someday we will have to fix the internal names (will break the ppds ;(
* List of possible ink settings ordered by descending ink count
*the driver will check if the current print mode supports the ink combination before offering it
*/
static struct canon_inktype_s {
    const unsigned int ink_type;
    const unsigned int num_channels;
    const char* name;
    const char* text;
} canon_inktypes[] = {
    {CANON_INK_CcMmYyK,7,"PhotoCMYK","Photo CcMmYyK Color"},
    {CANON_INK_CcMmYK,6,"PhotoCMY", "Photo CcMmYK Color"},
    {CANON_INK_CMYK,4,"CMYK","CMYK Color"},
    {CANON_INK_CMY,3,"RGB","CMY Color"},
    {CANON_INK_K,1,"Gray","Black"}
};

/* the PIXMA iP4000 and maybe other printers use following table to store
   5 pixels with 3 levels in 1 byte, All possible pixel combinations are given
   numbers from 0 (=00,00,00,00,00) to 242 (=10,10,10,10,10)
   combinations where the value of one of the pixels would be 3 are skipped
*/
static const unsigned char tentoeight[] =
{
    0,  1,  2,  0,  3,  4,  5,  0,  6,  7,  8,  0,  0,  0,  0,  0,
    9, 10, 11,  0, 12, 13, 14,  0, 15, 16, 17,  0,  0,  0,  0,  0,
   18, 19, 20,  0, 21, 22, 23,  0, 24, 25, 26,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   27, 28, 29,  0, 30, 31, 32,  0, 33, 34, 35,  0,  0,  0,  0,  0,
   36, 37, 38,  0, 39, 40, 41,  0, 42, 43, 44,  0,  0,  0,  0,  0,
   45, 46, 47,  0, 48, 49, 50,  0, 51, 52, 53,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   54, 55, 56,  0, 57, 58, 59,  0, 60, 61, 62,  0,  0,  0,  0,  0,
   63, 64, 65,  0, 66, 67, 68,  0, 69, 70, 71,  0,  0,  0,  0,  0,
   72, 73, 74,  0, 75, 76, 77,  0, 78, 79, 80,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   81, 82, 83,  0, 84, 85, 86,  0, 87, 88, 89,  0,  0,  0,  0,  0,
   90, 91, 92,  0, 93, 94, 95,  0, 96, 97, 98,  0,  0,  0,  0,  0,
   99,100,101,  0,102,103,104,  0,105,106,107,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  108,109,110,  0,111,112,113,  0,114,115,116,  0,  0,  0,  0,  0,
  117,118,119,  0,120,121,122,  0,123,124,125,  0,  0,  0,  0,  0,
  126,127,128,  0,129,130,131,  0,132,133,134,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  135,136,137,  0,138,139,140,  0,141,142,143,  0,  0,  0,  0,  0,
  144,145,146,  0,147,148,149,  0,150,151,152,  0,  0,  0,  0,  0,
  153,154,155,  0,156,157,158,  0,159,160,161,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  162,163,164,  0,165,166,167,  0,168,169,170,  0,  0,  0,  0,  0,
  171,172,173,  0,174,175,176,  0,177,178,179,  0,  0,  0,  0,  0,
  180,181,182,  0,183,184,185,  0,186,187,188,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  189,190,191,  0,192,193,194,  0,195,196,197,  0,  0,  0,  0,  0,
  198,199,200,  0,201,202,203,  0,204,205,206,  0,  0,  0,  0,  0,
  207,208,209,  0,210,211,212,  0,213,214,215,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  216,217,218,  0,219,220,221,  0,222,223,224,  0,  0,  0,  0,  0,
  225,226,227,  0,228,229,230,  0,231,232,233,  0,  0,  0,  0,  0,
  234,235,236,  0,237,238,239,  0,240,241,242,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

const char* prexml_iP2700 ="<?xml version=\"1.0\" encoding=\"utf-8\" ?><cmd xmlns:ivec=\"http://www.canon.com/ns/cmd/2008/07/common/\"><ivec:contents><ivec:operation>StartJob</ivec:operation><ivec:param_set servicetype=\"print\"><ivec:jobID>00000001</ivec:jobID><ivec:bidi>0</ivec:bidi></ivec:param_set></ivec:contents></cmd><?xml version=\"1.0\" encoding=\"utf-8\" ?><cmd xmlns:ivec=\"http://www.canon.com/ns/cmd/2008/07/common/\" xmlns:vcn=\"http://www.canon.com/ns/cmd/2008/07/canon/\"><ivec:contents><ivec:operation>VendorCmd</ivec:operation><ivec:param_set servicetype=\"print\"><vcn:ijoperation>ModeShift</vcn:ijoperation><vcn:ijmode>1</vcn:ijmode><ivec:jobID>00000001</ivec:jobID></ivec:param_set></ivec:contents></cmd>";

const char* postxml_iP2700 ="<?xml version=\"1.0\" encoding=\"utf-8\" ?><cmd xmlns:ivec=\"http://www.canon.com/ns/cmd/2008/07/common/\"><ivec:contents><ivec:operation>EndJob</ivec:operation><ivec:param_set servicetype=\"print\"><ivec:jobID>00000001</ivec:jobID></ivec:param_set></ivec:contents></cmd>";

#endif
