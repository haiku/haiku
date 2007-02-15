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

#if !defined SLANG_ASSEMBLE_TYPEINFO_H
#define SLANG_ASSEMBLE_TYPEINFO_H

#if defined __cplusplus
extern "C" {
#endif


/**
 * The basic shading language types (float, vec4, mat3, etc)
 */
typedef enum slang_type_specifier_type_
{
   slang_spec_void,
   slang_spec_bool,
   slang_spec_bvec2,
   slang_spec_bvec3,
   slang_spec_bvec4,
   slang_spec_int,
   slang_spec_ivec2,
   slang_spec_ivec3,
   slang_spec_ivec4,
   slang_spec_float,
   slang_spec_vec2,
   slang_spec_vec3,
   slang_spec_vec4,
   slang_spec_mat2,
   slang_spec_mat3,
   slang_spec_mat4,
   slang_spec_sampler1D,
   slang_spec_sampler2D,
   slang_spec_sampler3D,
   slang_spec_samplerCube,
   slang_spec_sampler1DShadow,
   slang_spec_sampler2DShadow,
   slang_spec_struct,
   slang_spec_array
} slang_type_specifier_type;


/**
 * Describes more sophisticated types, like structs and arrays.
 */
typedef struct slang_type_specifier_
{
   slang_type_specifier_type type;
   struct slang_struct_ *_struct;         /**< used if type == spec_struct */
   struct slang_type_specifier_ *_array;  /**< used if type == spec_array */
} slang_type_specifier;


extern GLvoid
slang_type_specifier_ctr(slang_type_specifier *);

extern GLvoid
slang_type_specifier_dtr(slang_type_specifier *);

extern GLboolean
slang_type_specifier_copy(slang_type_specifier *, const slang_type_specifier *);

extern GLboolean
slang_type_specifier_equal(const slang_type_specifier *,
                           const slang_type_specifier *);


typedef struct slang_assembly_typeinfo_
{
   GLboolean can_be_referenced;
   GLboolean is_swizzled;
   slang_swizzle swz;
   slang_type_specifier spec;
   GLuint array_len;
} slang_assembly_typeinfo;

extern GLboolean
slang_assembly_typeinfo_construct(slang_assembly_typeinfo *);

extern GLvoid
slang_assembly_typeinfo_destruct(slang_assembly_typeinfo *);


/**
 * Retrieves type information about an operation.
 * Returns GL_TRUE on success.
 * Returns GL_FALSE otherwise.
 */
extern GLboolean
_slang_typeof_operation(const slang_assemble_ctx *,
                        const struct slang_operation_ *,
                        slang_assembly_typeinfo *);

extern GLboolean
_slang_typeof_operation_(const struct slang_operation_ *,
                         const slang_assembly_name_space *,
                         slang_assembly_typeinfo *, slang_atom_pool *);

/**
 * Retrieves type of a function prototype, if one exists.
 * Returns GL_TRUE on success, even if the function was not found.
 * Returns GL_FALSE otherwise.
 */
extern GLboolean
_slang_typeof_function(slang_atom a_name,
                       const struct slang_operation_ *params,
                       GLuint num_params, const slang_assembly_name_space *,
                       slang_type_specifier *spec, GLboolean *exists,
                       slang_atom_pool *);

extern GLboolean
_slang_type_is_matrix(slang_type_specifier_type);

extern GLboolean
_slang_type_is_vector(slang_type_specifier_type);

extern slang_type_specifier_type
_slang_type_base(slang_type_specifier_type);

extern GLuint
_slang_type_dim(slang_type_specifier_type);



#ifdef __cplusplus
}
#endif

#endif

