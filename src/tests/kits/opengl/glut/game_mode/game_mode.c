
#include <stdio.h>
#include <GL/glut.h>

float spin = 0;

static void
draw_cube(void)
{
  glLoadIdentity();

  /* A step backward, then spin the cube */
  glTranslatef(0, 0, -5);
  glRotatef(spin, 0, 0, 1);
  glRotatef(spin, 1, 0.6, 0);

  /* We tell we want to draw quads */
  glBegin (GL_QUADS);

  /* Every four calls to glVertex, a quad is drawn */
  glColor3f(0, 0, 0); glVertex3f(-1, -1, -1);
  glColor3f(0, 0, 1); glVertex3f(-1, -1,  1);
  glColor3f(0, 1, 1); glVertex3f(-1,  1,  1);
  glColor3f(0, 1, 0); glVertex3f(-1,  1, -1);

  glColor3f(1, 0, 0); glVertex3f( 1, -1, -1);
  glColor3f(1, 0, 1); glVertex3f( 1, -1,  1);
  glColor3f(1, 1, 1); glVertex3f( 1,  1,  1);
  glColor3f(1, 1, 0); glVertex3f( 1,  1, -1);

  glColor3f(0, 0, 0); glVertex3f(-1, -1, -1);
  glColor3f(0, 0, 1); glVertex3f(-1, -1,  1);
  glColor3f(1, 0, 1); glVertex3f( 1, -1,  1);
  glColor3f(1, 0, 0); glVertex3f( 1, -1, -1);

  glColor3f(0, 1, 0); glVertex3f(-1,  1, -1);
  glColor3f(0, 1, 1); glVertex3f(-1,  1,  1);
  glColor3f(1, 1, 1); glVertex3f( 1,  1,  1);
  glColor3f(1, 1, 0); glVertex3f( 1,  1, -1);

  glColor3f(0, 0, 0); glVertex3f(-1, -1, -1);
  glColor3f(0, 1, 0); glVertex3f(-1,  1, -1);
  glColor3f(1, 1, 0); glVertex3f( 1,  1, -1);
  glColor3f(1, 0, 0); glVertex3f( 1, -1, -1);

  glColor3f(0, 0, 1); glVertex3f(-1, -1,  1);
  glColor3f(0, 1, 1); glVertex3f(-1,  1,  1);
  glColor3f(1, 1, 1); glVertex3f( 1,  1,  1);
  glColor3f(1, 0, 1); glVertex3f( 1, -1,  1);

  glEnd();
}


static void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  draw_cube();

  glutSwapBuffers();
}


static void
idle(void)
{
	spin += 2.0;
	if (spin > 360.0)
		spin = 0.0;

	glutPostRedisplay();
}


static void
reshape(int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	// Setup the view of the cube.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective( /* field of view in degree */ 40.0,
		/* aspect ratio */ 1.0,
		/* Z near */ 1.0, /* Z far */ 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0, 0.0, 5.0,  // eye is at (0,0,5)
		0.0, 0.0, 0.0,        // center is at (0,0,0)
		0.0, 1.0, 0.);        // up is in positive Y direction

	// Adjust cube position to be asthetic angle.
	glTranslatef(0.0, 0.0, -1.0);
	glRotatef(60, 1.0, 0.0, 0.0);
	glRotatef(-20, 0.0, 0.0, 1.0);
}


static void
init(void)
{
	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	glClearColor(0, 0, 0, 0);
}


int
main(int argc, char **argv)
{
	glutInit(&argc, argv);

  	if (argc > 1) {
  		printf("glutGameModeString(\"%s\"): ", argv[1]);

  		glutGameModeString(argv[1]);

  		if (!glutGameModeGet(GLUT_GAME_MODE_POSSIBLE))
  			printf("*not* possible: fallback to windowed mode.\n");
  		else {
 	 		printf("possible!\nglutEnterGameMode()\n");
  			glutEnterGameMode();

  			printf("glutGameModeGet(GLUT_GAME_MODE_DISPLAY_CHANGED) = %d\n",
  				glutGameModeGet(GLUT_GAME_MODE_DISPLAY_CHANGED));
  		}
	}

	if (!glutGameModeGet(GLUT_GAME_MODE_ACTIVE)) {
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
		glutCreateWindow(argv[0]);
	}

	init();


	glutMainLoop();

	if (glutGameModeGet(GLUT_GAME_MODE_ACTIVE)) {
 		printf("glutLeaveGameMode()\n");
		glutLeaveGameMode();
	}

	return 0;
}
