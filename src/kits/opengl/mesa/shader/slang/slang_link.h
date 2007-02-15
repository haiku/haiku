/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
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

#if !defined SLANG_LINK_H
#define SLANG_LINK_H

#include "slang_compile.h"

#if defined __cplusplus
extern "C" {
#endif

enum
{
   SLANG_SHADER_VERTEX,
   SLANG_SHADER_FRAGMENT,
   SLANG_SHADER_MAX
};


/**
 * Active variables.
 *
 * Active uniforms/attribs can be queried by the application to get a
 * list of uniforms/attribs actually used by shaders (uniforms) or
 * vertex shader only (attribs).
 */
/*@{*/
typedef struct
{
   slang_export_data_quant *quant;
   GLchar *name;
} slang_active_variable;

typedef struct
{
   slang_active_variable *table;
   GLuint count;
} slang_active_variables;
/*@}*/


/**
 * Attrib binding override.
 *
 * The application can override GL attrib binding by specifying its
 * preferred index assignment for a given attrib name. Those overrides
 * are taken into account while linking the program.
 */
/*@{*/
typedef struct
{
   GLuint index;
   GLchar *name;
} slang_attrib_override;

typedef struct
{
   slang_attrib_override *table;
   GLuint count;
} slang_attrib_overrides;
/*@}*/


extern GLboolean
_slang_attrib_overrides_add (slang_attrib_overrides *, GLuint, const GLchar *);


/**
 * Uniform bindings.
 *
 * Each slang_uniform_binding holds an array of addresses to actual
 * memory locations in those shader types that use that
 * uniform. Uniform bindings are held in an array and accessed by
 * array index which is seen to the application as a uniform location.
 *
 * When the application writes to a particular uniform, it specifies
 * its location.  This location is treated as an array index to
 * slang_uniform_bindings::table and tested against
 * slang_uniform_bindings::count limit. The result is a pointer to
 * slang_uniform_binding.  The type of data being written to uniform
 * is tested against slang_uniform_binding::quant.  If the types are
 * compatible, the array slang_uniform_binding::address is iterated
 * for each shader type and if the address is valid (i.e. the uniform
 * is used by this shader type), the new uniform value is written at
 * that address.
 */
/*@{*/
typedef struct
{
   slang_export_data_quant *quant;
   GLchar *name;
   GLuint address[SLANG_SHADER_MAX];
} slang_uniform_binding;

typedef struct
{
   slang_uniform_binding *table;
   GLuint count;
} slang_uniform_bindings;
/*@}*/


/**
 * Attrib bindings.
 *
 * There is a fixed number of vertex attrib vectors (attrib
 * slots). The slang_attrib_slot::addr maps vertex attrib index to the
 * actual memory location of the attrib in vertex shader.  One vertex
 * attrib can span over many attrib slots (this is the case for
 * matrices). The slang_attrib_binding::first_slot_index holds the
 * first slot index that the attrib is bound to.
 */
/*@{*/
typedef struct
{
   slang_export_data_quant *quant;
   GLchar *name;
   GLuint first_slot_index;
} slang_attrib_binding;

typedef struct
{
   GLuint addr;   /**< memory location */
   GLuint fill;   /**< 1..4, number of components used */
} slang_attrib_slot;

typedef struct
{
   slang_attrib_binding bindings[MAX_VERTEX_ATTRIBS];
   GLuint binding_count;
   slang_attrib_slot slots[MAX_VERTEX_ATTRIBS];
} slang_attrib_bindings;
/*@}*/



/**
 * Varying bindings.
 *
 * There is a fixed number of varying floats (varying slots). The
 * slang_varying_slot::vert_addr maps varying float index to the
 * actual memory location of the output variable in vertex shader.
 * The slang_varying_slot::frag_addr maps varying float index to the
 * actual memory location of the input variable in fragment shader.
 */
/*@{*/
typedef struct
{
   GLuint vert_addr;
   GLuint frag_addr;
} slang_varying_slot;

typedef struct
{
   slang_export_data_quant *quant;
   GLchar *name;
   GLuint first_slot_index;
} slang_varying_binding;

typedef struct
{
   slang_varying_binding bindings[MAX_VARYING_FLOATS];
   GLuint binding_count;
   slang_varying_slot slots[MAX_VARYING_FLOATS];
   GLuint slot_count;
} slang_varying_bindings;
/*@}*/


/**
 * Texture usage.
 *
 * A slang_texture_usage struct holds indirect information about
 * texture image unit usage. The slang_texture_usages::table is
 * derived from active uniform table by extracting only uniforms that
 * are samplers.
 *
 * To collect current texture usage one must iterate the
 * slang_texture_usages::table and read uniform at address
 * slang_texture_usage::frag_address to get texture unit index. This
 * index, coupled with texture access type (target) taken from
 * slang_texture_usage::quant forms texture usage for that texture
 * unit.
 */
/*@{*/
typedef struct
{
   slang_export_data_quant *quant;
   GLuint frag_address;
} slang_texture_usage;

typedef struct
{
   slang_texture_usage *table;
   GLuint count;
} slang_texture_usages;
/*@}*/


extern GLvoid
_slang_texture_usages_ctr (slang_texture_usages *);

extern GLvoid
_slang_texture_usages_dtr (slang_texture_usages *);

enum
{
   SLANG_COMMON_FIXED_MODELVIEWMATRIX,
   SLANG_COMMON_FIXED_PROJECTIONMATRIX,
   SLANG_COMMON_FIXED_MODELVIEWPROJECTIONMATRIX,
   SLANG_COMMON_FIXED_TEXTUREMATRIX,
   SLANG_COMMON_FIXED_NORMALMATRIX,
   SLANG_COMMON_FIXED_MODELVIEWMATRIXINVERSE,
   SLANG_COMMON_FIXED_PROJECTIONMATRIXINVERSE,
   SLANG_COMMON_FIXED_MODELVIEWPROJECTIONMATRIXINVERSE,
   SLANG_COMMON_FIXED_TEXTUREMATRIXINVERSE,
   SLANG_COMMON_FIXED_MODELVIEWMATRIXTRANSPOSE,
   SLANG_COMMON_FIXED_PROJECTIONMATRIXTRANSPOSE,
   SLANG_COMMON_FIXED_MODELVIEWPROJECTIONMATRIXTRANSPOSE,
   SLANG_COMMON_FIXED_TEXTUREMATRIXTRANSPOSE,
   SLANG_COMMON_FIXED_MODELVIEWMATRIXINVERSETRANSPOSE,
   SLANG_COMMON_FIXED_PROJECTIONMATRIXINVERSETRANSPOSE,
   SLANG_COMMON_FIXED_MODELVIEWPROJECTIONMATRIXINVERSETRANSPOSE,
   SLANG_COMMON_FIXED_TEXTUREMATRIXINVERSETRANSPOSE,
   SLANG_COMMON_FIXED_NORMALSCALE,
   SLANG_COMMON_FIXED_DEPTHRANGE,
   SLANG_COMMON_FIXED_CLIPPLANE,
   SLANG_COMMON_FIXED_POINT,
   SLANG_COMMON_FIXED_FRONTMATERIAL,
   SLANG_COMMON_FIXED_BACKMATERIAL,
   SLANG_COMMON_FIXED_LIGHTSOURCE,
   SLANG_COMMON_FIXED_LIGHTMODEL,
   SLANG_COMMON_FIXED_FRONTLIGHTMODELPRODUCT,
   SLANG_COMMON_FIXED_BACKLIGHTMODELPRODUCT,
   SLANG_COMMON_FIXED_FRONTLIGHTPRODUCT,
   SLANG_COMMON_FIXED_BACKLIGHTPRODUCT,
   SLANG_COMMON_FIXED_TEXTUREENVCOLOR,
   SLANG_COMMON_FIXED_EYEPLANES,
   SLANG_COMMON_FIXED_EYEPLANET,
   SLANG_COMMON_FIXED_EYEPLANER,
   SLANG_COMMON_FIXED_EYEPLANEQ,
   SLANG_COMMON_FIXED_OBJECTPLANES,
   SLANG_COMMON_FIXED_OBJECTPLANET,
   SLANG_COMMON_FIXED_OBJECTPLANER,
   SLANG_COMMON_FIXED_OBJECTPLANEQ,
   SLANG_COMMON_FIXED_FOG,
   SLANG_COMMON_FIXED_MAX
};

enum
{
   SLANG_VERTEX_FIXED_POSITION,
   SLANG_VERTEX_FIXED_POINTSIZE,
   SLANG_VERTEX_FIXED_CLIPVERTEX,
   SLANG_VERTEX_FIXED_COLOR,
   SLANG_VERTEX_FIXED_SECONDARYCOLOR,
   SLANG_VERTEX_FIXED_NORMAL,
   SLANG_VERTEX_FIXED_VERTEX,
   SLANG_VERTEX_FIXED_MULTITEXCOORD0,
   SLANG_VERTEX_FIXED_MULTITEXCOORD1,
   SLANG_VERTEX_FIXED_MULTITEXCOORD2,
   SLANG_VERTEX_FIXED_MULTITEXCOORD3,
   SLANG_VERTEX_FIXED_MULTITEXCOORD4,
   SLANG_VERTEX_FIXED_MULTITEXCOORD5,
   SLANG_VERTEX_FIXED_MULTITEXCOORD6,
   SLANG_VERTEX_FIXED_MULTITEXCOORD7,
   SLANG_VERTEX_FIXED_FOGCOORD,
   SLANG_VERTEX_FIXED_FRONTCOLOR,
   SLANG_VERTEX_FIXED_BACKCOLOR,
   SLANG_VERTEX_FIXED_FRONTSECONDARYCOLOR,
   SLANG_VERTEX_FIXED_BACKSECONDARYCOLOR,
   SLANG_VERTEX_FIXED_TEXCOORD,
   SLANG_VERTEX_FIXED_FOGFRAGCOORD,
   SLANG_VERTEX_FIXED_MAX
};

enum
{
   SLANG_FRAGMENT_FIXED_FRAGCOORD,
   SLANG_FRAGMENT_FIXED_FRONTFACING,
   SLANG_FRAGMENT_FIXED_FRAGCOLOR,
   SLANG_FRAGMENT_FIXED_FRAGDATA,
   SLANG_FRAGMENT_FIXED_FRAGDEPTH,
   SLANG_FRAGMENT_FIXED_COLOR,
   SLANG_FRAGMENT_FIXED_SECONDARYCOLOR,
   SLANG_FRAGMENT_FIXED_TEXCOORD,
   SLANG_FRAGMENT_FIXED_FOGFRAGCOORD,
   SLANG_FRAGMENT_FIXED_MAX
};

enum
{
   SLANG_COMMON_CODE_MAIN,
   SLANG_COMMON_CODE_MAX
};

typedef struct
{
   slang_active_variables active_uniforms;
   slang_active_variables active_attribs;
   slang_attrib_overrides attrib_overrides;
   slang_uniform_bindings uniforms;
   slang_attrib_bindings attribs;
   slang_varying_bindings varyings;
   slang_texture_usages texture_usage;
   GLuint common_fixed_entries[SLANG_SHADER_MAX][SLANG_COMMON_FIXED_MAX];
   GLuint vertex_fixed_entries[SLANG_VERTEX_FIXED_MAX];
   GLuint fragment_fixed_entries[SLANG_FRAGMENT_FIXED_MAX];
   GLuint code[SLANG_SHADER_MAX][SLANG_COMMON_CODE_MAX];
   slang_machine *machines[SLANG_SHADER_MAX];
   slang_assembly_file *assemblies[SLANG_SHADER_MAX];
} slang_program;

extern GLvoid
_slang_program_ctr (slang_program *);

extern GLvoid
_slang_program_dtr (slang_program *);

extern GLvoid
_slang_program_rst (slang_program *);

extern GLboolean
_slang_link (slang_program *, slang_code_object **, GLuint);

#ifdef __cplusplus
}
#endif

#endif

