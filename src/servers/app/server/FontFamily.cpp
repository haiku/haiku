//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		FontFamily.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	classes to represent font styles and families
//  
//------------------------------------------------------------------------------
#include "FontFamily.h"
#include "ServerFont.h"
#include FT_CACHE_H

FTC_Manager ftmanager;

/*!
	\brief Constructor
	\param filepath path to a font file
	\param face FreeType handle for the font file after it is loaded - for its info only
*/
FontStyle::FontStyle(const char *filepath, FT_Face face)
{
	name=new BString(face->style_name);
	cachedface=new CachedFaceRec;
	cachedface->file_path=filepath;
	family=NULL;
	instances=new BList(0);
	has_bitmaps=(face->num_fixed_sizes>0)?true:false;
	is_fixedwidth=(face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)?true:false;
	is_scalable=(face->face_flags & FT_FACE_FLAG_SCALABLE)?true:false;
	has_kerning=(face->face_flags & FT_FACE_FLAG_KERNING)?true:false;
	glyphcount=face->num_glyphs;
	charmapcount=face->num_charmaps;
	tunedcount=face->num_fixed_sizes;
	path=new BString(filepath);
	fbounds.Set(0,0,0,0);
}

/*!
	\brief Destructor
	
	Frees all data allocated on the heap. All child ServerFonts are marked as having
	a NULL style so that each ServerFont knows that it is running on borrowed time. 
	This is done because a FontStyle should be deleted only when it no longer has any
	dependencies.
*/
FontStyle::~FontStyle(void)
{
	delete name;
	delete path;
	delete cachedface;
	
	// Mark all instances as Free here
	int32 index=0;

	ServerFont *fs=(ServerFont*)instances->ItemAt(index);

	while(fs)
	{
		fs->fstyle=NULL;
		index++;
		fs=(ServerFont*)instances->ItemAt(index);
	}

	instances->MakeEmpty();
	delete instances;
}

/*!
	\brief Returns the name of the style as a string
	\return The style's name
*/
const char *FontStyle::Name(void)
{
	return name->String();
}

/*!
	\brief Returns a handle to the style in question, straight from FreeType's cache
	\return FreeType face handle or NULL if there was an internal error
*/

// TODO: Re-enable when I understand how the FT2 Cache system changed from
// 2.1.4 to 2.1.8
/*
FT_Face FontStyle::GetFace(void)
{
	FT_Face f;
	return (FTC_Manager_Lookup_Face(ftmanager,cachedface,&f)!=0)?f:NULL;
}
*/

/*!
	\brief Returns the path to the style's font file 
	\return The style's font file path
*/
const char *FontStyle::GetPath(void)
{
	return path->String();
}

/*!
	\brief Converts an ASCII character to Unicode for the style
	\param c An ASCII character
	\return A Unicode value for the character
*/

// TODO: Re-enable when I understand how the FT2 Cache system changed from
// 2.1.4 to 2.1.8
/*
int16 FontStyle::ConvertToUnicode(uint16 c)
{
	FT_Face f;
	if(FTC_Manager_Lookup_Face(ftmanager,cachedface,&f)!=0)
		return 0;
	
	return FT_Get_Char_Index(f,c);
}
*/

/*!
	\brief Creates a new ServerFont object for the style, given size, shear, and rotation.
	\param size character size in points
	\param rotation rotation in degrees
	\param shear shear (slant) in degrees. 45 <= shear <= 135. 90 is vertical
	\return The new ServerFont object
*/
ServerFont *FontStyle::Instantiate(float size, float rotation, float shear)
{
	ServerFont *f=new ServerFont(this, size, rotation, shear);
	instances->AddItem(f);
	return f;
}

/*!
	\brief Constructor
	\param namestr Name of the family
*/
FontFamily::FontFamily(const char *namestr)
{
	name=new BString(namestr);
	styles=new BList(0);
}

/*!
	\brief Destructor
	
	Deletes all child styles. Note that a FontFamily should not be deleted unless
	its styles have no dependencies or some other really good reason, such as 
	system shutdown.
*/
FontFamily::~FontFamily(void)
{
	delete name;
	
	BString *string;
	for(int32 i=0; i<styles->CountItems(); i++)
	{
		string=(BString *)styles->RemoveItem(i);
		if(string)
			delete string;
	}
	styles->MakeEmpty();	// should be empty, but just in case...
	delete styles;
}

/*!
	\brief Returns the name of the family
	\return The family's name
*/
const char *FontFamily::Name(void)
{
	return name->String();
}

/*!
	\brief Adds the style to the family
	\param path full path to the style's font file
	\param face FreeType face handle used to obtain info about the font
*/
void FontFamily::AddStyle(const char *path,FT_Face face)
{
	if(!path)
		return;

	BString style(face->style_name);
	FontStyle *item;

	// Don't add if it already is in the family.	
	int32 count=styles->CountItems();
	for(int32 i=0; i<count; i++)
	{
		item=(FontStyle *)styles->ItemAt(i);
		if(item->name->Compare(face->style_name)==0)
			return;
	}

	item=new FontStyle(path, face);
	item->family=this;
	styles->AddItem(item);
	AddDependent();
}

/*!
	\brief Removes a style from the family and deletes it
	\param style Name of the style to be removed from the family
*/
void FontFamily::RemoveStyle(const char *style)
{
	int32 count=styles->CountItems();
	if(!style || count<1)
		return;

	FontStyle *fs;
	for(int32 i=0; i<count; i++)
	{
		fs=(FontStyle *)styles->ItemAt(i);
		if(fs && fs->name->Compare(style)==0)
		{
			fs=(FontStyle *)styles->RemoveItem(i);
			if(fs)
			{
				delete fs;
				RemoveDependent();
			}
		}
	}
}

/*!
	\brief Returns the number of styles in the family
	\return The number of styles in the family
*/
int32 FontFamily::CountStyles(void)
{
	return styles->CountItems();
}

/*!
	\brief Determines whether the style belongs to the family
	\param style Name of the style being checked
	\return True if it belongs, false if not
*/
bool FontFamily::HasStyle(const char *style)
{
	int32 count=styles->CountItems();
	if(!style || count<1)
		return false;
	FontStyle *fs;
	for(int32 i=0; i<count; i++)
	{
		fs=(FontStyle *)styles->ItemAt(i);
		if(fs && fs->name->Compare(style)==0)
			return true;
	}
	return false;
}

/*! 
	\brief Returns the name of a style in the family
	\param index list index of the style to be found
	\return name of the style or NULL if the index is not valid
*/
const char *FontFamily::GetStyle(int32 index)
{
	FontStyle *fs=(FontStyle*)styles->ItemAt(index);
	if(!fs)
		return NULL;
	return fs->Name();
}

/*!
	\brief Get the FontStyle object for the name given
	\param style Name of the style to be obtained
	\return The FontStyle object or NULL if none was found.
	
	The object returned belongs to the family and must not be deleted.
*/
FontStyle *FontFamily::GetStyle(const char *style)
{
	int32 count=styles->CountItems();
	if(!style || count<1)
		return NULL;
	FontStyle *fs;
	for(int32 i=0; i<count; i++)
	{
		fs=(FontStyle *)styles->ItemAt(i);
		if(fs && fs->name->Compare(style)==0)
			return fs;
	}
	return NULL;
}
