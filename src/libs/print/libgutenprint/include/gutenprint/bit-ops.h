/*
 * "$Id: bit-ops.h,v 1.4 2009/05/17 19:25:36 rlk Exp $"
 *
 *   Softweave calculator for gimp-print.
 *
 *   Copyright 2000 Charles Briscoe-Smith <cpbs@debian.org>
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

/**
 * @file gutenprint/bit-ops.h
 * @brief Bit operations.
 */

#ifndef GUTENPRINT_BIT_OPS_H
#define GUTENPRINT_BIT_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interleave a buffer consisting of two bit strings of length single_length
 * into one string of packed two-bit ints.
 *
 * @param line the input bit string
 * @param single_length the length (in bytes) of the input
 * @param outbuf the output.
 */
extern void	stp_fold(const unsigned char *line, int single_length,
			 unsigned char *outbuf);

/**
 * Interleave a buffer consisting of three bit strings of length single_length
 * into one string of packed three-bit ints.
 *
 * @param line the input bit string
 * @param single_length the length (in bytes) of the input
 * @param outbuf the output.
 */
extern void	stp_fold_3bit(const unsigned char *line, int single_length,
			 unsigned char *outbuf);

/**
 * Interleave a buffer consisting of three bit strings of length single_length
 * into one string of packed three-bit ints.
 *
 * @param line the input bit string
 * @param single_length the length (in bytes) of the input
 * @param outbuf the output.
 */
extern void	stp_fold_3bit_323(const unsigned char *line, int single_length,
			 unsigned char *outbuf);

/**
 * Interleave a buffer consisting of four bit strings of length single_length
 * into one string of packed four-bit ints.
 *
 * @param line the input bit string
 * @param single_length the length (in bytes) of the input
 * @param outbuf the output.
 */
extern void	stp_fold_4bit(const unsigned char *line, int single_length,
			 unsigned char *outbuf);

/**
 * Split an input sequence of packed 1 or 2 bit integers into two or more
 * outputs of equal length, distributing non-zero integers round robin
 * into each output.  Used in "high quality" modes when extra passes are
 * made, to ensure that each pass gets an equal number of ink drops.
 * Each output is as long as the input.
 *
 * @param height the number of integers in the input divided by 8
 * @param bits the bit depth (1 or 2)
 * @param n the number of outputs into which the input should be distributed
 * @param in the input bit string
 * @param stride the stride across the outputs (if it's necessary to
 * distribute the input over non-contiguous members of the array of outputs)
 * @param outs the array of output bit strings
 */
extern void	stp_split(int height, int bits, int n, const unsigned char *in,
			  int stride, unsigned char **outs);

/**
 * Deprecated -- use stp_split
 */
extern void	stp_split_2(int height, int bits, const unsigned char *in,
			    unsigned char *outhi, unsigned char *outlo);

/**
 * Deprecated -- use stp_split
 */
extern void	stp_split_4(int height, int bits, const unsigned char *in,
			    unsigned char *out0, unsigned char *out1,
			    unsigned char *out2, unsigned char *out3);

/**
 * Unpack an input sequence of packed 1 or 2 bit integers into two or more
 * outputs of equal length.  The input is round robined into the outputs.
 * Each output is 1/n as long as the input.
 *
 * @param height the number of integers in the input divided by 8
 * @param bits the bit depth (1 or 2)
 * @param n the number of outputs into which the input should be distributed
 * @param in the input bit string
 * @param outs the array of output bit strings
 * 
 */
extern void	stp_unpack(int height, int bits, int n, const unsigned char *in,
			   unsigned char **outs);

/**
 * Deprecated -- use stp_unpack
 */
extern void	stp_unpack_2(int height, int bits, const unsigned char *in,
			     unsigned char *outlo, unsigned char *outhi);

/**
 * Deprecated -- use stp_unpack
 */
extern void	stp_unpack_4(int height, int bits, const unsigned char *in,
			     unsigned char *out0, unsigned char *out1,
			     unsigned char *out2, unsigned char *out3);

/**
 * Deprecated -- use stp_unpack
 */
extern void	stp_unpack_8(int height, int bits, const unsigned char *in,
			     unsigned char *out0, unsigned char *out1,
			     unsigned char *out2, unsigned char *out3,
			     unsigned char *out4, unsigned char *out5,
			     unsigned char *out6, unsigned char *out7);

/**
 * Deprecated -- use stp_unpack
 */
extern void	stp_unpack_16(int height, int bits, const unsigned char *in,
			      unsigned char *out0, unsigned char *out1,
			      unsigned char *out2, unsigned char *out3,
			      unsigned char *out4, unsigned char *out5,
			      unsigned char *out6, unsigned char *out7,
			      unsigned char *out8, unsigned char *out9,
			      unsigned char *out10, unsigned char *out11,
			      unsigned char *out12, unsigned char *out13,
			      unsigned char *out14, unsigned char *out15);

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_BIT_OPS_H */
