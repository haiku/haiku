/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					The book has 4 pages with images, and the user can move pages 2/3.
					Texture management is really primitive - only the default image is
					shared, while the other page images are unique instances.
*/

#include <stdio.h>
#include <assert.h>

#include <Bitmap.h>

#include "GLUtility.h"
#include "ViewBook.h"

//	Local definitions

// 	Local functions
//	void GetPage(Paper *

//	Local variables

/********************************
	Paper scene
*********************************/
class Paper
{
public:
	enum ORIENTATION
	{
		FRONT_FACING,
		BACK_FACING,
		FRONT_AND_BACK_FACING,
	};
	enum TEXTURE_SIDE
	{
		FRONT_TEXTURE,
		BACK_TEXTURE,
		UNASSIGNED_TEXTURE,
	};
				Paper(float x_size, float y_size, ORIENTATION orientation, const int number_columns = 6, const int number_rows = 4);
				~Paper();

	void		Render();
	void		SetMediaSource(TEXTURE_SIDE side, MediaSource *source);
	void		SetAngle(float angle);

private:
	ORIENTATION		fOrientation;
	MediaSource		*fFrontMediaSource;
	MediaSource		*fBackMediaSource;
	float			*fGeometry;
	float			*fTextureCoords;

	int				fNumberVertices;
	float			fWidth, fHeight;
	int				fNumberColumns, fNumberRows;

	void			InitTextureCoordinates();
	void			ModifyGeometry();

	float			fAngle;
};

/*	FUNCTION:		Paper :: Paper
	ARGUMENTS:		x_size, y_size
					number_columns
					number_rows;
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Paper :: Paper(float x_size, float y_size, ORIENTATION orientation, const int number_columns, const int number_rows)
{
	assert(number_rows%2 == 0);	// rows must be divisible by 2

	fOrientation = orientation;
	fFrontMediaSource = 0;
	fBackMediaSource = 0;
	fWidth = x_size;
	fHeight = y_size;
	fNumberColumns = number_columns;
	fNumberRows = number_rows;

	fNumberVertices = (fNumberColumns*2+1)*fNumberRows + 1;
	fGeometry = new float [fNumberVertices*3];
	fTextureCoords = new float [fNumberVertices * 2];
	InitTextureCoordinates();

	SetAngle(0);
}

/*	FUNCTION:		Paper :: ~Paper
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor.  MediaSource destruction handled by ViewBook
*/
Paper :: ~Paper()
{
	delete [] fGeometry;
	delete [] fTextureCoords;
}

/*	FUNCTION:		Paper :: SetMediaSource
	ARGUMENTS:		side
					source
	RETURN:			n/a
	DESCRIPTION:	Assign paper texture id
*/
void Paper :: SetMediaSource(TEXTURE_SIDE side, MediaSource *source)
{
	if (side == FRONT_TEXTURE)
		fFrontMediaSource = source;
	else
		fBackMediaSource = source;
}

/*	FUNCTION:		Paper :: SetAngle
	ARGUMENTS:		angle		0 - paper is on left side of book
								90 - paper is vertical
								180 - paper is on right side of book
	RETURN:			n/a
	DESCRIPTION:	Set paper angle and recalculate geometry
*/
void Paper :: SetAngle(float angle)
{
	fAngle = angle;
	ModifyGeometry();
}

/*	FUNCTION:		Paper :: Render
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Draw paper
*/
void Paper :: Render()
{
	glPushMatrix();
	glTranslatef(-0.5f*fWidth, -0.5f*fHeight, 0);

	if ((fOrientation == FRONT_FACING) || (fOrientation == FRONT_AND_BACK_FACING))
	{
		glColor4f(1,1,1,1);
		glBindTexture(GL_TEXTURE_2D, fFrontMediaSource->mTextureID);

		glTexCoordPointer(2, GL_FLOAT, 0, fTextureCoords);
		glVertexPointer(3, GL_FLOAT, 0, fGeometry);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, fNumberVertices);
	}

	if ((fOrientation == BACK_FACING) || (fOrientation == FRONT_AND_BACK_FACING))
	{
		glColor4f(1,1,1,1);
		glBindTexture(GL_TEXTURE_2D, fBackMediaSource->mTextureID);

		//	Mirror texture coordinates
		glMatrixMode(GL_TEXTURE);
		glScalef(-1,1,1);

		glFrontFace(GL_CW);
		glTexCoordPointer(2, GL_FLOAT, 0, fTextureCoords);
		glVertexPointer(3, GL_FLOAT, 0, fGeometry);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, fNumberVertices);
		glFrontFace(GL_CCW);

		//	Restore texture mirroring
		glScalef(-1,1,1);
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}

/*	FUNCTION:		Paper :: InitTextureCoordinates
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Set texture coordinates (they never change)
*/
void Paper :: InitTextureCoordinates()
{
	float *p = fTextureCoords;
	for (int r = fNumberRows-2; r >= 0; r-=2)
	{
		if (r == fNumberRows-2)
		{
			*p++ = 0;
			*p++ = 0;
		}
		for (int c=0; c < fNumberColumns; c++)
		{
			*p++ = (float)c/fNumberColumns;
			*p++ = 1 - (float)(r+1)/fNumberRows;

			*p++ = (float)(c+1)/fNumberColumns;
			*p++ = 1 - (float)(r+2)/fNumberRows;
		}
		for (int c = fNumberColumns; c >= 0; c--)
		{
			*p++ = (float)c/fNumberColumns;
			*p++ = 1 - (float)(r+1)/fNumberRows;

			*p++ = (float)c/fNumberColumns;
			*p++ = 1 - (float)r/fNumberRows;
		}
	}
}

/*	FUNCTION:		Paper :: ModifyGeometry
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Modify vertices only
*/
void Paper :: ModifyGeometry()
{
	float *p = fGeometry;
	const float kPageConcaveHeight = 0.1f*fWidth;

	//	Cache frequent calculations
	float ca = cosd(fAngle);
	float w = fWidth * sind(fAngle);

	for (int r = fNumberRows-2; r >= 0; r-=2)
	{
		if (r == fNumberRows-2)
		{
			*p++ = (1-ca)*fWidth;
			*p++ = fHeight;
			*p++ = w;
		}
		for (int c=0; c < fNumberColumns; c++)
		{
			*p++ = (1-ca)*fWidth + ca*(float)c/fNumberColumns * fWidth;
			*p++ = (float)(r+1)/fNumberRows * fHeight;
			*p++ = (1-(float)c/fNumberColumns) * w + kPageConcaveHeight*sind(180*(float)c/fNumberColumns);

			*p++ = (1-ca)*fWidth + ca*(float)(c+1)/fNumberColumns * fWidth;
			*p++ = (float)(r+2)/fNumberRows * fHeight;
			*p++ = (1-(float)(c+1)/fNumberColumns) * w + + kPageConcaveHeight*sind(180*(float)(c+1)/fNumberColumns);
		}
		for (int c = fNumberColumns; c >= 0; c--)
		{
			*p++ = (1-ca)*fWidth + ca*(float)c/fNumberColumns * fWidth;
			*p++ = (float)(r+1)/fNumberRows * fHeight;
			*p++ = (1-(float)c/fNumberColumns) * w + kPageConcaveHeight*sind(180*(float)c/fNumberColumns);

			*p++ = (1-ca)*fWidth + ca*(float)c/fNumberColumns * fWidth;
			*p++ = (float)r/fNumberRows * fHeight;
			*p++ = (1-(float)c/fNumberColumns) * w + kPageConcaveHeight*sind(180*(float)c/fNumberColumns);
		}
	}
}

/********************************
	ViewBook
*********************************/

static const float kPageWidth = 0.210f;		// A4 book
static const float kPageHeight = 0.297f;
static const float kPageMinAngle = 4.0f;
static const float kPageMaxAngle = 176.0f;

/*	FUNCTION:		ViewBook :: ViewBook
	ARGUMENTS:		frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
ViewBook :: ViewBook(BRect frame)
	: ViewObject(frame)
{
	fStartTime = real_time_clock_usecs();
	for (int i=0; i < NUMBER_SOURCES; i++)
		fMediaSources[i] = 0;
	for (int i=0; i < NUMBER_PAGES; i++)
		fPages[i] = 0;

	fMouseTracking = false;
	fPageAngle = kPageMaxAngle;
}

/*	FUNCTION:		ViewBook :: ~ViewBook
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
ViewBook :: ~ViewBook()
{
	for (int i=0; i < NUMBER_SOURCES; i++)
	{
		if (fMediaSources[i] != GetDefaultMediaSource())
			delete fMediaSources[i];
	}

	for (int i=0; i < NUMBER_PAGES; i++)
		delete fPages[i];
}

/*	FUNCTION:		ViewBook :: AttachedToWindow
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Hook function called when view attached to window (looper)
*/
void ViewBook :: AttachedToWindow(void)
{
	ViewObject::AttachedToWindow();

	LockGL();
	glClearColor(0,0,0,1);

	fPages[PAGE_MIDDLE] = new Paper(kPageWidth, kPageHeight, Paper::FRONT_AND_BACK_FACING);
	fPages[PAGE_LEFT] = new Paper(kPageWidth, kPageHeight, Paper::FRONT_FACING);
	fPages[PAGE_RIGHT] = new Paper(kPageWidth, kPageHeight, Paper::BACK_FACING);

	for (int i=0; i < NUMBER_SOURCES; i++)
		fMediaSources[i] = GetDefaultMediaSource();

	//	left page
	fPages[PAGE_LEFT]->SetMediaSource(Paper::FRONT_TEXTURE, fMediaSources[0]);
	//	middle page
	fPages[PAGE_MIDDLE]->SetMediaSource(Paper::FRONT_TEXTURE, fMediaSources[1]);
	fPages[PAGE_MIDDLE]->SetMediaSource(Paper::BACK_TEXTURE, fMediaSources[2]);
	//	right page
	fPages[PAGE_RIGHT]->SetMediaSource(Paper::BACK_TEXTURE, fMediaSources[3]);
	fPages[PAGE_RIGHT]->SetAngle(180);

	UnlockGL();
}

/*	FUNCTION:		ViewBook :: Render
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Draw view contents
*/
void ViewBook :: Render(void)
{
	LockGL();

	bigtime_t	current_time = real_time_clock_usecs();
	bigtime_t	delta = current_time - fStartTime;
	fStartTime = current_time;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//	page freefall (gravity)
	if (!fMouseTracking && (fPageAngle > kPageMinAngle) && (fPageAngle < kPageMaxAngle))
	{
		if (fPageAngle < 90)
			fPageAngle -= 30*(float)delta/1000000.0f;
		else
			fPageAngle += 30*(float)delta/1000000.0f;
	}
	fPages[PAGE_MIDDLE]->SetAngle(fPageAngle);

	//	Draw pages
	glPushMatrix();
	glTranslatef(-0.5f*kPageWidth, 0.5f*kPageHeight, 0);
	fPages[PAGE_MIDDLE]->Render();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-0.5f*kPageWidth, 0.5f*kPageHeight, 0);
	fPages[PAGE_LEFT]->Render();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-0.5f*kPageWidth, 0.5f*kPageHeight, 0);
	fPages[PAGE_RIGHT]->Render();
	glPopMatrix();


	glFlush();
	SwapBuffers();

	//	Frame rate
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

/*	FUNCTION:		ViewBook :: MouseDown
	ARGUMENTS:		p
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse down detected
*/
void ViewBook :: MouseDown(BPoint p)
{
	//	Determine mouse button
	BMessage* msg = Window()->CurrentMessage();
	uint32 buttons;

	msg->FindInt32("buttons", (int32*)&buttons);

	if (buttons & B_PRIMARY_MOUSE_BUTTON)
	{
		Paper::TEXTURE_SIDE side = Paper::UNASSIGNED_TEXTURE;
		Paper *page = GetPage(p.x, p.y, (int *)&side);
		if (page == fPages[PAGE_MIDDLE])
		{
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
			fMouseTracking = true;
		}
	}
}

/*	FUNCTION:		ViewBook :: MouseMoved
	ARGUMENTS:		p
					transit
					message
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse move detected
*/
void ViewBook :: MouseMoved(BPoint p, uint32 transit, const BMessage *message)
{
	if (fMouseTracking)
	{
		if (transit == B_INSIDE_VIEW)
		{
			BRect frame = Bounds();
			fPageAngle = 180 * p.x / frame.Width();
			if (fPageAngle < kPageMinAngle)
				fPageAngle = kPageMinAngle;
			if (fPageAngle > kPageMaxAngle)
				fPageAngle = kPageMaxAngle;
		}
	}
}

/*	FUNCTION:		ViewBook :: MouseUp
	ARGUMENTS:		p
	RETURN:			n/a
	DESCRIPTION:	Hook function called when mouse up detected
*/
void ViewBook :: MouseUp(BPoint p)
{
	fMouseTracking = false;
}

/*	FUNCTION:		ViewBook :: GetPage
	ARGUMENTS:		mouse_x
					mouse_y
	RETURN:			page corresponding to mouse_x, mouse_y
	DESCRIPTION:	Determine which page does mouse_x/mouse_y correspond to
*/
Paper * ViewBook :: GetPage(float mouse_x, float mouse_y, int *side)
{
	BRect frame = Bounds();
	Paper *page = 0;

	if (fPageAngle > 90)
	{
		if (mouse_x < 0.5f*frame.Width())
		{
			page = fPages[PAGE_LEFT];
			*side = Paper::FRONT_TEXTURE;
		}
		else
		{
			page = fPages[PAGE_MIDDLE];
			*side = Paper::BACK_TEXTURE;
		}
	}
	else
	{
		if (mouse_x < 0.5f*frame.Width())
		{
			page = fPages[PAGE_MIDDLE];
			*side = Paper::FRONT_TEXTURE;
		}
		else
		{
			page = fPages[PAGE_RIGHT];
			*side = Paper::BACK_TEXTURE;
		}
	}
	return page;
}

/*	FUNCTION:		ViewBook :: DragDropImage
	ARGUMENTS:		texture_id
					mouse_x
					mouse_y
	RETURN:			true if source ownership acquired
	DESCRIPTION:	Hook function called when user drags/drops image to app window
*/
bool ViewBook :: SurfaceUpdate(MediaSource *source, float mouse_x, float mouse_y)
{
	Paper::TEXTURE_SIDE side = Paper::UNASSIGNED_TEXTURE;
	Paper *page = GetPage(mouse_x, mouse_y, (int *)&side);

	LockGL();
	int index = -1;
	if (page == fPages[PAGE_LEFT])
		index = 0;
	else if (page == fPages[PAGE_RIGHT])
		index = 3;
	else if (page == fPages[PAGE_MIDDLE])
	{
		if (side == Paper::FRONT_TEXTURE)
			index = 1;
		else if (side == Paper::BACK_TEXTURE)
			index = 2;
	}
	if (index >= 0)
	{
		if (fMediaSources[index] != GetDefaultMediaSource())
			delete fMediaSources[index];
	}
	page->SetMediaSource(side, source);
	fMediaSources[index] = source;
	UnlockGL();

	return true;
}


