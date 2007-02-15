/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * \file program.c
 * Vertex and fragment program support functions.
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "program.h"
#include "nvfragparse.h"
#include "program_instruction.h"
#include "nvvertparse.h"
#include "atifragshader.h"


static const char *
make_state_string(const GLint stateTokens[6]);

static GLbitfield
make_state_flags(const GLint state[]);


/**********************************************************************/
/* Utility functions                                                  */
/**********************************************************************/


/* A pointer to this dummy program is put into the hash table when
 * glGenPrograms is called.
 */
struct gl_program _mesa_DummyProgram;


/**
 * Init context's vertex/fragment program state
 */
void
_mesa_init_program(GLcontext *ctx)
{
   GLuint i;

   ctx->Program.ErrorPos = -1;
   ctx->Program.ErrorString = _mesa_strdup("");

#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program
   ctx->VertexProgram.Enabled = GL_FALSE;
   ctx->VertexProgram.PointSizeEnabled = GL_FALSE;
   ctx->VertexProgram.TwoSideEnabled = GL_FALSE;
   ctx->VertexProgram.Current = (struct gl_vertex_program *) ctx->Shared->DefaultVertexProgram;
   assert(ctx->VertexProgram.Current);
   ctx->VertexProgram.Current->Base.RefCount++;
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS / 4; i++) {
      ctx->VertexProgram.TrackMatrix[i] = GL_NONE;
      ctx->VertexProgram.TrackMatrixTransform[i] = GL_IDENTITY_NV;
   }
#endif

#if FEATURE_NV_fragment_program || FEATURE_ARB_fragment_program
   ctx->FragmentProgram.Enabled = GL_FALSE;
   ctx->FragmentProgram.Current = (struct gl_fragment_program *) ctx->Shared->DefaultFragmentProgram;
   assert(ctx->FragmentProgram.Current);
   ctx->FragmentProgram.Current->Base.RefCount++;
#endif

   /* XXX probably move this stuff */
#if FEATURE_ATI_fragment_shader
   ctx->ATIFragmentShader.Enabled = GL_FALSE;
   ctx->ATIFragmentShader.Current = (struct ati_fragment_shader *) ctx->Shared->DefaultFragmentShader;
   assert(ctx->ATIFragmentShader.Current);
   ctx->ATIFragmentShader.Current->RefCount++;
#endif
}


/**
 * Free a context's vertex/fragment program state
 */
void
_mesa_free_program_data(GLcontext *ctx)
{
#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program
   if (ctx->VertexProgram.Current) {
      ctx->VertexProgram.Current->Base.RefCount--;
      if (ctx->VertexProgram.Current->Base.RefCount <= 0)
         ctx->Driver.DeleteProgram(ctx, &(ctx->VertexProgram.Current->Base));
   }
#endif
#if FEATURE_NV_fragment_program || FEATURE_ARB_fragment_program
   if (ctx->FragmentProgram.Current) {
      ctx->FragmentProgram.Current->Base.RefCount--;
      if (ctx->FragmentProgram.Current->Base.RefCount <= 0)
         ctx->Driver.DeleteProgram(ctx, &(ctx->FragmentProgram.Current->Base));
   }
#endif
   /* XXX probably move this stuff */
#if FEATURE_ATI_fragment_shader
   if (ctx->ATIFragmentShader.Current) {
      ctx->ATIFragmentShader.Current->RefCount--;
      if (ctx->ATIFragmentShader.Current->RefCount <= 0) {
         _mesa_free(ctx->ATIFragmentShader.Current);
      }
   }
#endif
   _mesa_free((void *) ctx->Program.ErrorString);
}




/**
 * Set the vertex/fragment program error state (position and error string).
 * This is generally called from within the parsers.
 */
void
_mesa_set_program_error(GLcontext *ctx, GLint pos, const char *string)
{
   ctx->Program.ErrorPos = pos;
   _mesa_free((void *) ctx->Program.ErrorString);
   if (!string)
      string = "";
   ctx->Program.ErrorString = _mesa_strdup(string);
}


/**
 * Find the line number and column for 'pos' within 'string'.
 * Return a copy of the line which contains 'pos'.  Free the line with
 * _mesa_free().
 * \param string  the program string
 * \param pos     the position within the string
 * \param line    returns the line number corresponding to 'pos'.
 * \param col     returns the column number corresponding to 'pos'.
 * \return copy of the line containing 'pos'.
 */
const GLubyte *
_mesa_find_line_column(const GLubyte *string, const GLubyte *pos,
                       GLint *line, GLint *col)
{
   const GLubyte *lineStart = string;
   const GLubyte *p = string;
   GLubyte *s;
   int len;

   *line = 1;

   while (p != pos) {
      if (*p == (GLubyte) '\n') {
         (*line)++;
         lineStart = p + 1;
      }
      p++;
   }

   *col = (pos - lineStart) + 1;

   /* return copy of this line */
   while (*p != 0 && *p != '\n')
      p++;
   len = p - lineStart;
   s = (GLubyte *) _mesa_malloc(len + 1);
   _mesa_memcpy(s, lineStart, len);
   s[len] = 0;

   return s;
}


/**
 * Initialize a new vertex/fragment program object.
 */
static struct gl_program *
_mesa_init_program_struct( GLcontext *ctx, struct gl_program *prog,
                           GLenum target, GLuint id)
{
   (void) ctx;
   if (prog) {
      prog->Id = id;
      prog->Target = target;
      prog->Resident = GL_TRUE;
      prog->RefCount = 1;
      prog->Format = GL_PROGRAM_FORMAT_ASCII_ARB;
   }

   return prog;
}


/**
 * Initialize a new fragment program object.
 */
struct gl_program *
_mesa_init_fragment_program( GLcontext *ctx, struct gl_fragment_program *prog,
                             GLenum target, GLuint id)
{
   if (prog) 
      return _mesa_init_program_struct( ctx, &prog->Base, target, id );
   else
      return NULL;
}


/**
 * Initialize a new vertex program object.
 */
struct gl_program *
_mesa_init_vertex_program( GLcontext *ctx, struct gl_vertex_program *prog,
                           GLenum target, GLuint id)
{
   if (prog) 
      return _mesa_init_program_struct( ctx, &prog->Base, target, id );
   else
      return NULL;
}


/**
 * Allocate and initialize a new fragment/vertex program object but
 * don't put it into the program hash table.  Called via
 * ctx->Driver.NewProgram.  May be overridden (ie. replaced) by a
 * device driver function to implement OO deriviation with additional
 * types not understood by this function.
 * 
 * \param ctx  context
 * \param id   program id/number
 * \param target  program target/type
 * \return  pointer to new program object
 */
struct gl_program *
_mesa_new_program(GLcontext *ctx, GLenum target, GLuint id)
{
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: /* == GL_VERTEX_PROGRAM_NV */
      return _mesa_init_vertex_program(ctx, CALLOC_STRUCT(gl_vertex_program),
                                       target, id );
   case GL_FRAGMENT_PROGRAM_NV:
   case GL_FRAGMENT_PROGRAM_ARB:
      return _mesa_init_fragment_program(ctx,
                                         CALLOC_STRUCT(gl_fragment_program),
                                         target, id );
   default:
      _mesa_problem(ctx, "bad target in _mesa_new_program");
      return NULL;
   }
}


/**
 * Delete a program and remove it from the hash table, ignoring the
 * reference count.
 * Called via ctx->Driver.DeleteProgram.  May be wrapped (OO deriviation)
 * by a device driver function.
 */
void
_mesa_delete_program(GLcontext *ctx, struct gl_program *prog)
{
   (void) ctx;
   ASSERT(prog);

   if (prog == &_mesa_DummyProgram)
      return;
                 
   if (prog->String)
      _mesa_free(prog->String);

   if (prog->Instructions) {
      GLuint i;
      for (i = 0; i < prog->NumInstructions; i++) {
         if (prog->Instructions[i].Data)
            _mesa_free(prog->Instructions[i].Data);
      }
      _mesa_free(prog->Instructions);
   }

   if (prog->Parameters) {
      _mesa_free_parameter_list(prog->Parameters);
   }

   /* XXX this is a little ugly */
   if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
      struct gl_vertex_program *vprog = (struct gl_vertex_program *) prog;
      if (vprog->TnlData)
         _mesa_free(vprog->TnlData);
   }

   _mesa_free(prog);
}


/**
 * Return the gl_program object for a given ID.
 * Basically just a wrapper for _mesa_HashLookup() to avoid a lot of
 * casts elsewhere.
 */
struct gl_program *
_mesa_lookup_program(GLcontext *ctx, GLuint id)
{
   if (id)
      return (struct gl_program *) _mesa_HashLookup(ctx->Shared->Programs, id);
   else
      return NULL;
}


/**********************************************************************/
/* Program parameter functions                                        */
/**********************************************************************/

struct gl_program_parameter_list *
_mesa_new_parameter_list(void)
{
   return (struct gl_program_parameter_list *)
      _mesa_calloc(sizeof(struct gl_program_parameter_list));
}


/**
 * Free a parameter list and all its parameters
 */
void
_mesa_free_parameter_list(struct gl_program_parameter_list *paramList)
{
   GLuint i;
   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Name)
	 _mesa_free((void *) paramList->Parameters[i].Name);
   }
   _mesa_free(paramList->Parameters);
   if (paramList->ParameterValues)
      _mesa_align_free(paramList->ParameterValues);
   _mesa_free(paramList);
}


/**
 * Add a new parameter to a parameter list.
 * \param paramList  the list to add the parameter to
 * \param name  the parameter name, will be duplicated/copied!
 * \param values  initial parameter value, 4 GLfloats
 * \param type  type of parameter, such as 
 * \return  index of new parameter in the list, or -1 if error (out of mem)
 */
static GLint
add_parameter(struct gl_program_parameter_list *paramList,
              const char *name, const GLfloat values[4],
              enum register_file type)
{
   const GLuint n = paramList->NumParameters;

   if (n == paramList->Size) {
      /* Need to grow the parameter list array */
      if (paramList->Size == 0)
	 paramList->Size = 8;
      else
         paramList->Size *= 2;

      /* realloc arrays */
      paramList->Parameters = (struct gl_program_parameter *)
	 _mesa_realloc(paramList->Parameters,
		       n * sizeof(struct gl_program_parameter),
		       paramList->Size * sizeof(struct gl_program_parameter));

      paramList->ParameterValues = (GLfloat (*)[4])
         _mesa_align_realloc(paramList->ParameterValues,         /* old buf */
                             n * 4 * sizeof(GLfloat),            /* old size */
                             paramList->Size * 4 *sizeof(GLfloat), /* new sz */
                             16);
   }

   if (!paramList->Parameters ||
       !paramList->ParameterValues) {
      /* out of memory */
      paramList->NumParameters = 0;
      paramList->Size = 0;
      return -1;
   }
   else {
      paramList->NumParameters = n + 1;

      _mesa_memset(&paramList->Parameters[n], 0, 
		   sizeof(struct gl_program_parameter));

      paramList->Parameters[n].Name = name ? _mesa_strdup(name) : NULL;
      paramList->Parameters[n].Type = type;
      if (values)
         COPY_4V(paramList->ParameterValues[n], values);
      return (GLint) n;
   }
}


/**
 * Add a new named program parameter (Ex: NV_fragment_program DEFINE statement)
 * \return index of the new entry in the parameter list
 */
GLint
_mesa_add_named_parameter(struct gl_program_parameter_list *paramList,
                          const char *name, const GLfloat values[4])
{
   return add_parameter(paramList, name, values, PROGRAM_NAMED_PARAM);
}


/**
 * Add a new named constant to the parameter list.
 * This will be used when the program contains something like this:
 *    PARAM myVals = { 0, 1, 2, 3 };
 *
 * \param paramList  the parameter list
 * \param name  the name for the constant
 * \param values  four float values
 * \return index/position of the new parameter in the parameter list
 */
GLint
_mesa_add_named_constant(struct gl_program_parameter_list *paramList,
                         const char *name, const GLfloat values[4],
                         GLuint size)
{
#if 0 /* disable this for now -- we need to save the name! */
   GLuint pos, swizzle;
   ASSERT(size == 4); /* XXX future feature */
   /* check if we already have this constant */
   if (_mesa_lookup_parameter_constant(paramList, values, 4, &pos, &swizzle)) {
      return pos;
   }
#endif
   return add_parameter(paramList, name, values, PROGRAM_CONSTANT);
}


/**
 * Add a new unnamed constant to the parameter list.
 * This will be used when the program contains something like this:
 *    MOV r, { 0, 1, 2, 3 };
 *
 * \param paramList  the parameter list
 * \param values  four float values
 * \return index/position of the new parameter in the parameter list.
 */
GLint
_mesa_add_unnamed_constant(struct gl_program_parameter_list *paramList,
                           const GLfloat values[4], GLuint size)
{
   GLuint pos, swizzle;
   ASSERT(size == 4); /* XXX future feature */
   /* check if we already have this constant */
   if (_mesa_lookup_parameter_constant(paramList, values, 4, &pos, &swizzle)) {
      return pos;
   }
   return add_parameter(paramList, NULL, values, PROGRAM_CONSTANT);
}


/**
 * Add a new state reference to the parameter list.
 * This will be used when the program contains something like this:
 *    PARAM ambient = state.material.front.ambient;
 *
 * \param paramList  the parameter list
 * \param state  an array of 6 state tokens
 * \return index of the new parameter.
 */
GLint
_mesa_add_state_reference(struct gl_program_parameter_list *paramList,
                          const GLint *stateTokens)
{
   /* XXX we should probably search the current parameter list to see if
    * the new state reference is already present.
    */
   GLint index;
   const char *name = make_state_string(stateTokens);

   index = add_parameter(paramList, name, NULL, PROGRAM_STATE_VAR);
   if (index >= 0) {
      GLuint i;
      for (i = 0; i < 6; i++) {
         paramList->Parameters[index].StateIndexes[i]
            = (enum state_index) stateTokens[i];
      }
      paramList->StateFlags |= make_state_flags(stateTokens);
   }

   /* free name string here since we duplicated it in add_parameter() */
   _mesa_free((void *) name);

   return index;
}


/**
 * Lookup a parameter value by name in the given parameter list.
 * \return pointer to the float[4] values.
 */
GLfloat *
_mesa_lookup_parameter_value(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name)
{
   GLuint i;

   if (!paramList)
      return NULL;

   if (nameLen == -1) {
      /* name is null-terminated */
      for (i = 0; i < paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strcmp(paramList->Parameters[i].Name, name) == 0)
            return paramList->ParameterValues[i];
      }
   }
   else {
      /* name is not null-terminated, use nameLen */
      for (i = 0; i < paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strncmp(paramList->Parameters[i].Name, name, nameLen) == 0
             && _mesa_strlen(paramList->Parameters[i].Name) == (size_t)nameLen)
            return paramList->ParameterValues[i];
      }
   }
   return NULL;
}


/**
 * Given a program parameter name, find its position in the list of parameters.
 * \param paramList  the parameter list to search
 * \param nameLen  length of name (in chars).
 *                 If length is negative, assume that name is null-terminated.
 * \param name  the name to search for
 * \return index of parameter in the list.
 */
GLint
_mesa_lookup_parameter_index(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name)
{
   GLint i;

   if (!paramList)
      return -1;

   if (nameLen == -1) {
      /* name is null-terminated */
      for (i = 0; i < (GLint) paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strcmp(paramList->Parameters[i].Name, name) == 0)
            return i;
      }
   }
   else {
      /* name is not null-terminated, use nameLen */
      for (i = 0; i < (GLint) paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strncmp(paramList->Parameters[i].Name, name, nameLen) == 0
             && _mesa_strlen(paramList->Parameters[i].Name) == (size_t)nameLen)
            return i;
      }
   }
   return -1;
}


/**
 * Look for a float vector in the given parameter list.  The float vector
 * may be of length 1, 2, 3 or 4.
 * \param paramList  the parameter list to search
 * \param v  the float vector to search for
 * \param size  number of element in v
 * \param posOut  returns the position of the constant, if found
 * \param swizzleOut  returns a swizzle mask describing location of the
 *                    vector elements if found
 * \return GL_TRUE if found, GL_FALSE if not found
 */
GLboolean
_mesa_lookup_parameter_constant(const struct gl_program_parameter_list *paramList,
                                const GLfloat v[], GLsizei vSize,
                                GLuint *posOut, GLuint *swizzleOut)
{
   GLuint i;

   assert(vSize >= 1);
   assert(vSize <= 4);

   if (!paramList)
      return -1;

   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Type == PROGRAM_CONSTANT) {
         const GLint maxShift = 4 - vSize;
         GLint shift, j;
         for (shift = 0; shift <= maxShift; shift++) {
            GLint matched = 0;
            GLuint swizzle[4];
            swizzle[0] = swizzle[1] = swizzle[2] = swizzle[3] = 0;
            /* XXX we could do out-of-order swizzle matches too, someday */
            for (j = 0; j < vSize; j++) {
               assert(shift + j < 4);
               if (paramList->ParameterValues[i][shift + j] == v[j]) {
                  matched++;
                  swizzle[j] = shift + j;
               }
            }
            if (matched == vSize) {
               /* found! */
               *posOut = i;
               *swizzleOut = MAKE_SWIZZLE4(swizzle[0], swizzle[1],
                                           swizzle[2], swizzle[3]);
               return GL_TRUE;
            }
         }
      }
   }

   return GL_FALSE;
}


/**
 * Use the list of tokens in the state[] array to find global GL state
 * and return it in <value>.  Usually, four values are returned in <value>
 * but matrix queries may return as many as 16 values.
 * This function is used for ARB vertex/fragment programs.
 * The program parser will produce the state[] values.
 */
static void
_mesa_fetch_state(GLcontext *ctx, const enum state_index state[],
                  GLfloat *value)
{
   switch (state[0]) {
   case STATE_MATERIAL:
      {
         /* state[1] is either 0=front or 1=back side */
         const GLuint face = (GLuint) state[1];
         const struct gl_material *mat = &ctx->Light.Material;
         ASSERT(face == 0 || face == 1);
         /* we rely on tokens numbered so that _BACK_ == _FRONT_+ 1 */
         ASSERT(MAT_ATTRIB_FRONT_AMBIENT + 1 == MAT_ATTRIB_BACK_AMBIENT);
         /* XXX we could get rid of this switch entirely with a little
          * work in arbprogparse.c's parse_state_single_item().
          */
         /* state[2] is the material attribute */
         switch (state[2]) {
         case STATE_AMBIENT:
            COPY_4V(value, mat->Attrib[MAT_ATTRIB_FRONT_AMBIENT + face]);
            return;
         case STATE_DIFFUSE:
            COPY_4V(value, mat->Attrib[MAT_ATTRIB_FRONT_DIFFUSE + face]);
            return;
         case STATE_SPECULAR:
            COPY_4V(value, mat->Attrib[MAT_ATTRIB_FRONT_SPECULAR + face]);
            return;
         case STATE_EMISSION:
            COPY_4V(value, mat->Attrib[MAT_ATTRIB_FRONT_EMISSION + face]);
            return;
         case STATE_SHININESS:
            value[0] = mat->Attrib[MAT_ATTRIB_FRONT_SHININESS + face][0];
            value[1] = 0.0F;
            value[2] = 0.0F;
            value[3] = 1.0F;
            return;
         default:
            _mesa_problem(ctx, "Invalid material state in fetch_state");
            return;
         }
      }
   case STATE_LIGHT:
      {
         /* state[1] is the light number */
         const GLuint ln = (GLuint) state[1];
         /* state[2] is the light attribute */
         switch (state[2]) {
         case STATE_AMBIENT:
            COPY_4V(value, ctx->Light.Light[ln].Ambient);
            return;
         case STATE_DIFFUSE:
            COPY_4V(value, ctx->Light.Light[ln].Diffuse);
            return;
         case STATE_SPECULAR:
            COPY_4V(value, ctx->Light.Light[ln].Specular);
            return;
         case STATE_POSITION:
            COPY_4V(value, ctx->Light.Light[ln].EyePosition);
            return;
         case STATE_ATTENUATION:
            value[0] = ctx->Light.Light[ln].ConstantAttenuation;
            value[1] = ctx->Light.Light[ln].LinearAttenuation;
            value[2] = ctx->Light.Light[ln].QuadraticAttenuation;
            value[3] = ctx->Light.Light[ln].SpotExponent;
            return;
         case STATE_SPOT_DIRECTION:
            COPY_3V(value, ctx->Light.Light[ln].EyeDirection);
            value[3] = ctx->Light.Light[ln]._CosCutoff;
            return;
         case STATE_HALF:
            {
               GLfloat eye_z[] = {0, 0, 1};
					
               /* Compute infinite half angle vector:
                *   half-vector = light_position + (0, 0, 1) 
                * and then normalize.  w = 0
		*
		* light.EyePosition.w should be 0 for infinite lights.
                */
	       ADD_3V(value, eye_z, ctx->Light.Light[ln].EyePosition);
	       NORMALIZE_3FV(value);
	       value[3] = 0;
            }						  
            return;
	 case STATE_POSITION_NORMALIZED:
            COPY_4V(value, ctx->Light.Light[ln].EyePosition);
	    NORMALIZE_3FV( value );
            return;
         default:
            _mesa_problem(ctx, "Invalid light state in fetch_state");
            return;
         }
      }
   case STATE_LIGHTMODEL_AMBIENT:
      COPY_4V(value, ctx->Light.Model.Ambient);
      return;
   case STATE_LIGHTMODEL_SCENECOLOR:
      if (state[1] == 0) {
         /* front */
         GLint i;
         for (i = 0; i < 3; i++) {
            value[i] = ctx->Light.Model.Ambient[i]
               * ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT][i]
               + ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_EMISSION][i];
         }
	 value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE][3];
      }
      else {
         /* back */
         GLint i;
         for (i = 0; i < 3; i++) {
            value[i] = ctx->Light.Model.Ambient[i]
               * ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_AMBIENT][i]
               + ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_EMISSION][i];
         }
	 value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_DIFFUSE][3];
      }
      return;
   case STATE_LIGHTPROD:
      {
         const GLuint ln = (GLuint) state[1];
         const GLuint face = (GLuint) state[2];
         GLint i;
         ASSERT(face == 0 || face == 1);
         switch (state[3]) {
            case STATE_AMBIENT:
               for (i = 0; i < 3; i++) {
                  value[i] = ctx->Light.Light[ln].Ambient[i] *
                     ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT+face][i];
               }
               /* [3] = material alpha */
               value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][3];
               return;
            case STATE_DIFFUSE:
               for (i = 0; i < 3; i++) {
                  value[i] = ctx->Light.Light[ln].Diffuse[i] *
                     ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][i];
               }
               /* [3] = material alpha */
               value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][3];
               return;
            case STATE_SPECULAR:
               for (i = 0; i < 3; i++) {
                  value[i] = ctx->Light.Light[ln].Specular[i] *
                     ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SPECULAR+face][i];
               }
               /* [3] = material alpha */
               value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][3];
               return;
            default:
               _mesa_problem(ctx, "Invalid lightprod state in fetch_state");
               return;
         }
      }
   case STATE_TEXGEN:
      {
         /* state[1] is the texture unit */
         const GLuint unit = (GLuint) state[1];
         /* state[2] is the texgen attribute */
         switch (state[2]) {
         case STATE_TEXGEN_EYE_S:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneS);
            return;
         case STATE_TEXGEN_EYE_T:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneT);
            return;
         case STATE_TEXGEN_EYE_R:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneR);
            return;
         case STATE_TEXGEN_EYE_Q:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneQ);
            return;
         case STATE_TEXGEN_OBJECT_S:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneS);
            return;
         case STATE_TEXGEN_OBJECT_T:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneT);
            return;
         case STATE_TEXGEN_OBJECT_R:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneR);
            return;
         case STATE_TEXGEN_OBJECT_Q:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneQ);
            return;
         default:
            _mesa_problem(ctx, "Invalid texgen state in fetch_state");
            return;
         }
      }
   case STATE_TEXENV_COLOR:
      {		
         /* state[1] is the texture unit */
         const GLuint unit = (GLuint) state[1];
         COPY_4V(value, ctx->Texture.Unit[unit].EnvColor);
      }			
      return;
   case STATE_FOG_COLOR:
      COPY_4V(value, ctx->Fog.Color);
      return;
   case STATE_FOG_PARAMS:
      value[0] = ctx->Fog.Density;
      value[1] = ctx->Fog.Start;
      value[2] = ctx->Fog.End;
      value[3] = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
      return;
   case STATE_CLIPPLANE:
      {
         const GLuint plane = (GLuint) state[1];
         COPY_4V(value, ctx->Transform.EyeUserPlane[plane]);
      }
      return;
   case STATE_POINT_SIZE:
      value[0] = ctx->Point.Size;
      value[1] = ctx->Point.MinSize;
      value[2] = ctx->Point.MaxSize;
      value[3] = ctx->Point.Threshold;
      return;
   case STATE_POINT_ATTENUATION:
      value[0] = ctx->Point.Params[0];
      value[1] = ctx->Point.Params[1];
      value[2] = ctx->Point.Params[2];
      value[3] = 1.0F;
      return;
   case STATE_MATRIX:
      {
         /* state[1] = modelview, projection, texture, etc. */
         /* state[2] = which texture matrix or program matrix */
         /* state[3] = first column to fetch */
         /* state[4] = last column to fetch */
         /* state[5] = transpose, inverse or invtrans */

         const GLmatrix *matrix;
         const enum state_index mat = state[1];
         const GLuint index = (GLuint) state[2];
         const GLuint first = (GLuint) state[3];
         const GLuint last = (GLuint) state[4];
         const enum state_index modifier = state[5];
         const GLfloat *m;
         GLuint row, i;
         if (mat == STATE_MODELVIEW) {
            matrix = ctx->ModelviewMatrixStack.Top;
         }
         else if (mat == STATE_PROJECTION) {
            matrix = ctx->ProjectionMatrixStack.Top;
         }
         else if (mat == STATE_MVP) {
            matrix = &ctx->_ModelProjectMatrix;
         }
         else if (mat == STATE_TEXTURE) {
            matrix = ctx->TextureMatrixStack[index].Top;
         }
         else if (mat == STATE_PROGRAM) {
            matrix = ctx->ProgramMatrixStack[index].Top;
         }
         else {
            _mesa_problem(ctx, "Bad matrix name in _mesa_fetch_state()");
            return;
         }
         if (modifier == STATE_MATRIX_INVERSE ||
             modifier == STATE_MATRIX_INVTRANS) {
            /* Be sure inverse is up to date:
	     */
            _math_matrix_alloc_inv( (GLmatrix *) matrix );
	    _math_matrix_analyse( (GLmatrix*) matrix );
            m = matrix->inv;
         }
         else {
            m = matrix->m;
         }
         if (modifier == STATE_MATRIX_TRANSPOSE ||
             modifier == STATE_MATRIX_INVTRANS) {
            for (i = 0, row = first; row <= last; row++) {
               value[i++] = m[row * 4 + 0];
               value[i++] = m[row * 4 + 1];
               value[i++] = m[row * 4 + 2];
               value[i++] = m[row * 4 + 3];
            }
         }
         else {
            for (i = 0, row = first; row <= last; row++) {
               value[i++] = m[row + 0];
               value[i++] = m[row + 4];
               value[i++] = m[row + 8];
               value[i++] = m[row + 12];
            }
         }
      }
      return;
   case STATE_DEPTH_RANGE:
      value[0] = ctx->Viewport.Near;                     /* near       */
      value[1] = ctx->Viewport.Far;                      /* far        */
      value[2] = ctx->Viewport.Far - ctx->Viewport.Near; /* far - near */
      value[3] = 0;
      return;
   case STATE_FRAGMENT_PROGRAM:
      {
         /* state[1] = {STATE_ENV, STATE_LOCAL} */
         /* state[2] = parameter index          */
         const int idx = (int) state[2];
         switch (state[1]) {
            case STATE_ENV:
               COPY_4V(value, ctx->FragmentProgram.Parameters[idx]);
               break;
            case STATE_LOCAL:
               COPY_4V(value, ctx->FragmentProgram.Current->Base.LocalParams[idx]);
               break;
            default:
               _mesa_problem(ctx, "Bad state switch in _mesa_fetch_state()");
               return;
         }
      }
      return;
		
   case STATE_VERTEX_PROGRAM:
      {
         /* state[1] = {STATE_ENV, STATE_LOCAL} */
         /* state[2] = parameter index          */
         const int idx = (int) state[2];
         switch (state[1]) {
            case STATE_ENV:
               COPY_4V(value, ctx->VertexProgram.Parameters[idx]);
               break;
            case STATE_LOCAL:
               COPY_4V(value, ctx->VertexProgram.Current->Base.LocalParams[idx]);
               break;
            default:
               _mesa_problem(ctx, "Bad state switch in _mesa_fetch_state()");
               return;
         }
      }
      return;

   case STATE_INTERNAL:
      {
         switch (state[1]) {
	    case STATE_NORMAL_SCALE:
               ASSIGN_4V(value, ctx->_ModelViewInvScale, 0, 0, 1);
               break;
	    case STATE_TEXRECT_SCALE: {
   	       const int unit = (int) state[2];
	       const struct gl_texture_object *texObj = ctx->Texture.Unit[unit]._Current;
	       if (texObj) {
		  struct gl_texture_image *texImage = texObj->Image[0][0];
		  ASSIGN_4V(value, 1.0 / texImage->Width, 1.0 / texImage->Height, 0, 1);
	       }
               break;
	    }
	    default:
	       /* unknown state indexes are silently ignored
	       *  should be handled by the driver.
	       */
               return;
         }
      }
      return;

   default:
      _mesa_problem(ctx, "Invalid state in _mesa_fetch_state");
      return;
   }
}


/**
 * Return a bitmask of the Mesa state flags (_NEW_* values) which would
 * indicate that the given context state may have changed.
 * The bitmask is used during validation to determine if we need to update
 * vertex/fragment program parameters (like "state.material.color") when
 * some GL state has changed.
 */
static GLbitfield
make_state_flags(const GLint state[])
{
   switch (state[0]) {
   case STATE_MATERIAL:
   case STATE_LIGHT:
   case STATE_LIGHTMODEL_AMBIENT:
   case STATE_LIGHTMODEL_SCENECOLOR:
   case STATE_LIGHTPROD:
      return _NEW_LIGHT;

   case STATE_TEXGEN:
   case STATE_TEXENV_COLOR:
      return _NEW_TEXTURE;

   case STATE_FOG_COLOR:
   case STATE_FOG_PARAMS:
      return _NEW_FOG;

   case STATE_CLIPPLANE:
      return _NEW_TRANSFORM;

   case STATE_POINT_SIZE:
   case STATE_POINT_ATTENUATION:
      return _NEW_POINT;

   case STATE_MATRIX:
      switch (state[1]) {
      case STATE_MODELVIEW:
	 return _NEW_MODELVIEW;
      case STATE_PROJECTION:
	 return _NEW_PROJECTION;
      case STATE_MVP:
	 return _NEW_MODELVIEW | _NEW_PROJECTION;
      case STATE_TEXTURE:
	 return _NEW_TEXTURE_MATRIX;
      case STATE_PROGRAM:
	 return _NEW_TRACK_MATRIX;
      default:
	 _mesa_problem(NULL, "unexpected matrix in make_state_flags()");
	 return 0;
      }

   case STATE_DEPTH_RANGE:
      return _NEW_VIEWPORT;

   case STATE_FRAGMENT_PROGRAM:
   case STATE_VERTEX_PROGRAM:
      return _NEW_PROGRAM;

   case STATE_INTERNAL:
      switch (state[1]) {
      case STATE_NORMAL_SCALE:
	 return _NEW_MODELVIEW;
      case STATE_TEXRECT_SCALE:
	 return _NEW_TEXTURE;
      default:
         /* unknown state indexes are silently ignored and
         *  no flag set, since it is handled by the driver.
         */
	 return 0;
      }

   default:
      _mesa_problem(NULL, "unexpected state[0] in make_state_flags()");
      return 0;
   }
}


static void
append(char *dst, const char *src)
{
   while (*dst)
      dst++;
   while (*src)
     *dst++ = *src++;
   *dst = 0;
}


static void
append_token(char *dst, enum state_index k)
{
   switch (k) {
   case STATE_MATERIAL:
      append(dst, "material.");
      break;
   case STATE_LIGHT:
      append(dst, "light");
      break;
   case STATE_LIGHTMODEL_AMBIENT:
      append(dst, "lightmodel.ambient");
      break;
   case STATE_LIGHTMODEL_SCENECOLOR:
      break;
   case STATE_LIGHTPROD:
      append(dst, "lightprod");
      break;
   case STATE_TEXGEN:
      append(dst, "texgen");
      break;
   case STATE_FOG_COLOR:
      append(dst, "fog.color");
      break;
   case STATE_FOG_PARAMS:
      append(dst, "fog.params");
      break;
   case STATE_CLIPPLANE:
      append(dst, "clip");
      break;
   case STATE_POINT_SIZE:
      append(dst, "point.size");
      break;
   case STATE_POINT_ATTENUATION:
      append(dst, "point.attenuation");
      break;
   case STATE_MATRIX:
      append(dst, "matrix.");
      break;
   case STATE_MODELVIEW:
      append(dst, "modelview");
      break;
   case STATE_PROJECTION:
      append(dst, "projection");
      break;
   case STATE_MVP:
      append(dst, "mvp");
      break;
   case STATE_TEXTURE:
      append(dst, "texture");
      break;
   case STATE_PROGRAM:
      append(dst, "program");
      break;
   case STATE_MATRIX_INVERSE:
      append(dst, ".inverse");
      break;
   case STATE_MATRIX_TRANSPOSE:
      append(dst, ".transpose");
      break;
   case STATE_MATRIX_INVTRANS:
      append(dst, ".invtrans");
      break;
   case STATE_AMBIENT:
      append(dst, "ambient");
      break;
   case STATE_DIFFUSE:
      append(dst, "diffuse");
      break;
   case STATE_SPECULAR:
      append(dst, "specular");
      break;
   case STATE_EMISSION:
      append(dst, "emission");
      break;
   case STATE_SHININESS:
      append(dst, "shininess");
      break;
   case STATE_HALF:
      append(dst, "half");
      break;
   case STATE_POSITION:
      append(dst, ".position");
      break;
   case STATE_ATTENUATION:
      append(dst, ".attenuation");
      break;
   case STATE_SPOT_DIRECTION:
      append(dst, ".spot.direction");
      break;
   case STATE_TEXGEN_EYE_S:
      append(dst, "eye.s");
      break;
   case STATE_TEXGEN_EYE_T:
      append(dst, "eye.t");
      break;
   case STATE_TEXGEN_EYE_R:
      append(dst, "eye.r");
      break;
   case STATE_TEXGEN_EYE_Q:
      append(dst, "eye.q");
      break;
   case STATE_TEXGEN_OBJECT_S:
      append(dst, "object.s");
      break;
   case STATE_TEXGEN_OBJECT_T:
      append(dst, "object.t");
      break;
   case STATE_TEXGEN_OBJECT_R:
      append(dst, "object.r");
      break;
   case STATE_TEXGEN_OBJECT_Q:
      append(dst, "object.q");
      break;
   case STATE_TEXENV_COLOR:
      append(dst, "texenv");
      break;
   case STATE_DEPTH_RANGE:
      append(dst, "depth.range");
      break;
   case STATE_VERTEX_PROGRAM:
   case STATE_FRAGMENT_PROGRAM:
      break;
   case STATE_ENV:
      append(dst, "env");
      break;
   case STATE_LOCAL:
      append(dst, "local");
      break;
   case STATE_INTERNAL:
   case STATE_NORMAL_SCALE:
   case STATE_POSITION_NORMALIZED:
      append(dst, "(internal)");
      break;
   default:
      ;
   }
}

static void
append_face(char *dst, GLint face)
{
   if (face == 0)
      append(dst, "front.");
   else
      append(dst, "back.");
}

static void
append_index(char *dst, GLint index)
{
   char s[20];
   _mesa_sprintf(s, "[%d].", index);
   append(dst, s);
}

/**
 * Make a string from the given state vector.
 * For example, return "state.matrix.texture[2].inverse".
 * Use _mesa_free() to deallocate the string.
 */
static const char *
make_state_string(const GLint state[6])
{
   char str[1000] = "";
   char tmp[30];

   append(str, "state.");
   append_token(str, (enum state_index) state[0]);

   switch (state[0]) {
   case STATE_MATERIAL:
      append_face(str, state[1]);
      append_token(str, (enum state_index) state[2]);
      break;
   case STATE_LIGHT:
      append(str, "light");
      append_index(str, state[1]); /* light number [i]. */
      append_token(str, (enum state_index) state[2]); /* coefficients */
      break;
   case STATE_LIGHTMODEL_AMBIENT:
      append(str, "lightmodel.ambient");
      break;
   case STATE_LIGHTMODEL_SCENECOLOR:
      if (state[1] == 0) {
         append(str, "lightmodel.front.scenecolor");
      }
      else {
         append(str, "lightmodel.back.scenecolor");
      }
      break;
   case STATE_LIGHTPROD:
      append_index(str, state[1]); /* light number [i]. */
      append_face(str, state[2]);
      append_token(str, (enum state_index) state[3]);
      break;
   case STATE_TEXGEN:
      append_index(str, state[1]); /* tex unit [i] */
      append_token(str, (enum state_index) state[2]); /* plane coef */
      break;
   case STATE_TEXENV_COLOR:
      append_index(str, state[1]); /* tex unit [i] */
      append(str, "color");
      break;
   case STATE_FOG_COLOR:
   case STATE_FOG_PARAMS:
      break;
   case STATE_CLIPPLANE:
      append_index(str, state[1]); /* plane [i] */
      append(str, "plane");
      break;
   case STATE_POINT_SIZE:
   case STATE_POINT_ATTENUATION:
      break;
   case STATE_MATRIX:
      {
         /* state[1] = modelview, projection, texture, etc. */
         /* state[2] = which texture matrix or program matrix */
         /* state[3] = first column to fetch */
         /* state[4] = last column to fetch */
         /* state[5] = transpose, inverse or invtrans */
         const enum state_index mat = (enum state_index) state[1];
         const GLuint index = (GLuint) state[2];
         const GLuint first = (GLuint) state[3];
         const GLuint last = (GLuint) state[4];
         const enum state_index modifier = (enum state_index) state[5];
         append_token(str, mat);
         if (index)
            append_index(str, index);
         if (modifier)
            append_token(str, modifier);
         if (first == last)
            _mesa_sprintf(tmp, ".row[%d]", first);
         else
            _mesa_sprintf(tmp, ".row[%d..%d]", first, last);
         append(str, tmp);
      }
      break;
   case STATE_DEPTH_RANGE:
      break;
   case STATE_FRAGMENT_PROGRAM:
   case STATE_VERTEX_PROGRAM:
      /* state[1] = {STATE_ENV, STATE_LOCAL} */
      /* state[2] = parameter index          */
      append_token(str, (enum state_index) state[1]);
      append_index(str, state[2]);
      break;
   case STATE_INTERNAL:
      break;
   default:
      _mesa_problem(NULL, "Invalid state in make_state_string");
      break;
   }

   return _mesa_strdup(str);
}


/**
 * Loop over all the parameters in a parameter list.  If the parameter
 * is a GL state reference, look up the current value of that state
 * variable and put it into the parameter's Value[4] array.
 * This would be called at glBegin time when using a fragment program.
 */
void
_mesa_load_state_parameters(GLcontext *ctx,
                            struct gl_program_parameter_list *paramList)
{
   GLuint i;

   if (!paramList)
      return;

   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Type == PROGRAM_STATE_VAR) {
         _mesa_fetch_state(ctx, 
			   paramList->Parameters[i].StateIndexes,
                           paramList->ParameterValues[i]);
      }
   }
}


/**
 * Initialize program instruction fields to defaults.
 * \param inst  first instruction to initialize
 * \param count  number of instructions to initialize
 */
void
_mesa_init_instructions(struct prog_instruction *inst, GLuint count)
{
   GLuint i;

   _mesa_bzero(inst, count * sizeof(struct prog_instruction));

   for (i = 0; i < count; i++) {
      inst[i].SrcReg[0].File = PROGRAM_UNDEFINED;
      inst[i].SrcReg[0].Swizzle = SWIZZLE_NOOP;
      inst[i].SrcReg[1].File = PROGRAM_UNDEFINED;
      inst[i].SrcReg[1].Swizzle = SWIZZLE_NOOP;
      inst[i].SrcReg[2].File = PROGRAM_UNDEFINED;
      inst[i].SrcReg[2].Swizzle = SWIZZLE_NOOP;

      inst[i].DstReg.File = PROGRAM_UNDEFINED;
      inst[i].DstReg.WriteMask = WRITEMASK_XYZW;
      inst[i].DstReg.CondMask = COND_TR;
      inst[i].DstReg.CondSwizzle = SWIZZLE_NOOP;

      inst[i].SaturateMode = SATURATE_OFF;
      inst[i].Precision = FLOAT32;
   }
}


/**
 * Allocate an array of program instructions.
 * \param numInst  number of instructions
 * \return pointer to instruction memory
 */
struct prog_instruction *
_mesa_alloc_instructions(GLuint numInst)
{
   return (struct prog_instruction *)
      _mesa_calloc(numInst * sizeof(struct prog_instruction));
}


/**
 * Reallocate memory storing an array of program instructions.
 * This is used when we need to append additional instructions onto an
 * program.
 * \param oldInst  pointer to first of old/src instructions
 * \param numOldInst  number of instructions at <oldInst>
 * \param numNewInst  desired size of new instruction array.
 * \return  pointer to start of new instruction array.
 */
struct prog_instruction *
_mesa_realloc_instructions(struct prog_instruction *oldInst,
                           GLuint numOldInst, GLuint numNewInst)
{
   struct prog_instruction *newInst;

   newInst = (struct prog_instruction *)
      _mesa_realloc(oldInst,
                    numOldInst * sizeof(struct prog_instruction),
                    numNewInst * sizeof(struct prog_instruction));

   return newInst;
}


/**
 * Basic info about each instruction
 */
struct instruction_info
{
   enum prog_opcode Opcode;
   const char *Name;
   GLuint NumSrcRegs;
};

/**
 * Instruction info
 * \note Opcode should equal array index!
 */
static const struct instruction_info InstInfo[MAX_OPCODE] = {
   { OPCODE_ABS,    "ABS",   1 },
   { OPCODE_ADD,    "ADD",   2 },
   { OPCODE_ARA,    "ARA",   1 },
   { OPCODE_ARL,    "ARL",   1 },
   { OPCODE_ARL_NV, "ARL",   1 },
   { OPCODE_ARR,    "ARL",   1 },
   { OPCODE_BRA,    "BRA",   1 },
   { OPCODE_CAL,    "CAL",   1 },
   { OPCODE_CMP,    "CMP",   3 },
   { OPCODE_COS,    "COS",   1 },
   { OPCODE_DDX,    "DDX",   1 },
   { OPCODE_DDY,    "DDY",   1 },
   { OPCODE_DP3,    "DP3",   2 },
   { OPCODE_DP4,    "DP4",   2 },
   { OPCODE_DPH,    "DPH",   2 },
   { OPCODE_DST,    "DST",   2 },
   { OPCODE_END,    "END",   0 },
   { OPCODE_EX2,    "EX2",   1 },
   { OPCODE_EXP,    "EXP",   1 },
   { OPCODE_FLR,    "FLR",   1 },
   { OPCODE_FRC,    "FRC",   1 },
   { OPCODE_KIL,    "KIL",   1 },
   { OPCODE_KIL_NV, "KIL",   0 },
   { OPCODE_LG2,    "LG2",   1 },
   { OPCODE_LIT,    "LIT",   1 },
   { OPCODE_LOG,    "LOG",   1 },
   { OPCODE_LRP,    "LRP",   3 },
   { OPCODE_MAD,    "MAD",   3 },
   { OPCODE_MAX,    "MAX",   2 },
   { OPCODE_MIN,    "MIN",   2 },
   { OPCODE_MOV,    "MOV",   1 },
   { OPCODE_MUL,    "MUL",   2 },
   { OPCODE_PK2H,   "PK2H",  1 },
   { OPCODE_PK2US,  "PK2US", 1 },
   { OPCODE_PK4B,   "PK4B",  1 },
   { OPCODE_PK4UB,  "PK4UB", 1 },
   { OPCODE_POW,    "POW",   2 },
   { OPCODE_POPA,   "POPA",  0 },
   { OPCODE_PRINT,  "PRINT", 1 },
   { OPCODE_PUSHA,  "PUSHA", 0 },
   { OPCODE_RCC,    "RCC",   1 },
   { OPCODE_RCP,    "RCP",   1 },
   { OPCODE_RET,    "RET",   1 },
   { OPCODE_RFL,    "RFL",   1 },
   { OPCODE_RSQ,    "RSQ",   1 },
   { OPCODE_SCS,    "SCS",   1 },
   { OPCODE_SEQ,    "SEQ",   2 },
   { OPCODE_SFL,    "SFL",   0 },
   { OPCODE_SGE,    "SGE",   2 },
   { OPCODE_SGT,    "SGT",   2 },
   { OPCODE_SIN,    "SIN",   1 },
   { OPCODE_SLE,    "SLE",   2 },
   { OPCODE_SLT,    "SLT",   2 },
   { OPCODE_SNE,    "SNE",   2 },
   { OPCODE_SSG,    "SSG",   1 },
   { OPCODE_STR,    "STR",   0 },
   { OPCODE_SUB,    "SUB",   2 },
   { OPCODE_SWZ,    "SWZ",   1 },
   { OPCODE_TEX,    "TEX",   1 },
   { OPCODE_TXB,    "TXB",   1 },
   { OPCODE_TXD,    "TXD",   3 },
   { OPCODE_TXL,    "TXL",   1 },
   { OPCODE_TXP,    "TXP",   1 },
   { OPCODE_TXP_NV, "TXP",   1 },
   { OPCODE_UP2H,   "UP2H",  1 },
   { OPCODE_UP2US,  "UP2US", 1 },
   { OPCODE_UP4B,   "UP4B",  1 },
   { OPCODE_UP4UB,  "UP4UB", 1 },
   { OPCODE_X2D,    "X2D",   3 },
   { OPCODE_XPD,    "XPD",   2 }
};


/**
 * Return the number of src registers for the given instruction/opcode.
 */
GLuint
_mesa_num_inst_src_regs(enum prog_opcode opcode)
{
   ASSERT(opcode == InstInfo[opcode].Opcode);
   return InstInfo[opcode].NumSrcRegs;
}


/**
 * Return string name for given program opcode.
 */
const char *
_mesa_opcode_string(enum prog_opcode opcode)
{
   ASSERT(opcode < MAX_OPCODE);
   return InstInfo[opcode].Name;
}

/**
 * Return string name for given program/register file.
 */
static const char *
program_file_string(enum register_file f)
{
   switch (f) {
   case PROGRAM_TEMPORARY:
      return "TEMP";
   case PROGRAM_LOCAL_PARAM:
      return "LOCAL";
   case PROGRAM_ENV_PARAM:
      return "ENV";
   case PROGRAM_STATE_VAR:
      return "STATE";
   case PROGRAM_INPUT:
      return "INPUT";
   case PROGRAM_OUTPUT:
      return "OUTPUT";
   case PROGRAM_NAMED_PARAM:
      return "NAMED";
   case PROGRAM_CONSTANT:
      return "CONST";
   case PROGRAM_WRITE_ONLY:
      return "WRITE_ONLY";
   case PROGRAM_ADDRESS:
      return "ADDR";
   default:
      return "!unkown!";
   }
}


/**
 * Return a string representation of the given swizzle word.
 * If extended is true, use extended (comma-separated) format.
 */
static const char *
swizzle_string(GLuint swizzle, GLuint negateBase, GLboolean extended)
{
   static const char swz[] = "xyzw01";
   static char s[20];
   GLuint i = 0;

   if (!extended && swizzle == SWIZZLE_NOOP && negateBase == 0)
      return ""; /* no swizzle/negation */

   if (!extended)
      s[i++] = '.';

   if (negateBase & 0x1)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 0)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateBase & 0x2)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 1)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateBase & 0x4)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 2)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateBase & 0x8)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 3)];

   s[i] = 0;
   return s;
}


static const char *
writemask_string(GLuint writeMask)
{
   static char s[10];
   GLuint i = 0;

   if (writeMask == WRITEMASK_XYZW)
      return "";

   s[i++] = '.';
   if (writeMask & WRITEMASK_X)
      s[i++] = 'x';
   if (writeMask & WRITEMASK_Y)
      s[i++] = 'y';
   if (writeMask & WRITEMASK_Z)
      s[i++] = 'z';
   if (writeMask & WRITEMASK_W)
      s[i++] = 'w';

   s[i] = 0;
   return s;
}

static void
print_dst_reg(const struct prog_dst_register *dstReg)
{
   _mesa_printf(" %s[%d]%s",
                program_file_string((enum register_file) dstReg->File),
                dstReg->Index,
                writemask_string(dstReg->WriteMask));
}

static void
print_src_reg(const struct prog_src_register *srcReg)
{
   _mesa_printf("%s[%d]%s",
                program_file_string((enum register_file) srcReg->File),
                srcReg->Index,
                swizzle_string(srcReg->Swizzle,
                               srcReg->NegateBase, GL_FALSE));
}

void
_mesa_print_alu_instruction(const struct prog_instruction *inst,
			    const char *opcode_string, 
			    GLuint numRegs)
{
   GLuint j;

   _mesa_printf("%s", opcode_string);

   /* frag prog only */
   if (inst->SaturateMode == SATURATE_ZERO_ONE)
      _mesa_printf("_SAT");

   if (inst->DstReg.File != PROGRAM_UNDEFINED) {
      _mesa_printf(" %s[%d]%s",
		   program_file_string((enum register_file) inst->DstReg.File),
		   inst->DstReg.Index,
		   writemask_string(inst->DstReg.WriteMask));
   }

   if (numRegs > 0)
      _mesa_printf(", ");

   for (j = 0; j < numRegs; j++) {
      print_src_reg(inst->SrcReg + j);
      if (j + 1 < numRegs)
	 _mesa_printf(", ");
   }

   _mesa_printf(";\n");
}


/**
 * Print a single vertex/fragment program instruction.
 */
void
_mesa_print_instruction(const struct prog_instruction *inst)
{
   switch (inst->Opcode) {
   case OPCODE_PRINT:
      _mesa_printf("PRINT '%s'", inst->Data);
      if (inst->SrcReg[0].File != PROGRAM_UNDEFINED) {
         _mesa_printf(", ");
         _mesa_printf("%s[%d]%s",
                      program_file_string((enum register_file) inst->SrcReg[0].File),
                      inst->SrcReg[0].Index,
                      swizzle_string(inst->SrcReg[0].Swizzle,
                                     inst->SrcReg[0].NegateBase, GL_FALSE));
      }
      _mesa_printf(";\n");
      break;
   case OPCODE_SWZ:
      _mesa_printf("SWZ");
      if (inst->SaturateMode == SATURATE_ZERO_ONE)
         _mesa_printf("_SAT");
      print_dst_reg(&inst->DstReg);
      _mesa_printf("%s[%d], %s;\n",
                   program_file_string((enum register_file) inst->SrcReg[0].File),
                   inst->SrcReg[0].Index,
                   swizzle_string(inst->SrcReg[0].Swizzle,
                                  inst->SrcReg[0].NegateBase, GL_TRUE));
      break;
   case OPCODE_TEX:
   case OPCODE_TXP:
   case OPCODE_TXB:
      _mesa_printf("%s", _mesa_opcode_string(inst->Opcode));
      if (inst->SaturateMode == SATURATE_ZERO_ONE)
         _mesa_printf("_SAT");
      _mesa_printf(" ");
      print_dst_reg(&inst->DstReg);
      _mesa_printf(", ");
      print_src_reg(&inst->SrcReg[0]);
      _mesa_printf(", texture[%d], ", inst->TexSrcUnit);
      switch (inst->TexSrcTarget) {
      case TEXTURE_1D_INDEX:   _mesa_printf("1D");    break;
      case TEXTURE_2D_INDEX:   _mesa_printf("2D");    break;
      case TEXTURE_3D_INDEX:   _mesa_printf("3D");    break;
      case TEXTURE_CUBE_INDEX: _mesa_printf("CUBE");  break;
      case TEXTURE_RECT_INDEX: _mesa_printf("RECT");  break;
      default:
         ;
      }
      _mesa_printf("\n");
      break;
   case OPCODE_ARL:
      _mesa_printf("ARL addr.x, ");
      print_src_reg(&inst->SrcReg[0]);
      _mesa_printf(";\n");
      break;
   case OPCODE_END:
      _mesa_printf("END;\n");
      break;
   /* XXX may need for other special-case instructions */
   default:
      /* typical alu instruction */
      _mesa_print_alu_instruction(inst,
				  _mesa_opcode_string(inst->Opcode),
				  _mesa_num_inst_src_regs(inst->Opcode));
      break;
   }
}


/**
 * Print a vertx/fragment program to stdout.
 * XXX this function could be greatly improved.
 */
void
_mesa_print_program(const struct gl_program *prog)
{
   GLuint i;
   for (i = 0; i < prog->NumInstructions; i++) {
      _mesa_printf("%3d: ", i);
      _mesa_print_instruction(prog->Instructions + i);
   }
}


/**
 * Print all of a program's parameters.
 */
void
_mesa_print_program_parameters(GLcontext *ctx, const struct gl_program *prog)
{
   GLint i;

   _mesa_printf("NumInstructions=%d\n", prog->NumInstructions);
   _mesa_printf("NumTemporaries=%d\n", prog->NumTemporaries);
   _mesa_printf("NumParameters=%d\n", prog->NumParameters);
   _mesa_printf("NumAttributes=%d\n", prog->NumAttributes);
   _mesa_printf("NumAddressRegs=%d\n", prog->NumAddressRegs);
	
   _mesa_load_state_parameters(ctx, prog->Parameters);
			
#if 0	
   _mesa_printf("Local Params:\n");
   for (i = 0; i < MAX_PROGRAM_LOCAL_PARAMS; i++){
      const GLfloat *p = prog->LocalParams[i];
      _mesa_printf("%2d: %f, %f, %f, %f\n", i, p[0], p[1], p[2], p[3]);
   }
#endif	

   for (i = 0; i < prog->Parameters->NumParameters; i++){
      struct gl_program_parameter *param = prog->Parameters->Parameters + i;
      const GLfloat *v = prog->Parameters->ParameterValues[i];
      _mesa_printf("param[%d] %s = {%.3f, %.3f, %.3f, %.3f};\n",
                   i, param->Name, v[0], v[1], v[2], v[3]);
   }
}


/**
 * Mixing ARB and NV vertex/fragment programs can be tricky.
 * Note: GL_VERTEX_PROGRAM_ARB == GL_VERTEX_PROGRAM_NV
 *  but, GL_FRAGMENT_PROGRAM_ARB != GL_FRAGMENT_PROGRAM_NV
 * The two different fragment program targets are supposed to be compatible
 * to some extent (see GL_ARB_fragment_program spec).
 * This function does the compatibility check.
 */
static GLboolean
compatible_program_targets(GLenum t1, GLenum t2)
{
   if (t1 == t2)
      return GL_TRUE;
   if (t1 == GL_FRAGMENT_PROGRAM_ARB && t2 == GL_FRAGMENT_PROGRAM_NV)
      return GL_TRUE;
   if (t1 == GL_FRAGMENT_PROGRAM_NV && t2 == GL_FRAGMENT_PROGRAM_ARB)
      return GL_TRUE;
   return GL_FALSE;
}



/**********************************************************************/
/* API functions                                                      */
/**********************************************************************/


/**
 * Bind a program (make it current)
 * \note Called from the GL API dispatcher by both glBindProgramNV
 * and glBindProgramARB.
 */
void GLAPIENTRY
_mesa_BindProgram(GLenum target, GLuint id)
{
   struct gl_program *curProg, *newProg;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   /* Error-check target and get curProg */
   if ((target == GL_VERTEX_PROGRAM_ARB) && /* == GL_VERTEX_PROGRAM_NV */
        (ctx->Extensions.NV_vertex_program ||
         ctx->Extensions.ARB_vertex_program)) {
      curProg = &ctx->VertexProgram.Current->Base;
   }
   else if ((target == GL_FRAGMENT_PROGRAM_NV
             && ctx->Extensions.NV_fragment_program) ||
            (target == GL_FRAGMENT_PROGRAM_ARB
             && ctx->Extensions.ARB_fragment_program)) {
      curProg = &ctx->FragmentProgram.Current->Base;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindProgramNV/ARB(target)");
      return;
   }

   /*
    * Get pointer to new program to bind.
    * NOTE: binding to a non-existant program is not an error.
    * That's supposed to be caught in glBegin.
    */
   if (id == 0) {
      /* Bind a default program */
      newProg = NULL;
      if (target == GL_VERTEX_PROGRAM_ARB) /* == GL_VERTEX_PROGRAM_NV */
         newProg = ctx->Shared->DefaultVertexProgram;
      else
         newProg = ctx->Shared->DefaultFragmentProgram;
   }
   else {
      /* Bind a user program */
      newProg = _mesa_lookup_program(ctx, id);
      if (!newProg || newProg == &_mesa_DummyProgram) {
         /* allocate a new program now */
         newProg = ctx->Driver.NewProgram(ctx, target, id);
         if (!newProg) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindProgramNV/ARB");
            return;
         }
         _mesa_HashInsert(ctx->Shared->Programs, id, newProg);
      }
      else if (!compatible_program_targets(newProg->Target, target)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBindProgramNV/ARB(target mismatch)");
         return;
      }
   }

   /** All error checking is complete now **/

   if (curProg->Id == id) {
      /* binding same program - no change */
      return;
   }

   /* unbind/delete oldProg */
   if (curProg->Id != 0) {
      /* decrement refcount on previously bound fragment program */
      curProg->RefCount--;
      /* and delete if refcount goes below one */
      if (curProg->RefCount <= 0) {
         /* the program ID was already removed from the hash table */
         ctx->Driver.DeleteProgram(ctx, curProg);
      }
   }

   /* bind newProg */
   if (target == GL_VERTEX_PROGRAM_ARB) { /* == GL_VERTEX_PROGRAM_NV */
      ctx->VertexProgram.Current = (struct gl_vertex_program *) newProg;
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV ||
            target == GL_FRAGMENT_PROGRAM_ARB) {
      ctx->FragmentProgram.Current = (struct gl_fragment_program *) newProg;
   }
   newProg->RefCount++;

   /* Never null pointers */
   ASSERT(ctx->VertexProgram.Current);
   ASSERT(ctx->FragmentProgram.Current);

   if (ctx->Driver.BindProgram)
      ctx->Driver.BindProgram(ctx, target, newProg);
}


/**
 * Delete a list of programs.
 * \note Not compiled into display lists.
 * \note Called by both glDeleteProgramsNV and glDeleteProgramsARB.
 */
void GLAPIENTRY 
_mesa_DeletePrograms(GLsizei n, const GLuint *ids)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glDeleteProgramsNV" );
      return;
   }

   for (i = 0; i < n; i++) {
      if (ids[i] != 0) {
         struct gl_program *prog = _mesa_lookup_program(ctx, ids[i]);
         if (prog == &_mesa_DummyProgram) {
            _mesa_HashRemove(ctx->Shared->Programs, ids[i]);
         }
         else if (prog) {
            /* Unbind program if necessary */
            if (prog->Target == GL_VERTEX_PROGRAM_ARB || /* == GL_VERTEX_PROGRAM_NV */
                prog->Target == GL_VERTEX_STATE_PROGRAM_NV) {
               if (ctx->VertexProgram.Current &&
                   ctx->VertexProgram.Current->Base.Id == ids[i]) {
                  /* unbind this currently bound program */
                  _mesa_BindProgram(prog->Target, 0);
               }
            }
            else if (prog->Target == GL_FRAGMENT_PROGRAM_NV ||
                     prog->Target == GL_FRAGMENT_PROGRAM_ARB) {
               if (ctx->FragmentProgram.Current &&
                   ctx->FragmentProgram.Current->Base.Id == ids[i]) {
                  /* unbind this currently bound program */
                  _mesa_BindProgram(prog->Target, 0);
               }
            }
            else {
               _mesa_problem(ctx, "bad target in glDeleteProgramsNV");
               return;
            }
            /* The ID is immediately available for re-use now */
            _mesa_HashRemove(ctx->Shared->Programs, ids[i]);
            prog->RefCount--;
            if (prog->RefCount <= 0) {
               ctx->Driver.DeleteProgram(ctx, prog);
            }
         }
      }
   }
}


/**
 * Generate a list of new program identifiers.
 * \note Not compiled into display lists.
 * \note Called by both glGenProgramsNV and glGenProgramsARB.
 */
void GLAPIENTRY
_mesa_GenPrograms(GLsizei n, GLuint *ids)
{
   GLuint first;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenPrograms");
      return;
   }

   if (!ids)
      return;

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->Programs, n);

   /* Insert pointer to dummy program as placeholder */
   for (i = 0; i < (GLuint) n; i++) {
      _mesa_HashInsert(ctx->Shared->Programs, first + i, &_mesa_DummyProgram);
   }

   /* Return the program names */
   for (i = 0; i < (GLuint) n; i++) {
      ids[i] = first + i;
   }
}


/**********************************************************************/
/* GL_MESA_program_debug extension                                    */
/**********************************************************************/


/* XXX temporary */
GLAPI void GLAPIENTRY
glProgramCallbackMESA(GLenum target, GLprogramcallbackMESA callback,
                      GLvoid *data)
{
   _mesa_ProgramCallbackMESA(target, callback, data);
}


void
_mesa_ProgramCallbackMESA(GLenum target, GLprogramcallbackMESA callback,
                          GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);

   switch (target) {
      case GL_FRAGMENT_PROGRAM_ARB:
         if (!ctx->Extensions.ARB_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
            return;
         }
         ctx->FragmentProgram.Callback = callback;
         ctx->FragmentProgram.CallbackData = data;
         break;
      case GL_FRAGMENT_PROGRAM_NV:
         if (!ctx->Extensions.NV_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
            return;
         }
         ctx->FragmentProgram.Callback = callback;
         ctx->FragmentProgram.CallbackData = data;
         break;
      case GL_VERTEX_PROGRAM_ARB: /* == GL_VERTEX_PROGRAM_NV */
         if (!ctx->Extensions.ARB_vertex_program &&
             !ctx->Extensions.NV_vertex_program) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
            return;
         }
         ctx->VertexProgram.Callback = callback;
         ctx->VertexProgram.CallbackData = data;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
         return;
   }
}


/* XXX temporary */
GLAPI void GLAPIENTRY
glGetProgramRegisterfvMESA(GLenum target,
                           GLsizei len, const GLubyte *registerName,
                           GLfloat *v)
{
   _mesa_GetProgramRegisterfvMESA(target, len, registerName, v);
}


void
_mesa_GetProgramRegisterfvMESA(GLenum target,
                               GLsizei len, const GLubyte *registerName,
                               GLfloat *v)
{
   char reg[1000];
   GET_CURRENT_CONTEXT(ctx);

   /* We _should_ be inside glBegin/glEnd */
#if 0
   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramRegisterfvMESA");
      return;
   }
#endif

   /* make null-terminated copy of registerName */
   len = MIN2((unsigned int) len, sizeof(reg) - 1);
   _mesa_memcpy(reg, registerName, len);
   reg[len] = 0;

   switch (target) {
      case GL_VERTEX_PROGRAM_ARB: /* == GL_VERTEX_PROGRAM_NV */
         if (!ctx->Extensions.ARB_vertex_program &&
             !ctx->Extensions.NV_vertex_program) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetProgramRegisterfvMESA(target)");
            return;
         }
         if (!ctx->VertexProgram._Enabled) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetProgramRegisterfvMESA");
            return;
         }
         /* GL_NV_vertex_program */
         if (reg[0] == 'R') {
            /* Temp register */
            GLint i = _mesa_atoi(reg + 1);
            if (i >= (GLint)ctx->Const.VertexProgram.MaxTemps) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glGetProgramRegisterfvMESA(registerName)");
               return;
            }
#if 0 /* FIX ME */
            ctx->Driver.GetVertexProgramRegister(ctx, PROGRAM_TEMPORARY, i, v);
#endif
         }
         else if (reg[0] == 'v' && reg[1] == '[') {
            /* Vertex Input attribute */
            GLuint i;
            for (i = 0; i < ctx->Const.VertexProgram.MaxAttribs; i++) {
               const char *name = _mesa_nv_vertex_input_register_name(i);
               char number[10];
               _mesa_sprintf(number, "%d", i);
               if (_mesa_strncmp(reg + 2, name, 4) == 0 ||
                   _mesa_strncmp(reg + 2, number, _mesa_strlen(number)) == 0) {
#if 0 /* FIX ME */
                  ctx->Driver.GetVertexProgramRegister(ctx, PROGRAM_INPUT,
                                                       i, v);
#endif
                  return;
               }
            }
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramRegisterfvMESA(registerName)");
            return;
         }
         else if (reg[0] == 'o' && reg[1] == '[') {
            /* Vertex output attribute */
         }
         /* GL_ARB_vertex_program */
         else if (_mesa_strncmp(reg, "vertex.", 7) == 0) {

         }
         else {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramRegisterfvMESA(registerName)");
            return;
         }
         break;
      case GL_FRAGMENT_PROGRAM_ARB:
         if (!ctx->Extensions.ARB_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetProgramRegisterfvMESA(target)");
            return;
         }
         if (!ctx->FragmentProgram._Enabled) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetProgramRegisterfvMESA");
            return;
         }
         /* XXX to do */
         break;
      case GL_FRAGMENT_PROGRAM_NV:
         if (!ctx->Extensions.NV_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetProgramRegisterfvMESA(target)");
            return;
         }
         if (!ctx->FragmentProgram._Enabled) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetProgramRegisterfvMESA");
            return;
         }
         if (reg[0] == 'R') {
            /* Temp register */
            GLint i = _mesa_atoi(reg + 1);
            if (i >= (GLint)ctx->Const.FragmentProgram.MaxTemps) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glGetProgramRegisterfvMESA(registerName)");
               return;
            }
            ctx->Driver.GetFragmentProgramRegister(ctx, PROGRAM_TEMPORARY,
                                                   i, v);
         }
         else if (reg[0] == 'f' && reg[1] == '[') {
            /* Fragment input attribute */
            GLuint i;
            for (i = 0; i < ctx->Const.FragmentProgram.MaxAttribs; i++) {
               const char *name = _mesa_nv_fragment_input_register_name(i);
               if (_mesa_strncmp(reg + 2, name, 4) == 0) {
                  ctx->Driver.GetFragmentProgramRegister(ctx,
                                                         PROGRAM_INPUT, i, v);
                  return;
               }
            }
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramRegisterfvMESA(registerName)");
            return;
         }
         else if (_mesa_strcmp(reg, "o[COLR]") == 0) {
            /* Fragment output color */
            ctx->Driver.GetFragmentProgramRegister(ctx, PROGRAM_OUTPUT,
                                                   FRAG_RESULT_COLR, v);
         }
         else if (_mesa_strcmp(reg, "o[COLH]") == 0) {
            /* Fragment output color */
            ctx->Driver.GetFragmentProgramRegister(ctx, PROGRAM_OUTPUT,
                                                   FRAG_RESULT_COLH, v);
         }
         else if (_mesa_strcmp(reg, "o[DEPR]") == 0) {
            /* Fragment output depth */
            ctx->Driver.GetFragmentProgramRegister(ctx, PROGRAM_OUTPUT,
                                                   FRAG_RESULT_DEPR, v);
         }
         else {
            /* try user-defined identifiers */
            const GLfloat *value = _mesa_lookup_parameter_value(
                       ctx->FragmentProgram.Current->Base.Parameters, -1, reg);
            if (value) {
               COPY_4V(v, value);
            }
            else {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glGetProgramRegisterfvMESA(registerName)");
               return;
            }
         }
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glGetProgramRegisterfvMESA(target)");
         return;
   }
}
