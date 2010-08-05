/**
 * Utilities for OpenGL shading language
 *
 * Brian Paul
 * 9 April 2008
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"

/** time to compile previous shader */
static GLdouble CompileTime = 0.0;

/** time to linke previous program */
static GLdouble LinkTime = 0.0;


GLboolean
ShadersSupported(void)
{
   const char *version = (const char *) glGetString(GL_VERSION);

   /* NVIDIA binary drivers will return "3.0.0", and they clearly support
    * shaders.
    */
   if (version[0] >= '2' && version[1] == '.') {
      return GL_TRUE;
   }
   else if (glutExtensionSupported("GL_ARB_vertex_shader")
            && glutExtensionSupported("GL_ARB_fragment_shader")
            && glutExtensionSupported("GL_ARB_shader_objects")) {
      fprintf(stderr, "Warning: Trying ARB GLSL instead of OpenGL 2.x.  This may not work.\n");
      return GL_TRUE;
   }
   fprintf(stderr, "Sorry, GLSL not supported with this OpenGL.\n");
   return GL_FALSE;
}


GLuint
CompileShaderText(GLenum shaderType, const char *text)
{
   GLuint shader;
   GLint stat;
   GLdouble t0, t1;

   shader = glCreateShader(shaderType);
   glShaderSource(shader, 1, (const GLchar **) &text, NULL);

   t0 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   glCompileShader(shader);
   t1 = glutGet(GLUT_ELAPSED_TIME) * 0.001;

   CompileTime = t1 - t0;

   glGetShaderiv(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog(shader, 1000, &len, log);
      fprintf(stderr, "Error: problem compiling shader: %s\n", log);
      exit(1);
   }
   else {
      /*printf("Shader compiled OK\n");*/
   }
   return shader;
}


/**
 * Read a shader from a file.
 */
GLuint
CompileShaderFile(GLenum shaderType, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   GLuint shader;
   FILE *f;

   f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "Unable to open shader file %s\n", filename);
      free(buffer);
      return 0;
   }

   n = fread(buffer, 1, max, f);
   /*printf("read %d bytes from shader file %s\n", n, filename);*/
   if (n > 0) {
      buffer[n] = 0;
      shader = CompileShaderText(shaderType, buffer);
   }
   else {
      fclose(f);
      free(buffer);
      return 0;
   }

   fclose(f);
   free(buffer);

   return shader;
}


GLuint
LinkShaders(GLuint vertShader, GLuint fragShader)
{
   GLuint program = glCreateProgram();
   GLdouble t0, t1;

   assert(vertShader || fragShader);

   if (fragShader)
      glAttachShader(program, fragShader);
   if (vertShader)
      glAttachShader(program, vertShader);

   t0 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   glLinkProgram(program);
   t1 = glutGet(GLUT_ELAPSED_TIME) * 0.001;

   LinkTime = t1 - t0;

   /* check link */
   {
      GLint stat;
      glGetProgramiv(program, GL_LINK_STATUS, &stat);
      if (!stat) {
         GLchar log[1000];
         GLsizei len;
         glGetProgramInfoLog(program, 1000, &len, log);
         fprintf(stderr, "Shader link error:\n%s\n", log);
         return 0;
      }
   }

   return program;
}


GLboolean
ValidateShaderProgram(GLuint program)
{
   GLint stat;
   glValidateProgramARB(program);
   glGetProgramiv(program, GL_VALIDATE_STATUS, &stat);

   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetProgramInfoLog(program, 1000, &len, log);
      fprintf(stderr, "Program validation error:\n%s\n", log);
      return 0;
   }

   return (GLboolean) stat;
}


GLdouble
GetShaderCompileTime(void)
{
   return CompileTime;
}


GLdouble
GetShaderLinkTime(void)
{
   return LinkTime;
}


void
SetUniformValues(GLuint program, struct uniform_info uniforms[])
{
   GLuint i;

   for (i = 0; uniforms[i].name; i++) {
      uniforms[i].location
         = glGetUniformLocation(program, uniforms[i].name);

      switch (uniforms[i].type) {
      case GL_INT:
      case GL_SAMPLER_1D:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_3D:
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_2D_RECT_ARB:
         assert(uniforms[i].value[0] >= 0.0F);
         glUniform1i(uniforms[i].location,
                     (GLint) uniforms[i].value[0]);
         break;
      case GL_FLOAT:
         glUniform1fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case GL_FLOAT_VEC2:
         glUniform2fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case GL_FLOAT_VEC3:
         glUniform3fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case GL_FLOAT_VEC4:
         glUniform4fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      default:
         if (strncmp(uniforms[i].name, "gl_", 3) == 0) {
            /* built-in uniform: ignore */
         }
         else {
            fprintf(stderr,
                    "Unexpected uniform data type in SetUniformValues\n");
            abort();
         }
      }
   }
}


/** Get list of uniforms used in the program */
GLuint
GetUniforms(GLuint program, struct uniform_info uniforms[])
{
   GLint n, max, i;

   glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);
   glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max);

   for (i = 0; i < n; i++) {
      GLint size, len;
      GLenum type;
      char name[100];

      glGetActiveUniform(program, i, 100, &len, &size, &type, name);

      uniforms[i].name = strdup(name);
      uniforms[i].size = size;
      uniforms[i].type = type;
      uniforms[i].location = glGetUniformLocation(program, name);
   }

   uniforms[i].name = NULL; /* end of list */

   return n;
}


void
PrintUniforms(const struct uniform_info uniforms[])
{
   GLint i;

   printf("Uniforms:\n");

   for (i = 0; uniforms[i].name; i++) {
      printf("  %d: %s size=%d type=0x%x loc=%d value=%g, %g, %g, %g\n",
             i,
             uniforms[i].name,
             uniforms[i].size,
             uniforms[i].type,
             uniforms[i].location,
             uniforms[i].value[0],
             uniforms[i].value[1],
             uniforms[i].value[2],
             uniforms[i].value[3]);
   }
}


/** Get list of attribs used in the program */
GLuint
GetAttribs(GLuint program, struct attrib_info attribs[])
{
   GLint n, max, i;

   glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &n);
   glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max);

   for (i = 0; i < n; i++) {
      GLint size, len;
      GLenum type;
      char name[100];

      glGetActiveAttrib(program, i, 100, &len, &size, &type, name);

      attribs[i].name = strdup(name);
      attribs[i].size = size;
      attribs[i].type = type;
      attribs[i].location = glGetAttribLocation(program, name);
   }

   attribs[i].name = NULL; /* end of list */

   return n;
}


void
PrintAttribs(const struct attrib_info attribs[])
{
   GLint i;

   printf("Attribs:\n");

   for (i = 0; attribs[i].name; i++) {
      printf("  %d: %s size=%d type=0x%x loc=%d\n",
             i,
             attribs[i].name,
             attribs[i].size,
             attribs[i].type,
             attribs[i].location);
   }
}
