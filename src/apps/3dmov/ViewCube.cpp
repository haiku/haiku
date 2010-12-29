/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Just drag'n'drop media files to the 3D objects
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <Bitmap.h>
#include <TranslationUtils.h>

#include "GLUtility.h"
#include "ViewCube.h"

//	Local definitions

// 	Local functions

//	Local variables

/********************************
	Cube scene
*********************************/
class Cube
{
public:
	enum FACE
	{
		FACE_RIGHT,
		FACE_LEFT,
		FACE_FRONT,
		FACE_BACK,
		FACE_TOP,
		FACE_BOTTOM,
		NUMBER_CUBE_FACES,
	};
			Cube(const float half_extent);
			~Cube();
	void	Render();
	void	SetMediaSource(FACE face, MediaSource *source) {fMediaSources[face] = source;}
	void	SetAngle(float angle_x, float angle_y, float angle_z);
		
private:
	MediaSource		*fMediaSources[NUMBER_CUBE_FACES];
	float			fRotationX, fRotationY, fRotationZ;
	float			*fGeometry;
};

/*	FUNCTION:		Cube :: Cube
	ARGUMENTS:		half_extent
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Cube :: Cube(const float half_extent)
{
	for (int i=0; i < NUMBER_CUBE_FACES; i++)
		fMediaSources[i] = 0;
	fRotationX = fRotationY = fRotationZ = 0;
	
	const float kVertices[NUMBER_CUBE_FACES][4][3] = 
	{
		{	// FACE_RIGHT
			{half_extent, -half_extent, -half_extent},
			{half_extent, half_extent, -half_extent},
			{half_extent, -half_extent, half_extent},
			{half_extent, half_extent, half_extent},
		},
		{	// FACE_LEFT
			{-half_extent, half_extent, -half_extent},
			{-half_extent, -half_extent, -half_extent},
			{-half_extent, half_extent, half_extent},
			{-half_extent, -half_extent, half_extent},
		},
		{	// FACE_FRONT
			{-half_extent, -half_extent, -half_extent},
			{half_extent, -half_extent, -half_extent},
			{-half_extent, -half_extent, half_extent},
			{half_extent, -half_extent, half_extent},
		},
		{	// FACE_BACK
			{half_extent, half_extent, -half_extent},
			{-half_extent, half_extent, -half_extent},
			{half_extent, half_extent, half_extent},
			{-half_extent, half_extent, half_extent},
		},
		{	// FACE_TOP
			{-half_extent, -half_extent, half_extent},
			{half_extent, -half_extent, half_extent},
			{-half_extent, half_extent, half_extent},
			{half_extent, half_extent, half_extent},
		},
		{	// FACE_BOTTOM
			{-half_extent, half_extent, -half_extent},
			{half_extent, half_extent, -half_extent},
			{-half_extent, -half_extent, -half_extent},
			{half_extent, -half_extent, -half_extent},
		},
	};
	fGeometry = new float [sizeof(kVertices)/sizeof(float)];
	memcpy(fGeometry, kVertices, sizeof(kVertices));
}	

/*	FUNCTION:		Cube :: ~Cube
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor - fMediaSources owned by ViewCube
*/
Cube :: ~Cube()
{
	delete [] fGeometry;
}

/*	FUNCTION:		Cube :: SetAngle
	ARGUMENTS:		angle_x
					angle_y
					angle_z
	RETURN:			n/a
	DESCRIPTION:	Rotate cube by Euler angles
*/
void Cube :: SetAngle(float angle_x, float angle_y, float angle_z)
{
	fRotationX = angle_x;
	fRotationY = angle_y;
	fRotationZ = angle_z;
}

/*	FUNCTION:		Cube :: Render
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Draw sphere
*/
void Cube :: Render()
{
	const float kTextureCoords[4*2] = 
	{
		0.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
	};
	
	glPushMatrix();
	glRotatef(fRotationX, 1, 0, 0);
	glRotatef(fRotationY, 0, 1, 0);
	glRotatef(fRotationZ, 0, 0, 1);
	
	glColor4f(1,1,1,1);
	for (int i=0; i < NUMBER_CUBE_FACES; i++)
	{
		glBindTexture(GL_TEXTURE_2D, fMediaSources[i]->mTextureID);
		glTexCoordPointer(2, GL_FLOAT, 0, kTextureCoords);
		glVertexPointer(3, GL_FLOAT, 0, fGeometry + i*3*4);	
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	glPopMatrix();
}

/********************************
	ViewCube
*********************************/

/*	FUNCTION:		ViewCube :: ViewCube
	ARGUMENTS:		frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ViewCube :: ViewCube(BRect frame)
	: ViewObject(frame)
{
	fStartTime = real_time_clock_usecs();
	
	for (int i=0; i < NUMBER_FACES; i++)
		fMediaSources[i] = 0;
	fCube = 0;
	fSpeed = 10;
	fMouseTracking = false;
}

/*	FUNCTION:		ViewCube :: ~ViewCube
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ViewCube :: ~ViewCube()
{
	delete fCube;
	for (int i=0; i < NUMBER_FACES; i++)
	{
		if (fMediaSources[i] != GetDefaultMediaSource())
			delete fMediaSources[i];
	}
}

/*	FUNCTION:		ViewCube :: ViewCube
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when view attached to window (looper)
*/
void ViewCube :: AttachedToWindow(void)
{
	ViewObject::AttachedToWindow();
	
	LockGL();
	glClearColor(0,0,0,1);
	
	fCube = new Cube(0.075f);
	
	for (int i=0; i < NUMBER_FACES; i++)
	{
		fMediaSources[i] = GetDefaultMediaSource();
		fCube->SetMediaSource((Cube::FACE) i, fMediaSources[i]);
	}
	
	UnlockGL();
}	

/*	FUNCTION:		ViewCube :: Render
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Draw view contents	
*/
void ViewCube :: Render(void)
{
	LockGL();
	
	bigtime_t	current_time = real_time_clock_usecs();
	bigtime_t	delta = current_time - fStartTime;
	fStartTime = current_time;		
		
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	fCubeAngle += fSpeed*(float)delta/1000000.0f;
	fCube->SetAngle(0, 0, fCubeAngle);
	
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.15f);
	fCube->Render();
	glPopMatrix();
		
	glFlush();
	SwapBuffers();
	
	//	Display frame rate
	/*
	static int fps = 0;
	static int time_delta = 0;
	fps++;
	time_delta += delta;
	if (time_delta > 1000000)
	{
		printf("%d fps\n", fps);
		fps = 0;
		time_delta = 0;
	}
	*/
	UnlockGL();
}

/*	FUNCTION:		ViewCube :: MouseDown
	ARGUMENTS:		p
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse down detected	
*/
void ViewCube :: MouseDown(BPoint p)
{
	//	Determine mouse button
	BMessage* msg = Window()->CurrentMessage();
	uint32 buttons;
	
	msg->FindInt32("buttons", (int32*)&buttons);
	
	if (buttons & B_PRIMARY_MOUSE_BUTTON)
	{
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		fMouseTracking = true;
		fMousePosition = p;
	}
}

/*	FUNCTION:		ViewCube :: MouseMoved
	ARGUMENTS:		p
					transit
					message
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse move detected	
*/
void ViewCube :: MouseMoved(BPoint p, uint32 transit, const BMessage *message)
{
	if (fMouseTracking)
	{
		if (transit == B_INSIDE_VIEW)
		{
			fSpeed = 5.0f*(p.x - fMousePosition.x);
			fMousePosition = p;
		}
	}
		
}

/*	FUNCTION:		ViewSphere :: MouseUp
	ARGUMENTS:		p
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse up detected	
*/
void ViewCube :: MouseUp(BPoint p)
{
	fMouseTracking = false;
}

/*	FUNCTION:		ViewSphere :: DragDropImage
	ARGUMENTS:		texture_id
					mouse_x
					mouse_y
	RETURN:			true if source ownership acquired
	DESCRIPTION:	Hook function called when user drags/drops image to app window.
					TODO - actually determine exact face where refs received.  For this
					release, we only rely on current fCubeAngle.	
*/
bool ViewCube :: SurfaceUpdate(MediaSource *source, float mouse_x, float mouse_y)
{
	// determine face
	BRect frame = Bounds();
	Cube::FACE face = Cube::NUMBER_CUBE_FACES;
	
	if (mouse_y < frame.Height()*0.33f)
		face = Cube::FACE_TOP;
	else
	{
		if (fCubeAngle < 45)
			face = Cube::FACE_FRONT;
		else if (fCubeAngle < 135)
			face = Cube::FACE_LEFT;
		else if (fCubeAngle < 225)
			face = Cube::FACE_BACK;
		else if (fCubeAngle < 315)
			face = Cube::FACE_RIGHT;
		else
			face = Cube::FACE_FRONT;
	}
	if (fMediaSources[face] != GetDefaultMediaSource())
		delete fMediaSources[face];
	fMediaSources[face] = source;
	fCube->SetMediaSource(face, source);
	
	return true;
}




