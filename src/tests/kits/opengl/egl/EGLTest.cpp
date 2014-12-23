/*
 * Copyright (C) 2014 Adrián Arroyo Calle
 * Released under the terms of the MIT license.
 *
 * Authors:
 *   Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 *
 */

// Haiku EGL Test
//
// g++ -o HaikuTest HaikuTest.cpp /boot/home/mesa/build/haiku-x86-debug/egl/main/libEGL.so \
//   -lGL -lbe -I/boot/home/mesa/include && EGL_LOG_LEVEL=debug MESA_DEBUG=1 \
//   LIBGL_DRIVERS_PATH=/boot/home/mesa/build/haiku-x86-debug/mesa/drivers/haiku/swrast/ \
//   ./HaikuTest

#include <InterfaceKit.h>
#include <OpenGLKit.h>
#include <EGL/egl.h>
#include <stdio.h>


class EGLApp : public BApplication{
	public:
		EGLApp() : BApplication("application/x-egl-test"){};

		void
		ReadyToRun()
		{
			EGLContext ctx;
			EGLSurface surf;
			EGLConfig* configs;
			EGLint numConfigs;

			int maj;
			int min;
			
			printf("Starting EGL test...\n");
			
			printf("getDisplay...\n");
			EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);

			printf("eglInitialize...\n");
			eglInitialize(d, &maj, &min);

			printf("eglGetConfigs...\n");
			eglGetConfigs(d, configs, numConfigs, &numConfigs);

			printf("eglBindAPI...\n");
			eglBindAPI(EGL_OPENGL_API);

			printf("eglCreateContext...\n");
			ctx = eglCreateContext(d, configs[0], EGL_NO_CONTEXT, NULL);

			printf("new BWindow...\n");
			BWindow* win=new BWindow(BRect(100,100,500,500),"EGL App",B_TITLED_WINDOW,0);

			printf("eglCreateWindowSurface...\n");
			surf = eglCreateWindowSurface ( d, configs[0],
				(EGLNativeWindowType)win, NULL );

			printf("eglMakeCurrent...\n");
			eglMakeCurrent( d, surf, surf, ctx );

			printf("glClearColor + glClear + eglSwapBuffers...\n");
			float green=0.0f;
			while(1)
			{
				//sleep(1);
				glClearColor(1.0f,green,0.0f,1.0f);
				glClear ( GL_COLOR_BUFFER_BIT );
				eglSwapBuffers ( d, surf );
				green += 0.0001;
			}
		}
};


int
main()
{
	EGLApp* app=new EGLApp();
	
	app->Run();
	
	return 0;
}
