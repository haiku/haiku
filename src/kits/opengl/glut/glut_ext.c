
/* Copyright (c) Mark J. Kilgard, 1994, 1997. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <string.h>

#include "glutint.h"

/* CENTRY */
int GLUTAPIENTRY 
glutExtensionSupported(const char *extension)
{
  static const GLubyte *extensions = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  /* Extension names should not have spaces. */
  where = (GLubyte *) strchr(extension, ' ');
  if (where || *extension == '\0')
    return 0;

  if (!extensions) {
    extensions = glGetString(GL_EXTENSIONS);
  }
  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string.  Don't be fooled by sub-strings,
     etc. */
  start = extensions;
  for (;;) {
    /* If your application crashes in the strstr routine below,
       you are probably calling glutExtensionSupported without
       having a current window.  Calling glGetString without
       a current OpenGL context has unpredictable results.
       Please fix your program. */
    where = (GLubyte *) strstr((const char *) start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ') {
      if (*terminator == ' ' || *terminator == '\0') {
        return 1;
      }
    }
    start = terminator;
  }
  return 0;
}


struct name_address_pair {
   const char *name;
   const GLUTproc address;
};

static struct name_address_pair glut_functions[] = {
   { "glutInit", (const GLUTproc) glutInit },
   { "glutInitDisplayMode", (const GLUTproc) glutInitDisplayMode },
   { "glutInitDisplayString", (const GLUTproc) glutInitDisplayString },
   { "glutInitWindowPosition", (const GLUTproc) glutInitWindowPosition },
   { "glutInitWindowSize", (const GLUTproc) glutInitWindowSize },
   { "glutMainLoop", (const GLUTproc) glutMainLoop },
   { "glutCreateWindow", (const GLUTproc) glutCreateWindow },
   { "glutCreateSubWindow", (const GLUTproc) glutCreateSubWindow },
   { "glutDestroyWindow", (const GLUTproc) glutDestroyWindow },
   { "glutPostRedisplay", (const GLUTproc) glutPostRedisplay },
   { "glutPostWindowRedisplay", (const GLUTproc) glutPostWindowRedisplay },
   { "glutSwapBuffers", (const GLUTproc) glutSwapBuffers },
   { "glutGetWindow", (const GLUTproc) glutGetWindow },
   { "glutSetWindow", (const GLUTproc) glutSetWindow },
   { "glutSetWindowTitle", (const GLUTproc) glutSetWindowTitle },
   { "glutSetIconTitle", (const GLUTproc) glutSetIconTitle },
   { "glutPositionWindow", (const GLUTproc) glutPositionWindow },
   { "glutReshapeWindow", (const GLUTproc) glutReshapeWindow },
   { "glutPopWindow", (const GLUTproc) glutPopWindow },
   { "glutPushWindow", (const GLUTproc) glutPushWindow },
   { "glutIconifyWindow", (const GLUTproc) glutIconifyWindow },
   { "glutShowWindow", (const GLUTproc) glutShowWindow },
   { "glutHideWindow", (const GLUTproc) glutHideWindow },
   { "glutFullScreen", (const GLUTproc) glutFullScreen },
   { "glutSetCursor", (const GLUTproc) glutSetCursor },
   { "glutWarpPointer", (const GLUTproc) glutWarpPointer },
   { "glutEstablishOverlay", (const GLUTproc) glutEstablishOverlay },
   { "glutRemoveOverlay", (const GLUTproc) glutRemoveOverlay },
   { "glutUseLayer", (const GLUTproc) glutUseLayer },
   { "glutPostOverlayRedisplay", (const GLUTproc) glutPostOverlayRedisplay },
   { "glutPostWindowOverlayRedisplay", (const GLUTproc) glutPostWindowOverlayRedisplay },
   { "glutShowOverlay", (const GLUTproc) glutShowOverlay },
   { "glutHideOverlay", (const GLUTproc) glutHideOverlay },
   { "glutCreateMenu", (const GLUTproc) glutCreateMenu },
   { "glutDestroyMenu", (const GLUTproc) glutDestroyMenu },
   { "glutGetMenu", (const GLUTproc) glutGetMenu },
   { "glutSetMenu", (const GLUTproc) glutSetMenu },
   { "glutAddMenuEntry", (const GLUTproc) glutAddMenuEntry },
   { "glutAddSubMenu", (const GLUTproc) glutAddSubMenu },
   { "glutChangeToMenuEntry", (const GLUTproc) glutChangeToMenuEntry },
   { "glutChangeToSubMenu", (const GLUTproc) glutChangeToSubMenu },
   { "glutRemoveMenuItem", (const GLUTproc) glutRemoveMenuItem },
   { "glutAttachMenu", (const GLUTproc) glutAttachMenu },
   { "glutDetachMenu", (const GLUTproc) glutDetachMenu },
   { "glutDisplayFunc", (const GLUTproc) glutDisplayFunc },
   { "glutReshapeFunc", (const GLUTproc) glutReshapeFunc },
   { "glutKeyboardFunc", (const GLUTproc) glutKeyboardFunc },
   { "glutMouseFunc", (const GLUTproc) glutMouseFunc },
   { "glutMotionFunc", (const GLUTproc) glutMotionFunc },
   { "glutPassiveMotionFunc", (const GLUTproc) glutPassiveMotionFunc },
   { "glutEntryFunc", (const GLUTproc) glutEntryFunc },
   { "glutVisibilityFunc", (const GLUTproc) glutVisibilityFunc },
   { "glutIdleFunc", (const GLUTproc) glutIdleFunc },
   { "glutTimerFunc", (const GLUTproc) glutTimerFunc },
   { "glutMenuStateFunc", (const GLUTproc) glutMenuStateFunc },
   { "glutSpecialFunc", (const GLUTproc) glutSpecialFunc },
   { "glutSpaceballMotionFunc", (const GLUTproc) glutSpaceballMotionFunc },
   { "glutSpaceballRotateFunc", (const GLUTproc) glutSpaceballRotateFunc },
   { "glutSpaceballButtonFunc", (const GLUTproc) glutSpaceballButtonFunc },
   { "glutButtonBoxFunc", (const GLUTproc) glutButtonBoxFunc },
   { "glutDialsFunc", (const GLUTproc) glutDialsFunc },
   { "glutTabletMotionFunc", (const GLUTproc) glutTabletMotionFunc },
   { "glutTabletButtonFunc", (const GLUTproc) glutTabletButtonFunc },
   { "glutMenuStatusFunc", (const GLUTproc) glutMenuStatusFunc },
   { "glutOverlayDisplayFunc", (const GLUTproc) glutOverlayDisplayFunc },
   { "glutWindowStatusFunc", (const GLUTproc) glutWindowStatusFunc },
//   { "glutKeyboardUpFunc", (const GLUTproc) glutKeyboardUpFunc },
//   { "glutSpecialUpFunc", (const GLUTproc) glutSpecialUpFunc },
//   { "glutJoystickFunc", (const GLUTproc) glutJoystickFunc },
   { "glutSetColor", (const GLUTproc) glutSetColor },
   { "glutGetColor", (const GLUTproc) glutGetColor },
   { "glutCopyColormap", (const GLUTproc) glutCopyColormap },
   { "glutGet", (const GLUTproc) glutGet },
   { "glutDeviceGet", (const GLUTproc) glutDeviceGet },
   { "glutExtensionSupported", (const GLUTproc) glutExtensionSupported },
   { "glutGetModifiers", (const GLUTproc) glutGetModifiers },
   { "glutLayerGet", (const GLUTproc) glutLayerGet },
   { "glutGetProcAddress", (const GLUTproc) glutGetProcAddress },
   { "glutBitmapCharacter", (const GLUTproc) glutBitmapCharacter },
   { "glutBitmapWidth", (const GLUTproc) glutBitmapWidth },
   { "glutStrokeCharacter", (const GLUTproc) glutStrokeCharacter },
   { "glutStrokeWidth", (const GLUTproc) glutStrokeWidth },
   { "glutBitmapLength", (const GLUTproc) glutBitmapLength },
   { "glutStrokeLength", (const GLUTproc) glutStrokeLength },
   { "glutWireSphere", (const GLUTproc) glutWireSphere },
   { "glutSolidSphere", (const GLUTproc) glutSolidSphere },
   { "glutWireCone", (const GLUTproc) glutWireCone },
   { "glutSolidCone", (const GLUTproc) glutSolidCone },
   { "glutWireCube", (const GLUTproc) glutWireCube },
   { "glutSolidCube", (const GLUTproc) glutSolidCube },
   { "glutWireTorus", (const GLUTproc) glutWireTorus },
   { "glutSolidTorus", (const GLUTproc) glutSolidTorus },
   { "glutWireDodecahedron", (const GLUTproc) glutWireDodecahedron },
   { "glutSolidDodecahedron", (const GLUTproc) glutSolidDodecahedron },
   { "glutWireTeapot", (const GLUTproc) glutWireTeapot },
   { "glutSolidTeapot", (const GLUTproc) glutSolidTeapot },
   { "glutWireOctahedron", (const GLUTproc) glutWireOctahedron },
   { "glutSolidOctahedron", (const GLUTproc) glutSolidOctahedron },
   { "glutWireTetrahedron", (const GLUTproc) glutWireTetrahedron },
   { "glutSolidTetrahedron", (const GLUTproc) glutSolidTetrahedron },
   { "glutWireIcosahedron", (const GLUTproc) glutWireIcosahedron },
   { "glutSolidIcosahedron", (const GLUTproc) glutSolidIcosahedron },
   { "glutVideoResizeGet", (const GLUTproc) glutVideoResizeGet },
   { "glutSetupVideoResizing", (const GLUTproc) glutSetupVideoResizing },
   { "glutStopVideoResizing", (const GLUTproc) glutStopVideoResizing },
   { "glutVideoResize", (const GLUTproc) glutVideoResize },
   { "glutVideoPan", (const GLUTproc) glutVideoPan },
   { "glutReportErrors", (const GLUTproc) glutReportErrors },
//   { "glutIgnoreKeyRepeat", (const GLUTproc) glutIgnoreKeyRepeat },
//   { "glutSetKeyRepeat", (const GLUTproc) glutSetKeyRepeat },
//   { "glutForceJoystickFunc", (const GLUTproc) glutForceJoystickFunc },
//   { "glutGameModeString", (const GLUTproc) glutGameModeString },
//   { "glutEnterGameMode", (const GLUTproc) glutEnterGameMode },
//   { "glutLeaveGameMode", (const GLUTproc) glutLeaveGameMode },
//   { "glutGameModeGet", (const GLUTproc) glutGameModeGet },
   { NULL, NULL }
};   


/* XXX This isn't an official GLUT function, yet */
GLUTproc GLUTAPIENTRY 
glutGetProcAddress(const char *procName)
{
   /* Try GLUT functions first */
   int i;
   for (i = 0; glut_functions[i].name; i++) {
      if (strcmp(glut_functions[i].name, procName) == 0)
         return glut_functions[i].address;
   }

   /* Try core GL functions */
#if defined(_WIN32)
  return (GLUTProc) wglGetProcAddress((LPCSTR) procName);
#elif defined(GLX_ARB_get_proc_address)
  return (GLUTProc) glXGetProcAddressARB((const GLubyte *) procName);
#else
  return NULL;
#endif
}


/* ENDCENTRY */
