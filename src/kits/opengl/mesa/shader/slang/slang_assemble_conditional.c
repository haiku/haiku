/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file slang_assemble_conditional.c
 * slang condtional expressions assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_assemble.h"
#include "slang_compile.h"

/*
 * _slang_assemble_logicaland()
 *
 * and:
 *    <left-expression>
 *    jumpz zero
 *    <right-expression>
 *    jump end
 *    zero:
 *    push 0
 * end:
 */

GLboolean _slang_assemble_logicaland (slang_assemble_ctx *A, slang_operation *op)
{
	GLuint zero_jump, end_jump;

	/* evaluate left expression */
	if (!_slang_assemble_operation (A, &op->children[0], slang_ref_forbid))
		return GL_FALSE;

	/* jump to pushing 0 if not true */
	zero_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump_if_zero))
		return GL_FALSE;

	/* evaluate right expression */
	if (!_slang_assemble_operation (A, &op->children[1], slang_ref_forbid))
		return GL_FALSE;

	/* jump to the end of the expression */
	end_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* push 0 on stack */
	A->file->code[zero_jump].param[0] = A->file->count;
	if (!slang_assembly_file_push_literal (A->file, slang_asm_bool_push, (GLfloat) 0))
		return GL_FALSE;

	/* the end of the expression */
	A->file->code[end_jump].param[0] = A->file->count;

	return GL_TRUE;
}

/*
 * _slang_assemble_logicalor()
 *
 * or:
 *    <left-expression>
 *    jumpz right
 *    push 1
 *    jump end
 * right:
 *    <right-expression>
 * end:
 */

GLboolean _slang_assemble_logicalor (slang_assemble_ctx *A, slang_operation *op)
{
	GLuint right_jump, end_jump;

	/* evaluate left expression */
	if (!_slang_assemble_operation (A, &op->children[0], slang_ref_forbid))
		return GL_FALSE;

	/* jump to evaluation of right expression if not true */
	right_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump_if_zero))
		return GL_FALSE;

	/* push 1 on stack */
	if (!slang_assembly_file_push_literal (A->file, slang_asm_bool_push, (GLfloat) 1))
		return GL_FALSE;

	/* jump to the end of the expression */
	end_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* evaluate right expression */
	A->file->code[right_jump].param[0] = A->file->count;
	if (!_slang_assemble_operation (A, &op->children[1], slang_ref_forbid))
		return GL_FALSE;

	/* the end of the expression */
	A->file->code[end_jump].param[0] = A->file->count;

	return GL_TRUE;
}

/*
 * _slang_assemble_select()
 *
 * select:
 *    <condition-expression>
 *    jumpz false
 *    <true-expression>
 *    jump end
 * false:
 *    <false-expression>
 * end:
 */

GLboolean _slang_assemble_select (slang_assemble_ctx *A, slang_operation *op)
{
	GLuint cond_jump, end_jump;

	/* execute condition expression */
	if (!_slang_assemble_operation (A, &op->children[0], slang_ref_forbid))
		return GL_FALSE;

	/* jump to false expression if not true */
	cond_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump_if_zero))
		return GL_FALSE;

	/* execute true expression */
	if (!_slang_assemble_operation (A, &op->children[1], slang_ref_forbid))
		return GL_FALSE;

	/* jump to the end of the expression */
	end_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* resolve false point */
	A->file->code[cond_jump].param[0] = A->file->count;

	/* execute false expression */
	if (!_slang_assemble_operation (A, &op->children[2], slang_ref_forbid))
		return GL_FALSE;

	/* resolve the end of the expression */
	A->file->code[end_jump].param[0] = A->file->count;

	return GL_TRUE;
}

/*
 * _slang_assemble_for()
 *
 * for:
 *    <init-statement>
 *    jump start
 * break:
 *    jump end
 * continue:
 *    <loop-increment>
 * start:
 *    <condition-statement>
 *    jumpz end
 *    <loop-body>
 *    jump continue
 * end:
 */

GLboolean _slang_assemble_for (slang_assemble_ctx *A, slang_operation *op)
{
	GLuint start_jump, end_jump, cond_jump;
	GLuint break_label, cont_label;
	slang_assembly_flow_control save_flow = A->flow;

	/* execute initialization statement */
	if (!_slang_assemble_operation (A, &op->children[0], slang_ref_forbid/*slang_ref_freelance*/))
		return GL_FALSE;
	if (!_slang_cleanup_stack (A, &op->children[0]))
		return GL_FALSE;

	/* skip the "go to the end of the loop" and loop-increment statements */
	start_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* go to the end of the loop - break statements are directed here */
	break_label = A->file->count;
	end_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* resolve the beginning of the loop - continue statements are directed here */
	cont_label = A->file->count;

	/* execute loop-increment statement */
	if (!_slang_assemble_operation (A, &op->children[2], slang_ref_forbid/*slang_ref_freelance*/))
		return GL_FALSE;
	if (!_slang_cleanup_stack (A, &op->children[2]))
		return GL_FALSE;

	/* resolve the condition point */
	A->file->code[start_jump].param[0] = A->file->count;

	/* execute condition statement */
	if (!_slang_assemble_operation (A, &op->children[1], slang_ref_forbid))
		return GL_FALSE;

	/* jump to the end of the loop if not true */
	cond_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump_if_zero))
		return GL_FALSE;

	/* execute loop body */
	A->flow.loop_start = cont_label;
	A->flow.loop_end = break_label;
	if (!_slang_assemble_operation (A, &op->children[3], slang_ref_forbid/*slang_ref_freelance*/))
		return GL_FALSE;
	if (!_slang_cleanup_stack (A, &op->children[3]))
		return GL_FALSE;
	A->flow = save_flow;

	/* go to the beginning of the loop */
	if (!slang_assembly_file_push_label (A->file, slang_asm_jump, cont_label))
		return GL_FALSE;

	/* resolve the end of the loop */
	A->file->code[end_jump].param[0] = A->file->count;
	A->file->code[cond_jump].param[0] = A->file->count;

	return GL_TRUE;
}

/*
 * _slang_assemble_do()
 *
 * do:
 *    jump start
 * break:
 *    jump end
 * continue:
 *    jump condition
 * start:
 *    <loop-body>
 * condition:
 *    <condition-statement>
 *    jumpz end
 *    jump start
 * end:
 */

GLboolean _slang_assemble_do (slang_assemble_ctx *A, slang_operation *op)
{
	GLuint skip_jump, end_jump, cont_jump, cond_jump;
	GLuint break_label, cont_label;
	slang_assembly_flow_control save_flow = A->flow;

	/* skip the "go to the end of the loop" and "go to condition" statements */
	skip_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* go to the end of the loop - break statements are directed here */
	break_label = A->file->count;
	end_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* go to condition - continue statements are directed here */
	cont_label = A->file->count;
	cont_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* resolve the beginning of the loop */
	A->file->code[skip_jump].param[0] = A->file->count;

	/* execute loop body */
	A->flow.loop_start = cont_label;
	A->flow.loop_end = break_label;
	if (!_slang_assemble_operation (A, &op->children[0], slang_ref_forbid/*slang_ref_freelance*/))
		return GL_FALSE;
	if (!_slang_cleanup_stack (A, &op->children[0]))
		return GL_FALSE;
	A->flow = save_flow;

	/* resolve condition point */
	A->file->code[cont_jump].param[0] = A->file->count;

	/* execute condition statement */
	if (!_slang_assemble_operation (A, &op->children[1], slang_ref_forbid))
		return GL_FALSE;

	/* jump to the end of the loop if not true */
	cond_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump_if_zero))
		return GL_FALSE;

	/* jump to the beginning of the loop */
	if (!slang_assembly_file_push_label (A->file, slang_asm_jump, A->file->code[skip_jump].param[0]))
		return GL_FALSE;

	/* resolve the end of the loop */
	A->file->code[end_jump].param[0] = A->file->count;
	A->file->code[cond_jump].param[0] = A->file->count;

	return GL_TRUE;
}

/*
 * _slang_assemble_while()
 *
 * while:
 *    jump continue
 * break:
 *    jump end
 * continue:
 *    <condition-statement>
 *    jumpz end
 *    <loop-body>
 *    jump continue
 * end:
 */

GLboolean _slang_assemble_while (slang_assemble_ctx *A, slang_operation *op)
{
	GLuint skip_jump, end_jump, cond_jump;
	GLuint break_label;
	slang_assembly_flow_control save_flow = A->flow;

	/* skip the "go to the end of the loop" statement */
	skip_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* go to the end of the loop - break statements are directed here */
	break_label = A->file->count;
	end_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* resolve the beginning of the loop - continue statements are directed here */
	A->file->code[skip_jump].param[0] = A->file->count;

	/* execute condition statement */
	if (!_slang_assemble_operation (A, &op->children[0], slang_ref_forbid))
		return GL_FALSE;

	/* jump to the end of the loop if not true */
	cond_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump_if_zero))
		return GL_FALSE;

	/* execute loop body */
	A->flow.loop_start = A->file->code[skip_jump].param[0];
	A->flow.loop_end = break_label;
	if (!_slang_assemble_operation (A, &op->children[1], slang_ref_forbid/*slang_ref_freelance*/))
		return GL_FALSE;
	if (!_slang_cleanup_stack (A, &op->children[1]))
		return GL_FALSE;
	A->flow = save_flow;

	/* jump to the beginning of the loop */
	if (!slang_assembly_file_push_label (A->file, slang_asm_jump, A->file->code[skip_jump].param[0]))
		return GL_FALSE;

	/* resolve the end of the loop */
	A->file->code[end_jump].param[0] = A->file->count;
	A->file->code[cond_jump].param[0] = A->file->count;

	return GL_TRUE;
}

/*
 * _slang_assemble_if()
 *
 * if:
 *    <condition-statement>
 *    jumpz else
 *    <true-statement>
 *    jump end
 * else:
 *    <false-statement>
 * end:
 */

GLboolean _slang_assemble_if (slang_assemble_ctx *A, slang_operation *op)
{
	GLuint cond_jump, else_jump;

	/* execute condition statement */
	if (!_slang_assemble_operation (A, &op->children[0], slang_ref_forbid))
		return GL_FALSE;

	/* jump to false-statement if not true */
	cond_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump_if_zero))
		return GL_FALSE;

	/* execute true-statement */
	if (!_slang_assemble_operation (A, &op->children[1], slang_ref_forbid/*slang_ref_freelance*/))
		return GL_FALSE;
	if (!_slang_cleanup_stack (A, &op->children[1]))
		return GL_FALSE;

	/* skip if-false statement */
	else_jump = A->file->count;
	if (!slang_assembly_file_push (A->file, slang_asm_jump))
		return GL_FALSE;

	/* resolve start of false-statement */
	A->file->code[cond_jump].param[0] = A->file->count;

	/* execute false-statement */
	if (!_slang_assemble_operation (A, &op->children[2], slang_ref_forbid/*slang_ref_freelance*/))
		return GL_FALSE;
	if (!_slang_cleanup_stack (A, &op->children[2]))
		return GL_FALSE;

	/* resolve end of if-false statement */
	A->file->code[else_jump].param[0] = A->file->count;

	return GL_TRUE;
}

