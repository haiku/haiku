/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
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

#ifndef SLANG_COMPILE_OPERATION_H
#define SLANG_COMPILE_OPERATION_H

#if defined __cplusplus
extern "C" {
#endif


/**
 * Types of slang operations.
 * These are the types of the AST (abstract syntax tree) nodes.
 * [foo] indicates a sub-tree or reference to another type of node
 */
typedef enum slang_operation_type_
{
   slang_oper_none,
   slang_oper_block_no_new_scope,       /* "{" sequence "}" */
   slang_oper_block_new_scope,  /* "{" sequence "}" */
   slang_oper_variable_decl,    /* [type] [var] or [var] = [expr] */
   slang_oper_asm,
   slang_oper_break,            /* "break" statement */
   slang_oper_continue,         /* "continue" statement */
   slang_oper_discard,          /* "discard" (kill fragment) statement */
   slang_oper_return,           /* "return" [expr]  */
   slang_oper_expression,       /* [expr] */
   slang_oper_if,               /* "if" [0] then [1] else [2] */
   slang_oper_while,            /* "while" [cond] [body] */
   slang_oper_do,               /* "do" [body] "while" [cond] */
   slang_oper_for,              /* "for" [init] [while] [incr] [body] */
   slang_oper_void,             /* nop */
   slang_oper_literal_bool,     /* "true" or "false" */
   slang_oper_literal_int,      /* integer literal */
   slang_oper_literal_float,    /* float literal */
   slang_oper_identifier,       /* var name, func name, etc */
   slang_oper_sequence,         /* [expr] "," [expr] "," etc */
   slang_oper_assign,           /* [var] "=" [expr] */
   slang_oper_addassign,        /* [var] "+=" [expr] */
   slang_oper_subassign,        /* [var] "-=" [expr] */
   slang_oper_mulassign,        /* [var] "*=" [expr] */
   slang_oper_divassign,        /* [var] "/=" [expr] */
   /*slang_oper_modassign, */
   /*slang_oper_lshassign, */
   /*slang_oper_rshassign, */
   /*slang_oper_orassign, */
   /*slang_oper_xorassign, */
   /*slang_oper_andassign, */
   slang_oper_select,           /* [expr] "?" [expr] ":" [expr] */
   slang_oper_logicalor,        /* [expr] "||" [expr] */
   slang_oper_logicalxor,       /* [expr] "^^" [expr] */
   slang_oper_logicaland,       /* [expr] "&&" [expr] */
   /*slang_oper_bitor, */
   /*slang_oper_bitxor, */
   /*slang_oper_bitand, */
   slang_oper_equal,            /* [expr] "==" [expr] */
   slang_oper_notequal,         /* [expr] "!=" [expr] */
   slang_oper_less,             /* [expr] "<" [expr] */
   slang_oper_greater,          /* [expr] ">" [expr] */
   slang_oper_lessequal,        /* [expr] "<=" [expr] */
   slang_oper_greaterequal,     /* [expr] ">=" [expr] */
   /*slang_oper_lshift, */
   /*slang_oper_rshift, */
   slang_oper_add,              /* [expr] "+" [expr] */
   slang_oper_subtract,         /* [expr] "-" [expr] */
   slang_oper_multiply,         /* [expr] "*" [expr] */
   slang_oper_divide,           /* [expr] "/" [expr] */
   /*slang_oper_modulus, */
   slang_oper_preincrement,     /* "++" [var] */
   slang_oper_predecrement,     /* "--" [var] */
   slang_oper_plus,             /* "-" [expr] */
   slang_oper_minus,            /* "+" [expr] */
   /*slang_oper_complement, */
   slang_oper_not,              /* "!" [expr] */
   slang_oper_subscript,        /* [expr] "[" [expr] "]" */
   slang_oper_call,             /* [func name] [param] [param] [...] */
   slang_oper_field,            /* i.e.: ".next" or ".xzy" or ".xxx" etc */
   slang_oper_postincrement,    /* [var] "++" */
   slang_oper_postdecrement     /* [var] "--" */
} slang_operation_type;


/**
 * A slang_operation is basically a compiled instruction (such as assignment,
 * a while-loop, a conditional, a multiply, a function call, etc).
 * The AST (abstract syntax tree) is built from these nodes.
 * NOTE: This structure could have been implemented as a union of simpler
 * structs which would correspond to the operation types above.
 */
typedef struct slang_operation_
{
   slang_operation_type type;
   struct slang_operation_ *children;
   GLuint num_children;
   GLfloat literal;            /**< Used for float, int and bool values */
   slang_atom a_id;            /**< type: asm, identifier, call, field */
   slang_variable_scope *locals;       /**< local vars for scope */
} slang_operation;


extern GLboolean
slang_operation_construct(slang_operation *);

extern void
slang_operation_destruct(slang_operation *);

extern GLboolean
slang_operation_copy(slang_operation *, const slang_operation *);

extern slang_operation *
slang_operation_new(GLuint count);


#ifdef __cplusplus
}
#endif

#endif /* SLANG_COMPILE_OPERATION_H */
