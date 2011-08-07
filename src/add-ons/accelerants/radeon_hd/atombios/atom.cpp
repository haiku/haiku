/*
 * Copyright 2008 Advanced Micro Devices, Inc.  
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author: Stanislaw Skowronek
 */

/* Rewritten for the Haiku Operating System Radeon HD driver
 * Author:
 *	Alexander von Gluck, kallisti5@unixzen.com
 */


#include <Debug.h>

#include "atom.h"
#include "atom-names.h"
#include "atom-bits.h"


#undef TRACE

#define TRACE_ATOM
#ifdef TRACE_ATOM
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


#define ATOM_COND_ABOVE		0
#define ATOM_COND_ABOVEOREQUAL	1
#define ATOM_COND_ALWAYS	2
#define ATOM_COND_BELOW		3
#define ATOM_COND_BELOWOREQUAL	4
#define ATOM_COND_EQUAL		5
#define ATOM_COND_NOTEQUAL	6

#define ATOM_PORT_ATI	0
#define ATOM_PORT_PCI	1
#define ATOM_PORT_SYSIO	2

#define ATOM_UNIT_MICROSEC	0
#define ATOM_UNIT_MILLISEC	1

#define PLL_INDEX	2
#define PLL_DATA	3

typedef struct {
	atom_context *ctx;

	uint32 *ps, *ws;
	int ps_shift;
	uint16 start;
	uint16 last_jump;
	uint16 last_jump_count;
	bool abort;
} atom_exec_context;

int atom_debug = 0;
status_t atom_execute_table_locked(atom_context *ctx,
	int index, uint32 *params);
status_t atom_execute_table(atom_context *ctx, int index, uint32 *params);

static uint32 atom_arg_mask[8] = {0xFFFFFFFF, 0xFFFF, 0xFFFF00, 0xFFFF0000,
	0xFF, 0xFF00, 0xFF0000, 0xFF000000};
static int atom_arg_shift[8] = {0, 0, 8, 16, 0, 8, 16, 24};
static int atom_dst_to_src[8][4] = {
	// translate destination alignment field to the source alignment encoding
	{ 0, 0, 0, 0 },
	{ 1, 2, 3, 0 },
	{ 1, 2, 3, 0 },
	{ 1, 2, 3, 0 },
	{ 4, 5, 6, 7 },
	{ 4, 5, 6, 7 },
	{ 4, 5, 6, 7 },
	{ 4, 5, 6, 7 },
};
static int atom_def_dst[8] = { 0, 0, 1, 2, 0, 1, 2, 3 };

static int debug_depth = 0;

static uint32
atom_iio_execute(atom_context *ctx, int base, uint32 index, uint32 data)
{
	uint32 temp = 0xCDCDCDCD;
	while (1)
	switch(CU8(base)) {
	case ATOM_IIO_NOP:
		base++;
		break;
	case ATOM_IIO_READ:
		temp = ctx->card->ioreg_read(CU16(base + 1));
		base += 3;
		break;
	case ATOM_IIO_WRITE:
		ctx->card->ioreg_write(CU16(base + 1), temp);
		base += 3;
		break;
	case ATOM_IIO_CLEAR:
		temp &= ~((0xFFFFFFFF >> (32 - CU8(base + 1))) << CU8(base + 2));
		base += 3;
		break;
	case ATOM_IIO_SET:
		temp |= (0xFFFFFFFF >> (32 - CU8(base + 1))) << CU8(base + 2);
		base += 3;
		break;
	case ATOM_IIO_MOVE_INDEX:
		temp &= ~((0xFFFFFFFF >> (32 - CU8(base + 1))) << CU8(base + 2));
		temp |= ((index >> CU8(base + 2))
			& (0xFFFFFFFF >> (32 - CU8(base + 1)))) << CU8(base + 3);
		base += 4;
		break;
	case ATOM_IIO_MOVE_DATA:
		temp &= ~((0xFFFFFFFF >> (32 - CU8(base + 1))) << CU8(base + 2));
		temp |= ((data >> CU8(base + 2))
			& (0xFFFFFFFF >> (32 - CU8(base + 1)))) << CU8(base + 3);
		base += 4;
		break;
	case ATOM_IIO_MOVE_ATTR:
		temp &= ~((0xFFFFFFFF >> (32 - CU8(base + 1))) << CU8(base + 2));
		temp |= ((ctx->io_attr >> CU8(base + 2))
			& (0xFFFFFFFF >> (32 - CU8(base + 1)))) << CU8(base + 3);
		base += 4;
		break;
	case ATOM_IIO_END:
		return temp;
	default:
		TRACE("%s: Unknown IIO opcode.\n", __func__);
		return 0;
	}
}


static uint32
atom_get_src_int(atom_exec_context *ctx, uint8 attr, int *ptr,
	uint32 *saved, int print)
{
	uint32 idx, val = 0xCDCDCDCD, align, arg;
	atom_context *gctx = ctx->ctx;
	arg = attr & 7;
	align = (attr >> 3) & 7;
	switch(arg) {
		case ATOM_ARG_REG:
			idx = U16(*ptr);
			(*ptr)+=2;
			idx += gctx->reg_block;
			switch(gctx->io_mode) {
				case ATOM_IO_MM:
					val = gctx->card->reg_read(idx);
					break;
				case ATOM_IO_PCI:
					TRACE("%s: PCI registers are not implemented.\n", __func__);
					return 0;
				case ATOM_IO_SYSIO:
					TRACE("%s: SYSIO registers are not implemented.\n",
						__func__);
					return 0;
				default:
					if (!(gctx->io_mode & 0x80)) {
						TRACE("%s: Bad IO mode.\n", __func__);
						return 0;
					}
					if (!gctx->iio[gctx->io_mode & 0x7F]) {
						TRACE("%s: Undefined indirect IO read method %d.\n",
							__func__, gctx->io_mode & 0x7F);
						return 0;
					}
					val = atom_iio_execute(gctx,
						gctx->iio[gctx->io_mode & 0x7F], idx, 0);
			}
			break;
		case ATOM_ARG_PS:
			idx = U8(*ptr);
			(*ptr)++;
			val = ctx->ps[idx];
				// TODO	: val = get_unaligned_le32((u32 *)&ctx->ps[idx]);
			break;
		case ATOM_ARG_WS:
			idx = U8(*ptr);
			(*ptr)++;
			switch(idx) {
				case ATOM_WS_QUOTIENT:
					val = gctx->divmul[0];
					break;
				case ATOM_WS_REMAINDER:
					val = gctx->divmul[1];
					break;
				case ATOM_WS_DATAPTR:
					val = gctx->data_block;
					break;
				case ATOM_WS_SHIFT:
					val = gctx->shift;
					break;
				case ATOM_WS_OR_MASK:
					val = 1 << gctx->shift;
					break;
				case ATOM_WS_AND_MASK:
					val = ~(1 << gctx->shift);
					break;
				case ATOM_WS_FB_WINDOW:
					val = gctx->fb_base;
					break;
				case ATOM_WS_ATTRIBUTES:
					val = gctx->io_attr;
					break;
				case ATOM_WS_REGPTR:
					val = gctx->reg_block;
					break;
				default:
					val = ctx->ws[idx];
			}
			break;
		case ATOM_ARG_ID:
			idx = U16(*ptr);
			(*ptr) += 2;
			val = U32(idx + gctx->data_block);
			break;
		case ATOM_ARG_FB:
			idx = U8(*ptr);
			(*ptr)++;
			val = gctx->scratch[((gctx->fb_base + idx) / 4)];
			return 0;
		case ATOM_ARG_IMM:
			switch(align) {
				case ATOM_SRC_DWORD:
					val = U32(*ptr);
					(*ptr)+=4;
					return val;
				case ATOM_SRC_WORD0:
				case ATOM_SRC_WORD8:
				case ATOM_SRC_WORD16:
					val = U16(*ptr);
					(*ptr) += 2;
					return val;
				case ATOM_SRC_BYTE0:
				case ATOM_SRC_BYTE8:
				case ATOM_SRC_BYTE16:
				case ATOM_SRC_BYTE24:
					val = U8(*ptr);
					(*ptr)++;
					return val;
			}
			return 0;
		case ATOM_ARG_PLL:
			idx = U8(*ptr);
			(*ptr)++;
			val = gctx->card->pll_read(idx);
			break;
		case ATOM_ARG_MC:
			idx = U8(*ptr);
			(*ptr)++;
			val = gctx->card->mc_read(idx);
			return 0;
	}
	if (saved)
		*saved = val;
	val &= atom_arg_mask[align];
	val >>= atom_arg_shift[align];
	return val;
}


static void
atom_skip_src_int(atom_exec_context *ctx, uint8 attr, int *ptr)
{
	uint32 align = (attr >> 3) & 7, arg = attr & 7;
	switch(arg) {
		case ATOM_ARG_REG:
		case ATOM_ARG_ID:
			(*ptr) += 2;
			break;
		case ATOM_ARG_PLL:
		case ATOM_ARG_MC:
		case ATOM_ARG_PS:
		case ATOM_ARG_WS:
		case ATOM_ARG_FB:
			(*ptr)++;
			break;
		case ATOM_ARG_IMM:
			switch(align) {
				case ATOM_SRC_DWORD:
					(*ptr) += 4;
					return;
				case ATOM_SRC_WORD0:
				case ATOM_SRC_WORD8:
				case ATOM_SRC_WORD16:
					(*ptr) += 2;
					return;
				case ATOM_SRC_BYTE0:
				case ATOM_SRC_BYTE8:
				case ATOM_SRC_BYTE16:
				case ATOM_SRC_BYTE24:
					(*ptr)++;
					return;
			}
			return;
	}
}


static uint32
atom_get_src(atom_exec_context *ctx, uint8 attr, int *ptr)
{
	return atom_get_src_int(ctx, attr, ptr, NULL, 1);
}


static uint32
atom_get_src_direct(atom_exec_context *ctx, uint8_t align, int *ptr)
{
	uint32 val = 0xCDCDCDCD;

	switch (align) {
		case ATOM_SRC_DWORD:
			val = U32(*ptr);
			(*ptr) += 4;
			break;
		case ATOM_SRC_WORD0:
		case ATOM_SRC_WORD8:
		case ATOM_SRC_WORD16:
			val = U16(*ptr);
			(*ptr) += 2;
			break;
		case ATOM_SRC_BYTE0:
		case ATOM_SRC_BYTE8:
		case ATOM_SRC_BYTE16:
		case ATOM_SRC_BYTE24:
			val = U8(*ptr);
			(*ptr)++;
			break;
	}
	return val;
}


static uint32
atom_get_dst(atom_exec_context *ctx, int arg, uint8 attr,
	int *ptr, uint32 *saved, int print)
{
	return atom_get_src_int(ctx,
		arg|atom_dst_to_src[(attr>>3)&7][(attr>>6)&3]<<3, ptr, saved, print);
}


static void
atom_skip_dst(atom_exec_context *ctx, int arg, uint8 attr, int *ptr)
{
	atom_skip_src_int(ctx,
		arg|atom_dst_to_src[(attr>>3)&7][(attr>>6)&3]<<3, ptr);
}


static void
atom_put_dst(atom_exec_context *ctx, int arg, uint8 attr,
	int *ptr, uint32 val, uint32 saved)
{
	uint32 align = atom_dst_to_src[(attr>>3)&7][(attr>>6)&3],
		old_val = val, idx;
	atom_context *gctx = ctx->ctx;
	old_val &= atom_arg_mask[align] >> atom_arg_shift[align];
	val <<= atom_arg_shift[align];
	val &= atom_arg_mask[align];
	saved &= ~atom_arg_mask[align];
	val |= saved;
	switch(arg) {
		case ATOM_ARG_REG:
			idx = U16(*ptr);
			(*ptr) += 2;
			idx += gctx->reg_block;
			switch(gctx->io_mode) {
				case ATOM_IO_MM:
					if (idx == 0)
						gctx->card->reg_write(idx, val<<2);
					else
						gctx->card->reg_write(idx, val);
					break;
				case ATOM_IO_PCI:
					TRACE("%s: PCI registers are not implemented.\n",
						__func__);
					return;
				case ATOM_IO_SYSIO:
					TRACE("%s: SYSIO registers are not implemented.\n",
						__func__);
					return;
				default:
					if (!(gctx->io_mode&0x80)) {
						TRACE("%s: Bad IO mode.\n", __func__);
						return;
					}
					if (!gctx->iio[gctx->io_mode&0xFF]) {
						TRACE("%s: Undefined indirect IO write method %d\n",
							__func__, gctx->io_mode & 0x7F);
						return;
					}
					atom_iio_execute(gctx, gctx->iio[gctx->io_mode&0xFF],
						idx, val);
			}
			break;
		case ATOM_ARG_PS:
			idx = U8(*ptr);
			(*ptr)++;
			ctx->ps[idx] = B_HOST_TO_LENDIAN_INT32(val);
			break;
		case ATOM_ARG_WS:
			idx = U8(*ptr);
			(*ptr)++;
			switch(idx) {
				case ATOM_WS_QUOTIENT:
					gctx->divmul[0] = val;
					break;
				case ATOM_WS_REMAINDER:
					gctx->divmul[1] = val;
					break;
				case ATOM_WS_DATAPTR:
					gctx->data_block = val;
					break;
				case ATOM_WS_SHIFT:
					gctx->shift = val;
					break;
				case ATOM_WS_OR_MASK:
				case ATOM_WS_AND_MASK:
					break;
				case ATOM_WS_FB_WINDOW:
					gctx->fb_base = val;
					break;
				case ATOM_WS_ATTRIBUTES:
					gctx->io_attr = val;
					break;
				case ATOM_WS_REGPTR:
					gctx->reg_block = val;
					break;
				default:
					ctx->ws[idx] = val;
			}
			break;
		case ATOM_ARG_FB:
			idx = U8(*ptr);
			(*ptr)++;
			gctx->scratch[((gctx->fb_base + idx) / 4)] = val;
			return;
		case ATOM_ARG_PLL:
			idx = U8(*ptr);
			(*ptr)++;
			gctx->card->pll_write(idx, val);
			break;
		case ATOM_ARG_MC:
			idx = U8(*ptr);
			(*ptr)++;
			gctx->card->mc_write(idx, val);
			return;
	}
}


static void
atom_op_add(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, src, saved;
	int dptr = *ptr;
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	src = atom_get_src(ctx, attr, ptr);
	#ifdef TRACE_ATOM
	TRACE("%s: 0x%" B_PRIX32 " + 0x%" B_PRIX32 " is 0x%" B_PRIX32 "\n",
		__func__, dst, src, dst + src);
	#endif
	dst += src;
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void
atom_op_and(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, src, saved;
	int dptr = *ptr;
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	src = atom_get_src(ctx, attr, ptr);
	#ifdef TRACE_ATOM
	TRACE("%s: 0x%" B_PRIX32 " & 0x%" B_PRIX32 " is 0x%" B_PRIX32 "\n",
		__func__, src, dst, dst & src);
	#endif
	dst &= src;
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void
atom_op_beep(atom_exec_context *ctx, int *ptr, int arg)
{
	TRACE("%s: Quack!\n", __func__);
}


static void
atom_op_calltable(atom_exec_context *ctx, int *ptr, int arg)
{
	int idx = U8((*ptr)++);
	status_t result = B_OK;

	if (idx < ATOM_TABLE_NAMES_CNT)
		TRACE("%s: table: %s (%d)\n", __func__, atom_table_names[idx], idx);
	else
		TRACE("%s: table: unknown (%d)\n", __func__, idx);

	if (U16(ctx->ctx->cmd_table + 4 + 2 * idx)) {
		result = atom_execute_table_locked(ctx->ctx,
			idx, ctx->ps + ctx->ps_shift);
	}

	if (result != B_OK)
		ctx->abort = true;
}


static void
atom_op_clear(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 saved;
	int dptr = *ptr;
	attr &= 0x38;
	attr |= atom_def_dst[attr>>3]<<6;
	atom_get_dst(ctx, arg, attr, ptr, &saved, 0);
	TRACE("%s\n", __func__);
	atom_put_dst(ctx, arg, attr, &dptr, 0, saved);
}


static void
atom_op_compare(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, src;
	dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
	src = atom_get_src(ctx, attr, ptr);
	ctx->ctx->cs_equal = (dst == src);
	ctx->ctx->cs_above = (dst > src);
	TRACE("%s: 0x%" B_PRIX32 " %s 0x%" B_PRIX32 "\n", __func__,
		dst, ctx->ctx->cs_above ? ">" : "<=", src);
}


static void
atom_op_delay(atom_exec_context *ctx, int *ptr, int arg)
{
	bigtime_t count = U8((*ptr)++);
	if (arg == ATOM_UNIT_MICROSEC) {
		TRACE("%s: %" B_PRId64 " microseconds\n", __func__, count);
		// Microseconds
		snooze(count);
	} else {
		TRACE("%s: %" B_PRId64 " milliseconds\n", __func__, count);
		// Milliseconds
		snooze(count * 1000);
	}
}


static void
atom_op_div(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, src;
	dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
	src = atom_get_src(ctx, attr, ptr);
	if (src != 0) {
		ctx->ctx->divmul[0] = dst / src;
		ctx->ctx->divmul[1] = dst%src;
	} else {
		ctx->ctx->divmul[0] = 0;
		ctx->ctx->divmul[1] = 0;
	}
	#ifdef ATOM_TRACE
	TRACE("%s: 0x%" B_PRIX32 " / 0x%" B_PRIX32 " is 0x%" B_PRIX32
		" remander 0x%" B_PRIX32 "\n", __func__, dst, src,
		ctx->ctx->divmul[0], ctx->ctx->divmul[1]);
	#endif
}


static void
atom_op_eot(atom_exec_context *ctx, int *ptr, int arg)
{
	/* functionally, a nop */
}


static void
atom_op_jump(atom_exec_context *ctx, int *ptr, int arg)
{
	int execute = 0, target = U16(*ptr);
	(*ptr) += 2;
	switch(arg) {
		case ATOM_COND_ABOVE:
			execute = ctx->ctx->cs_above;
			break;
		case ATOM_COND_ABOVEOREQUAL:
			execute = ctx->ctx->cs_above || ctx->ctx->cs_equal;
			break;
		case ATOM_COND_ALWAYS:
			execute = 1;
			break;
		case ATOM_COND_BELOW:
			execute = !(ctx->ctx->cs_above || ctx->ctx->cs_equal);
			break;
		case ATOM_COND_BELOWOREQUAL:
			execute = !ctx->ctx->cs_above;
			break;
		case ATOM_COND_EQUAL:
			execute = ctx->ctx->cs_equal;
			break;
		case ATOM_COND_NOTEQUAL:
			execute = !ctx->ctx->cs_equal;
			break;
	}
	TRACE("%s: execute jump: %s; target: 0x%04X\n", __func__,
		execute? "yes" : "no", target);

	if (execute) {
		if (ctx->last_jump == (ctx->start + target)) {
			if (ctx->last_jump_count > 128) {
				TRACE("%s: DANGER! AtomBIOS stuck in infinite loop"
					" for more then 128 jumps... abort!\n", __func__);
				ctx->abort = true;
			} else {
				ctx->last_jump_count++;
			}
		} else {
			ctx->last_jump = ctx->start + target;
			ctx->last_jump_count = 1;
		}
		*ptr = ctx->start + target;
	}
}


static void
atom_op_mask(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, mask, src, saved;
	int dptr = *ptr;
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	mask = atom_get_src_direct(ctx, ((attr >> 3) & 7), ptr);
	src = atom_get_src(ctx, attr, ptr);
	dst &= mask;
	dst |= src;
	TRACE("%s: src: 0x%" B_PRIX32 " mask 0x%" B_PRIX32 " is 0x%" B_PRIX32 "\n",
		__func__, src, mask, dst);
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void
atom_op_move(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 src, saved;
	int dptr = *ptr;
	if (((attr >> 3) & 7) != ATOM_SRC_DWORD)
		atom_get_dst(ctx, arg, attr, ptr, &saved, 0);
	else {
		atom_skip_dst(ctx, arg, attr, ptr);
		saved = 0xCDCDCDCD;
	}
	src = atom_get_src(ctx, attr, ptr);
	TRACE("%s: src: 0x%" B_PRIX32 "; saved: 0x%" B_PRIX32 "\n",
		__func__, src, saved);
	atom_put_dst(ctx, arg, attr, &dptr, src, saved);
}


static void
atom_op_mul(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, src;
	dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
	src = atom_get_src(ctx, attr, ptr);
	ctx->ctx->divmul[0] = dst * src;
	TRACE("%s: 0x%" B_PRIX32 " * 0x%" B_PRIX32 " is 0x%" B_PRIX32 "\n",
		__func__, dst, src, ctx->ctx->divmul[0]);
}


static void
atom_op_nop(atom_exec_context *ctx, int *ptr, int arg)
{
	/* nothing */
}


static void
atom_op_or(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, src, saved;
	int dptr = *ptr;
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	src = atom_get_src(ctx, attr, ptr);
	#ifdef ATOM_TRACE
	TRACE("%s: 0x%" B_PRIX32 " | 0x%" B_PRIX32 " is 0x%" B_PRIX32 "\n",
		__func__, dst, src, dst | src);
	#endif
	dst |= src;
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void
atom_op_postcard(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 val = U8((*ptr)++);
	TRACE("%s: POST card output: 0x%" B_PRIX8 "\n", __func__, val);
}


static void atom_op_repeat(atom_exec_context *ctx, int *ptr, int arg)
{
	TRACE("%s: unimplemented!\n", __func__);
}


static void
atom_op_restorereg(atom_exec_context *ctx, int *ptr, int arg)
{
	TRACE("%s: unimplemented!\n", __func__);
}


static void
atom_op_savereg(atom_exec_context *ctx, int *ptr, int arg)
{
	TRACE("%s: unimplemented!\n", __func__);
}


static void
atom_op_setdatablock(atom_exec_context *ctx, int *ptr, int arg)
{
	int idx = U8(*ptr);
	(*ptr)++;
	TRACE("%s: block: %d\n", __func__, idx);
	if (!idx)
		ctx->ctx->data_block = 0;
	else if (idx == 255)
		ctx->ctx->data_block = ctx->start;
	else
		ctx->ctx->data_block = U16(ctx->ctx->data_table + 4 + 2 * idx);
}


static void
atom_op_setfbbase(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	ctx->ctx->fb_base = atom_get_src(ctx, attr, ptr);
	TRACE("%s: fb_base: 0x%" B_PRIX32 "\n", __func__, ctx->ctx->fb_base);
}


static void
atom_op_setport(atom_exec_context *ctx, int *ptr, int arg)
{
	int port;
	switch(arg) {
		case ATOM_PORT_ATI:
			port = U16(*ptr);
			if (port < ATOM_IO_NAMES_CNT) {
				TRACE("%s: port: %d (%s)\n", __func__,
					port, atom_io_names[port]);
			} else
				TRACE("%s: port: %d\n", __func__, port);
			if (!port)
				ctx->ctx->io_mode = ATOM_IO_MM;
			else
				ctx->ctx->io_mode = ATOM_IO_IIO | port;
			(*ptr) += 2;
			break;
		case ATOM_PORT_PCI:
			ctx->ctx->io_mode = ATOM_IO_PCI;
			(*ptr)++;
			break;
		case ATOM_PORT_SYSIO:
			ctx->ctx->io_mode = ATOM_IO_SYSIO;
			(*ptr)++;
			break;
	}
}


static void
atom_op_setregblock(atom_exec_context *ctx, int *ptr, int arg)
{
	ctx->ctx->reg_block = U16(*ptr);
	(*ptr)+=2;
}


static void atom_op_shift_left(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++), shift;
	uint32 saved, dst;
	int dptr = *ptr;
	attr &= 0x38;
	attr |= atom_def_dst[attr >> 3] << 6;
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	shift = atom_get_src_direct(ctx, ATOM_SRC_BYTE0, ptr);
	#ifdef ATOM_TRACE
	TRACE("%s: 0x%" B_PRIX32 " << %" B_PRId8 " is 0X%" B_PRIX32 "\n",
		__func__, dst, shift, dst << shift);
	#endif
	dst <<= shift;
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void atom_op_shift_right(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++), shift;
	uint32 saved, dst;
	int dptr = *ptr;
	attr &= 0x38;
	attr |= atom_def_dst[attr >> 3] << 6;
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	shift = atom_get_src_direct(ctx, ATOM_SRC_BYTE0, ptr);
	#ifdef ATOM_TRACE
	TRACE("%s: 0x%" B_PRIX32 " >> %" B_PRId8 " is 0X%" B_PRIX32 "\n",
		__func__, dst, shift, dst << shift);
	#endif
	dst >>= shift;
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void atom_op_shl(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++), shift;
	uint32 saved, dst;
	int dptr = *ptr;
	uint32 dst_align = atom_dst_to_src[(attr >> 3) & 7][(attr >> 6) & 3];
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	/* op needs to full dst value */
	dst = saved;
	shift = atom_get_src(ctx, attr, ptr);
	#ifdef ATOM_TRACE
	TRACE("%s: 0x%" B_PRIX32 " << %" B_PRId8 " is 0X%" B_PRIX32 "\n",
		__func__, dst, shift, dst << shift);
	#endif
	dst <<= shift;
	dst &= atom_arg_mask[dst_align];
	dst >>= atom_arg_shift[dst_align];
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void atom_op_shr(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++), shift;
	uint32 saved, dst;
	int dptr = *ptr;
	uint32 dst_align = atom_dst_to_src[(attr >> 3) & 7][(attr >> 6) & 3];
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	/* op needs to full dst value */
	dst = saved;
	shift = atom_get_src(ctx, attr, ptr);
	#ifdef ATOM_TRACE
	TRACE("%s: 0x%" B_PRIX32 " >> %" B_PRId8 " is 0X%" B_PRIX32 "\n",
		__func__, dst, shift, dst << shift);
	#endif
	dst >>= shift;
	dst &= atom_arg_mask[dst_align];
	dst >>= atom_arg_shift[dst_align];
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void
atom_op_sub(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, src, saved;
	int dptr = *ptr;
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	src = atom_get_src(ctx, attr, ptr);
	#ifdef TRACE_ATOM
	TRACE("%s: 0x%" B_PRIX32 " - 0x%" B_PRIX32 " is 0x%" B_PRIX32 "\n",
		__func__, dst, src, dst - src);
	#endif
	dst -= src;
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void
atom_op_switch(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 src, val, target;
	TRACE("%s: switch start\n", __func__);
	src = atom_get_src(ctx, attr, ptr);
	while (U16(*ptr) != ATOM_CASE_END)
	if (U8(*ptr) == ATOM_CASE_MAGIC) {
		(*ptr)++;
		TRACE("%s: switch case\n", __func__);
		val = atom_get_src(ctx, (attr & 0x38) | ATOM_ARG_IMM, ptr);
		target = U16(*ptr);
		if (val == src) {
			*ptr = ctx->start + target;
			return;
		}
		(*ptr) += 2;
	} else {
		TRACE("%s: ERROR bad case\n", __func__);
		return;
	}
	(*ptr) += 2;
}


static void
atom_op_test(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, src;
	dst = atom_get_dst(ctx, arg, attr, ptr, NULL, 1);
	src = atom_get_src(ctx, attr, ptr);
	ctx->ctx->cs_equal = ((dst & src) == 0);
	TRACE("%s: 0x%" B_PRIX32 " and 0x%" B_PRIX32 " are %s\n", __func__,
		dst, src, ctx->ctx->cs_equal ? "EQ" : "NE");
}


static void
atom_op_xor(atom_exec_context *ctx, int *ptr, int arg)
{
	uint8 attr = U8((*ptr)++);
	uint32 dst, src, saved;
	int dptr = *ptr;
	dst = atom_get_dst(ctx, arg, attr, ptr, &saved, 1);
	src = atom_get_src(ctx, attr, ptr);
	#ifdef ATOM_TRACE
	TRACE("%s: 0x%" B_PRIX32 " ^ 0X%" B_PRIX32 " is " B_PRIX32 "\n",
		__func__, dst, src, dst ^ src);
	#endif
	dst ^= src;
	atom_put_dst(ctx, arg, attr, &dptr, dst, saved);
}


static void
atom_op_debug(atom_exec_context *ctx, int *ptr, int arg)
{
	TRACE("%s: unimplemented!\n", __func__);
}


static struct {
	void (*func)(atom_exec_context *, int *, int);
	int arg;
} opcode_table[ATOM_OP_CNT] = {
	{ NULL, 0 },
	{ atom_op_move, ATOM_ARG_REG },
	{ atom_op_move, ATOM_ARG_PS },
	{ atom_op_move, ATOM_ARG_WS },
	{ atom_op_move, ATOM_ARG_FB },
	{ atom_op_move, ATOM_ARG_PLL },
	{ atom_op_move, ATOM_ARG_MC },
	{ atom_op_and, ATOM_ARG_REG },
	{ atom_op_and, ATOM_ARG_PS },
	{ atom_op_and, ATOM_ARG_WS },
	{ atom_op_and, ATOM_ARG_FB },
	{ atom_op_and, ATOM_ARG_PLL },
	{ atom_op_and, ATOM_ARG_MC },
	{ atom_op_or, ATOM_ARG_REG },
	{ atom_op_or, ATOM_ARG_PS },
	{ atom_op_or, ATOM_ARG_WS },
	{ atom_op_or, ATOM_ARG_FB },
	{ atom_op_or, ATOM_ARG_PLL },
	{ atom_op_or, ATOM_ARG_MC },
	{ atom_op_shift_left, ATOM_ARG_REG },
	{ atom_op_shift_left, ATOM_ARG_PS },
	{ atom_op_shift_left, ATOM_ARG_WS },
	{ atom_op_shift_left, ATOM_ARG_FB },
	{ atom_op_shift_left, ATOM_ARG_PLL },
	{ atom_op_shift_left, ATOM_ARG_MC },
	{ atom_op_shift_right, ATOM_ARG_REG },
	{ atom_op_shift_right, ATOM_ARG_PS },
	{ atom_op_shift_right, ATOM_ARG_WS },
	{ atom_op_shift_right, ATOM_ARG_FB },
	{ atom_op_shift_right, ATOM_ARG_PLL },
	{ atom_op_shift_right, ATOM_ARG_MC },
	{ atom_op_mul, ATOM_ARG_REG },
	{ atom_op_mul, ATOM_ARG_PS },
	{ atom_op_mul, ATOM_ARG_WS },
	{ atom_op_mul, ATOM_ARG_FB },
	{ atom_op_mul, ATOM_ARG_PLL },
	{ atom_op_mul, ATOM_ARG_MC },
	{ atom_op_div, ATOM_ARG_REG },
	{ atom_op_div, ATOM_ARG_PS },
	{ atom_op_div, ATOM_ARG_WS },
	{ atom_op_div, ATOM_ARG_FB },
	{ atom_op_div, ATOM_ARG_PLL },
	{ atom_op_div, ATOM_ARG_MC },
	{ atom_op_add, ATOM_ARG_REG },
	{ atom_op_add, ATOM_ARG_PS },
	{ atom_op_add, ATOM_ARG_WS },
	{ atom_op_add, ATOM_ARG_FB },
	{ atom_op_add, ATOM_ARG_PLL },
	{ atom_op_add, ATOM_ARG_MC },
	{ atom_op_sub, ATOM_ARG_REG },
	{ atom_op_sub, ATOM_ARG_PS },
	{ atom_op_sub, ATOM_ARG_WS },
	{ atom_op_sub, ATOM_ARG_FB },
	{ atom_op_sub, ATOM_ARG_PLL },
	{ atom_op_sub, ATOM_ARG_MC },
	{ atom_op_setport, ATOM_PORT_ATI },
	{ atom_op_setport, ATOM_PORT_PCI },
	{ atom_op_setport, ATOM_PORT_SYSIO },
	{ atom_op_setregblock, 0 },
	{ atom_op_setfbbase, 0 },
	{ atom_op_compare, ATOM_ARG_REG },
	{ atom_op_compare, ATOM_ARG_PS },
	{ atom_op_compare, ATOM_ARG_WS },
	{ atom_op_compare, ATOM_ARG_FB },
	{ atom_op_compare, ATOM_ARG_PLL },
	{ atom_op_compare, ATOM_ARG_MC },
	{ atom_op_switch, 0 },
	{ atom_op_jump, ATOM_COND_ALWAYS },
	{ atom_op_jump, ATOM_COND_EQUAL },
	{ atom_op_jump, ATOM_COND_BELOW },
	{ atom_op_jump, ATOM_COND_ABOVE },
	{ atom_op_jump, ATOM_COND_BELOWOREQUAL },
	{ atom_op_jump, ATOM_COND_ABOVEOREQUAL },
	{ atom_op_jump, ATOM_COND_NOTEQUAL },
	{ atom_op_test, ATOM_ARG_REG },
	{ atom_op_test, ATOM_ARG_PS },
	{ atom_op_test, ATOM_ARG_WS },
	{ atom_op_test, ATOM_ARG_FB },
	{ atom_op_test, ATOM_ARG_PLL },
	{ atom_op_test, ATOM_ARG_MC },
	{ atom_op_delay, ATOM_UNIT_MILLISEC },
	{ atom_op_delay, ATOM_UNIT_MICROSEC },
	{ atom_op_calltable, 0 },
	{ atom_op_repeat, 0 },
	{ atom_op_clear, ATOM_ARG_REG },
	{ atom_op_clear, ATOM_ARG_PS },
	{ atom_op_clear, ATOM_ARG_WS },
	{ atom_op_clear, ATOM_ARG_FB },
	{ atom_op_clear, ATOM_ARG_PLL },
	{ atom_op_clear, ATOM_ARG_MC },
	{ atom_op_nop, 0 },
	{ atom_op_eot, 0 },
	{ atom_op_mask, ATOM_ARG_REG },
	{ atom_op_mask, ATOM_ARG_PS },
	{ atom_op_mask, ATOM_ARG_WS },
	{ atom_op_mask, ATOM_ARG_FB },
	{ atom_op_mask, ATOM_ARG_PLL },
	{ atom_op_mask, ATOM_ARG_MC },
	{ atom_op_postcard, 0 },
	{ atom_op_beep, 0 },
	{ atom_op_savereg, 0 },
	{ atom_op_restorereg, 0 },
	{ atom_op_setdatablock, 0 },
	{ atom_op_xor, ATOM_ARG_REG },
	{ atom_op_xor, ATOM_ARG_PS },
	{ atom_op_xor, ATOM_ARG_WS },
	{ atom_op_xor, ATOM_ARG_FB },
	{ atom_op_xor, ATOM_ARG_PLL },
	{ atom_op_xor, ATOM_ARG_MC },
	{ atom_op_shl, ATOM_ARG_REG },
	{ atom_op_shl, ATOM_ARG_PS },
	{ atom_op_shl, ATOM_ARG_WS },
	{ atom_op_shl, ATOM_ARG_FB },
	{ atom_op_shl, ATOM_ARG_PLL },
	{ atom_op_shl, ATOM_ARG_MC },
	{ atom_op_shr, ATOM_ARG_REG },
	{ atom_op_shr, ATOM_ARG_PS },
	{ atom_op_shr, ATOM_ARG_WS },
	{ atom_op_shr, ATOM_ARG_FB },
	{ atom_op_shr, ATOM_ARG_PLL },
	{ atom_op_shr, ATOM_ARG_MC },
	{ atom_op_debug, 0 },
};


status_t
atom_execute_table_locked(atom_context *ctx, int index, uint32 * params)
{
	int base = CU16(ctx->cmd_table + 4 + 2 * index);
	int len, ws, ps, ptr;
	unsigned char op;
	atom_exec_context ectx;

	if (!base)
		return B_ERROR;

	len = CU16(base + ATOM_CT_SIZE_PTR);
	ws = CU8(base + ATOM_CT_WS_PTR);
	ps = CU8(base + ATOM_CT_PS_PTR) & ATOM_CT_PS_MASK;
	ptr = base + ATOM_CT_CODE_PTR;

	ectx.ctx = ctx;
	ectx.ps_shift = ps / 4;
	ectx.start = base;
	ectx.ps = params;
	ectx.abort = false;
	ectx.last_jump = 0;
	ectx.last_jump_count = 0;
	if (ws)
		ectx.ws = (uint32*)malloc(4 * ws);
	else
		ectx.ws = NULL;

	debug_depth++;
	while (1) {
		op = CU8(ptr++);
		if (op < ATOM_OP_NAMES_CNT) {
			TRACE("%s: %s (0x%" B_PRIX16 ")\n", __func__,
				atom_op_names[op], ptr - 1);
		} else
			TRACE("%s: unknown (0x%" B_PRIX16 ")\n", __func__, ptr - 1);

		if (ectx.abort == true) {
			TRACE("AtomBios parser was aborted executing (0x%" B_PRIX16 ")\n",
				ptr - 1);
			free(ectx.ws);
			return B_ERROR;
		}

		if (op < ATOM_OP_CNT && op > 0)
			opcode_table[op].func(&ectx, &ptr, opcode_table[op].arg);
		else
			break;

		if (op == ATOM_OP_EOT)
			break;
	}
	debug_depth--;

	free(ectx.ws);
	return B_OK;
}


status_t
atom_execute_table(atom_context *ctx, int index, uint32 *params)
{
	if (acquire_sem_etc(ctx->exec_sem, 1, B_RELATIVE_TIMEOUT, 5000000)
		!= B_NO_ERROR) {
		TRACE("%s: Timeout to obtain semaphore!\n", __func__);
		return B_ERROR;
	}
	/* reset reg block */
	ctx->reg_block = 0;
	/* reset fb window */
	ctx->fb_base = 0;
	/* reset io mode */
	ctx->io_mode = ATOM_IO_MM;
	status_t result = atom_execute_table_locked(ctx, index, params);

	release_sem(ctx->exec_sem);
	return result;
}


static int atom_iio_len[] = { 1, 2, 3, 3, 3, 3, 4, 4, 4, 3 };


static void
atom_index_iio(atom_context *ctx, int base)
{
	ctx->iio = (uint16*)malloc(2 * 256);
	while (CU8(base) == ATOM_IIO_START) {
		ctx->iio[CU8(base + 1)] = base + 2;
		base += 2;
		while (CU8(base) != ATOM_IIO_END)
			base += atom_iio_len[CU8(base)];
		base += 3;
	}
}


atom_context*
atom_parse(card_info *card, void *bios)
{
	atom_context *ctx = (atom_context*)malloc(sizeof(atom_context));

	if (ctx == NULL) {
		TRACE("%s: Error: No memory for atom_context mapping\n", __func__);
		return NULL;
	}

	ctx->card = card;
	ctx->bios = bios;

	if (CU16(0) != ATOM_BIOS_MAGIC) {
		TRACE("Invalid BIOS magic.\n");
		free(ctx);
		return NULL;
	}
	if (strncmp(CSTR(ATOM_ATI_MAGIC_PTR), ATOM_ATI_MAGIC,
		strlen(ATOM_ATI_MAGIC))) {
		TRACE("Invalid ATI magic.\n");
		free(ctx);
		return NULL;
	}

	int base = CU16(ATOM_ROM_TABLE_PTR);
	if (strncmp(CSTR(base + ATOM_ROM_MAGIC_PTR), ATOM_ROM_MAGIC,
		strlen(ATOM_ROM_MAGIC))) {
		TRACE("Invalid ATOM magic.\n");
		free(ctx);
		return NULL;
	}

	ctx->cmd_table = CU16(base + ATOM_ROM_CMD_PTR);
	ctx->data_table = CU16(base + ATOM_ROM_DATA_PTR);
	atom_index_iio(ctx, CU16(ctx->data_table + ATOM_DATA_IIO_PTR) + 4);

	char *str = CSTR(CU16(base + ATOM_ROM_MSG_PTR));
	while (*str && ((*str == '\n') || (*str == '\r')))
		str++;

	TRACE("ATOM BIOS: %s", str);

	return ctx;
}


status_t
atom_asic_init(atom_context *ctx)
{
	int hwi = CU16(ctx->data_table + ATOM_DATA_FWI_PTR);
	uint32 ps[16];
	memset(ps, 0, 64);

	ps[0] = CU32(hwi + ATOM_FWI_DEFSCLK_PTR);
	ps[1] = CU32(hwi + ATOM_FWI_DEFMCLK_PTR);
	if (!ps[0] || !ps[1])
		return B_ERROR;

	if (!CU16(ctx->cmd_table + 4 + 2 * ATOM_CMD_INIT))
		return B_ERROR;

	return atom_execute_table(ctx, ATOM_CMD_INIT, ps);
}


void
atom_destroy(atom_context *ctx)
{
	if (ctx != NULL) {
		free(ctx->iio);
		delete_sem(ctx->exec_sem);
	}

	free(ctx);
}


status_t
atom_parse_data_header(atom_context *ctx, int index, uint16 *size,
	uint8 *frev, uint8 *crev, uint16 *data_start)
{
	int offset = index * 2 + 4;
	int idx = CU16(ctx->data_table + offset);
	uint16 *mdt = (uint16 *)ctx->bios + ctx->data_table + 4;

	if (!mdt[index])
		return B_ERROR;

	if (size)
		*size = CU16(idx);
	if (frev)
		*frev = CU8(idx + 2);
	if (crev)
		*crev = CU8(idx + 3);
	*data_start = idx;
	return B_OK;
}


status_t
atom_parse_cmd_header(atom_context *ctx, int index, uint8 * frev,
	uint8 * crev)
{
	int offset = index * 2 + 4;
	int idx = CU16(ctx->cmd_table + offset);
	uint16 *mct = (uint16 *)ctx->bios + ctx->cmd_table + 4;

	if (!mct[index])
		return B_ERROR;

	if (frev)
		*frev = CU8(idx + 2);
	if (crev)
		*crev = CU8(idx + 3);
	return B_OK;
}

