
#include <stdio.h>
#include <string.h>

#include <GL/glut.h>


void display(void);
void idle(void);
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);

void draw_cube(void);
void game_mode(char *mode);
void dump_game_mode(void);
void init(void);
void on_exit(void);

float spin = 0;


void
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


void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  draw_cube();

  glutSwapBuffers();
}


void
idle(void)
{
	spin += 0.5;
	if (spin > 360.0)
		spin = 0.0;

	glutPostRedisplay();
}


void
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


void
keyboard(unsigned char key, int x, int y)
{
	switch(key) {
		case 27:
			exit(0);
			break;

		case 'g':
		case 'G':
			dump_game_mode();
			break;

		case 'e':
		case 'E': {
			char mode[255];

			printf("Game mode string? ");
			gets(mode);
			if (!strlen(mode))
				break;

			game_mode(mode);
			if (glutGameModeGet(GLUT_GAME_MODE_DISPLAY_CHANGED))
				init();
			break;
		}

		case 'l':
		case 'L': {
			// return to default window
			glutLeaveGameMode();
			if (!glutGetWindow())
				// exit if none
				exit(0);
			break;
		}
	}
}


void
init(void)
{
	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);

	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
}


void
on_exit(void)
{
	printf("Exit.\n");
}


void
game_mode(char *mode)
{
	glutGameModeString(mode);

	printf("glutGameModeString(\"%s\"): ", mode);
	if (!glutGameModeGet(GLUT_GAME_MODE_POSSIBLE)) {
		printf("*not* possible!\n");
		return;
	}

	printf("possible.\nglutEnterGameMode()\n");
	glutEnterGameMode();

	printf("glutGameModeGet(GLUT_GAME_MODE_DISPLAY_CHANGED) = %d\n",
			glutGameModeGet(GLUT_GAME_MODE_DISPLAY_CHANGED));
}


void
dump_game_mode()
{
	printf("glutGameModeGet():\n");

#	define DUMP(pname)	\
	printf("\t" #pname " = %d\n", glutGameModeGet(pname));

	DUMP(GLUT_GAME_MODE_ACTIVE);
	DUMP(GLUT_GAME_MODE_POSSIBLE);
	DUMP(GLUT_GAME_MODE_WIDTH);
	DUMP(GLUT_GAME_MODE_HEIGHT);
	DUMP(GLUT_GAME_MODE_PIXEL_DEPTH);
	DUMP(GLUT_GAME_MODE_REFRESH_RATE);
	DUMP(GLUT_GAME_MODE_DISPLAY_CHANGED);

#	undef DUMP

	printf("\n");
}

int
main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

  	if (argc > 1)
  		game_mode(argv[1]);

	if (!glutGameModeGet(GLUT_GAME_MODE_ACTIVE)) {
		printf("Using windowed mode.\n");
		glutCreateWindow(argv[0]);
	}

	init();

	atexit(on_exit);
	glutMainLoop();

	return 0;
}
