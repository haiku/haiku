//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------

#ifndef _ADD_ON_IMAGE_H
#define _ADD_ON_IMAGE_H

#include <image.h>

namespace BPrivate {

class AddOnImage {
public:
	AddOnImage();
	~AddOnImage();

	status_t Load(const char* path);
	void Unload();

	void SetID(image_id id);
	image_id ID() const	{ return fID; }

private:
	image_id	fID;
};

}	// namespace BPrivate

using BPrivate::AddOnImage;

#endif	// _ADD_ON_IMAGE_H
