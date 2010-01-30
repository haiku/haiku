/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Just drag'n'drop media files to the 3D objects
*/

#include <stdio.h>	
#include <assert.h>

#include <Bitmap.h>
#include <TranslationUtils.h>

#include "GLUtility.h"
#include "ViewSphere.h"

//	Local definitions

// 	Local functions

//	Local variables

/********************************
	Sphere scene
*********************************/
class Sphere
{
public:
			Sphere(const float radius, const int stacks = 10, const int slices = 10);
			~Sphere();
	void	Render();
	void	SetMediaSource(MediaSource *source) {fMediaSource = source;}
	void	SetAngle(float angle_y, float angle_z);
		
private:
	MediaSource		*fMediaSource;
	int				fNumberVertices;
	float			*fGeometry;
	float			*fTextureCoords;
	float			fRotationY, fRotationZ;
};

/*	FUNCTION:		Sphere :: Sphere
	ARGUMENTS:		radius
					stacks		vertical stacks
					slices		horizontal slices
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Sphere :: Sphere(const float radius, const int stacks, const int slices)
{
	fGeometry = 0;
	fTextureCoords = 0;	
	fMediaSource = 0;
	fRotationY = 0;
	fRotationZ = 0;
	
	fNumberVertices = 2*stacks*(slices+1);
	fGeometry = new float [fNumberVertices*3];
	fTextureCoords = new float [fNumberVertices*2];
	
	float *v = fGeometry;
	float *t = fTextureCoords;
	
	float z0, z1, r0, r1;
	float x, y, z;
	//YVector3 normal;	 // not used in 3Dmov, but useful to know 
	
	for (int i=0; i < stacks; i++)
	{
		z0 = (float)i/(float)stacks;
		z1 = (float)(i+1)/(float)stacks;
		r0 = radius * sind(180 * (float)i/(float)stacks);
		r1 = radius * sind(180 * (float)(i+1)/(float)stacks);

		for (int j=0; j < (slices + 1); j++)
		{
			x = sind(360.0f * (float)j/(float)slices);
			y = cosd(360.0f * (float)j/(float)slices);
			z = radius * cosd(180*z0);
			
			//	Vertices
			*v++ = x * r0;
			*v++ = -y * r0;
			*v++ = z;
			//	Normal not used in 3Dmov, but if you ever need it 
			//normal.Set(x*r0, -y*r0, z);
			//normal.Normalise();
			//	Textures
			*t++ = (float)j/(float)slices;
			*t++ = z0;
			
			z = radius * cosd(180*z1);
			//	Vertices
			*v++ = x * r1;
			*v++ = -y * r1;
			*v++ = z;
			//	Normals not used in 3Dmov, but if you ever need it
			//normal.Set(x*r1, -y*r1, z);
			//normal.Normalise();
			//	Textures
			*t++ = (float)j/(float)slices;
			*t++ = z1;
		}
	}
}

/*	FUNCTION:		Sphere :: ~Sphere
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor.  fMediaSource destroyed by ViewSphere
*/
Sphere :: ~Sphere()
{
	delete [] fGeometry;
	delete [] fTextureCoords;
}

/*	FUNCTION:		Sphere :: SetAngle
	ARGUMENTS:		angle_y
					angle_z
	RETURN:			n/a
	DESCRIPTION:	Rotate sphere by Euler angles
*/
void Sphere :: SetAngle(float angle_y, float angle_z)
{
	fRotationY = angle_y;
	fRotationZ = angle_z;
}

/*	FUNCTION:		Sphere :: Render
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Draw sphere
*/
void Sphere :: Render()
{
	glColor4f(1,1,1,1);
	glBindTexture(GL_TEXTURE_2D, fMediaSource->mTextureID);
	
	glPushMatrix();
	glRotatef(-30, 1, 0, 0);
	glRotatef(fRotationY, 0, 1, 0);
	glRotatef(fRotationZ, 0, 0, 1);
	glTexCoordPointer(2, GL_FLOAT, 0, fTextureCoords);
	glVertexPointer(3, GL_FLOAT, 0, fGeometry);	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, fNumberVertices);
	glPopMatrix();
}

/********************************
	ViewSphere
*********************************/

/*	FUNCTION:		ViewSphere :: ViewSphere
	ARGUMENTS:		frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ViewSphere :: ViewSphere(BRect frame)
	: ViewObject(frame)
{
	fStartTime = real_time_clock_usecs();
	fSphere = 0;
	fMediaSource = 0;
	fSpeedZ = 10.0f;
	fSpeedY = 0.0f;
	fAngleY = 0;
	fAngleZ = 0;
}

/*	FUNCTION:		ViewSphere :: ~ViewSphere
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ViewSphere :: ~ViewSphere()
{
	delete fSphere;
	if (fMediaSource != GetDefaultMediaSource())
		delete fMediaSource;
}

/*	FUNCTION:		ViewSphere :: AttachedToWindow
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when view attached to window (looper)
*/
void ViewSphere :: AttachedToWindow(void)
{
	ViewObject::AttachedToWindow();
	
	LockGL();
	glClearColor(0,0,0,1);
	
	fSphere = new Sphere(0.1f, 10, 10);
	fMediaSource = GetDefaultMediaSource();
	fSphere->SetMediaSource(fMediaSource);
	
	UnlockGL();
}	

/*	FUNCTION:		ViewSphere :: Render
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Draw view contents	
*/
void ViewSphere :: Render(void)
{
	LockGL();
	
	bigtime_t	current_time = real_time_clock_usecs();
	bigtime_t	delta = current_time - fStartTime;	
	fStartTime = current_time;
		
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	fAngleY += fSpeedY*(float)delta/1000000.0f;
	fAngleZ += fSpeedZ*(float)delta/1000000.0f;
	fSphere->SetAngle(fAngleY, fAngleZ);
	
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.15f);
	fSphere->Render();
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

/*	FUNCTION:		ViewSphere :: MouseDown
	ARGUMENTS:		p
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse down detected	
*/
void ViewSphere :: MouseDown(BPoint p)
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

/*	FUNCTION:		ViewSphere :: MouseMoved
	ARGUMENTS:		p
					transit
					message
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse move detected.
					This demo is succeptable to Gimbal lock, but it doesn't really matter.	
*/
void ViewSphere :: MouseMoved(BPoint p, uint32 transit, const BMessage *message)
{
	if (fMouseTracking)
	{
		if (transit == B_INSIDE_VIEW)
		{
			fSpeedY = 5*(p.y - fMousePosition.y);
			fSpeedZ = 5*(p.x - fMousePosition.x);
			fMousePosition = p;
		}
	}
}

/*	FUNCTION:		ViewSphere :: MouseUp
	ARGUMENTS:		p
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse up detected	
*/
void ViewSphere :: MouseUp(BPoint p)
{
	fMouseTracking = false;
}

/*	FUNCTION:		ViewSphere :: DragDropImage
	ARGUMENTS:		texture_id
					mouse_x
					mouse_y
	RETURN:			true if source ownership acquired
	DESCRIPTION:	Hook function called when user drags/drops image to app window	
*/
bool ViewSphere :: SurfaceUpdate(MediaSource *source, float mouse_x, float mouse_y)
{
	LockGL();
	
	if (fMediaSource != GetDefaultMediaSource())
		delete fMediaSource;
	fMediaSource = source;
	fSphere->SetMediaSource(source);
	
	UnlockGL();
	
	return true;

}




