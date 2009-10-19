/* $Id: p_icc9809.h 14574 2005-10-29 16:27:43Z bonefish $ 
 *
 * Header file of ICC (name see note above) for ICClib and PDFlib
 *
 */
 
/*
 * Note: Modified for use by icclib V2.00:
 *
 * Changed guard bands from ICC_H to ICC9809_H
 *
 * Replace tag last values 0xFFFFFFFFL with define icMaxTagVal,
 * and define this to be -1, for better compiler compatibility.
 *
 * Add section to use machine specific INR & ORD to define
 * the sizes of ic Numbers, if ORD is defined.
 *
 * Adding colorspaces 'MCH5-8' for Hexachrome and others. (Colorsync ?)
 * Added the Positive/Negative and Color/BlackAndWhite Attribute bits
 *
 * I believe icMeasurementFlare as an enumeration is bogus in
 * this file. It is meant to be a u16.16 number.
 *
 * Add Chromaticity Tag and Type from ICC.1A:1999-04,
 * but there is no formal "icc.h" from the ICC that indicates
 * what the names should be.
 *
 * Added Colorsync 2.5 specific VideoCardGamma defines.
 *
 *  Graeme Gill.
 */

/* Header file guard bands */
#ifndef P_ICC9809_H
#define P_ICC9809_H


#endif /* P_ICC9809_H */
