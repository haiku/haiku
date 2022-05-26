//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------

#include "AddOnImage.h"


// constructor
AddOnImage::AddOnImage()
	: fID(-1)
{
}


// destructor
AddOnImage::~AddOnImage()
{
	Unload();
}


// Load
status_t
AddOnImage::Load(const char* path)
{
	Unload();
	status_t error = (path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		image_id id = load_add_on(path);
		if (id >= 0)
			fID = id;
		else
			error = id;
	}
	return error;
}


// Unload
void
AddOnImage::Unload()
{
	if (fID >= 0) {
		unload_add_on(fID);
		fID = -1;
	}
}


// SetID
void
AddOnImage::SetID(image_id id)
{
	Unload();
	if (id >= 0)
		fID = id;
}
