#ifndef SHADER_UTIL_H
#define SHADER_UTIL_H



struct uniform_info
{
   const char *name;
   GLuint size;  /**< number of value[] elements: 1, 2, 3 or 4 */
   GLenum type;  /**< GL_FLOAT, GL_FLOAT_VEC4, GL_INT, etc */
   GLfloat value[4];
   GLint location;  /**< filled in by InitUniforms() */
};

#define END_OF_UNIFORMS   { NULL, 0, GL_NONE, { 0, 0, 0, 0 }, -1 }


struct attrib_info
{
   const char *name;
   GLuint size;  /**< number of value[] elements: 1, 2, 3 or 4 */
   GLenum type;  /**< GL_FLOAT, GL_FLOAT_VEC4, GL_INT, etc */
   GLint location;
};


extern GLboolean
ShadersSupported(void);

extern GLuint
CompileShaderText(GLenum shaderType, const char *text);

extern GLuint
CompileShaderFile(GLenum shaderType, const char *filename);

extern GLuint
LinkShaders(GLuint vertShader, GLuint fragShader);

extern GLboolean
ValidateShaderProgram(GLuint program);

extern GLdouble
GetShaderCompileTime(void);

extern GLdouble
GetShaderLinkTime(void);

extern void
SetUniformValues(GLuint program, struct uniform_info uniforms[]);

extern GLuint
GetUniforms(GLuint program, struct uniform_info uniforms[]);

extern void
PrintUniforms(const struct uniform_info uniforms[]);

extern GLuint
GetAttribs(GLuint program, struct attrib_info attribs[]);

extern void
PrintAttribs(const struct attrib_info attribs[]);

#endif /* SHADER_UTIL_H */
