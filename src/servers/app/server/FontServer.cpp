//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		FontServer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Handles the largest part of the font subsystem
//  
//------------------------------------------------------------------------------
#include <String.h>
#include <Directory.h>
#include <Entry.h>
#include <storage/Path.h>	// specified to be able to build under Dano
#include <File.h>
#include <Message.h>
#include <String.h>

#include <FontServer.h>
#include <FontFamily.h>
#include <ServerFont.h>
#include "ServerConfig.h"

extern FTC_Manager ftmanager; 
FT_Library ftlib;
FontServer *fontserver=NULL;

//#define PRINT_FONT_LIST

/*!
	\brief Access function to request a face via the FreeType font cache
*/
static FT_Error face_requester(FTC_FaceID face_id, FT_Library library,
	FT_Pointer request_data, FT_Face *aface)
{ 
	CachedFace face = (CachedFace) face_id;
	return FT_New_Face(ftlib,face->file_path.String(),face->face_index,aface); 
} 

//! Does basic set up so that directories can be scanned
FontServer::FontServer(void)
{
	lock=create_sem(1,"fontserver_lock");
	init=(FT_Init_FreeType(&ftlib)==0)?true:false;

/*
	Fire up the font caching subsystem.
	The three zeros tell FreeType to use the defaults, which are 2 faces,
	4 face sizes, and a maximum of 200000 bytes. I will probably change
	these numbers in the future to maximize performance for your "average"
	application.
*/
	if(FTC_Manager_New(ftlib,0,0,0,&face_requester,NULL,&ftmanager)!=0
				&& init)
		init=false;
		
	families=new BList(0);
	plain=NULL;
	bold=NULL;
	fixed=NULL;
}

//! Frees items allocated in the constructor and shuts down FreeType
FontServer::~FontServer(void)
{
	delete_sem(lock);
	delete families;
	FTC_Manager_Done(ftmanager);
	FT_Done_FreeType(ftlib);
}

//! Locks access to the font server
void FontServer::Lock(void)
{
	acquire_sem(lock);
}

//! Unlocks access to the font server
void FontServer::Unlock(void)
{
	release_sem(lock);
}

/*!
	\brief Counts the number of font families available
	\return The number of unique font families currently available
*/
int32 FontServer::CountFamilies(void)
{
	if(init)
		return families->CountItems();
	return 0;
}

/*!
	\brief Counts the number of styles available in a font family
	\param family Name of the font family to scan
	\return The number of font styles currently available for the font family
*/
int32 FontServer::CountStyles(const char *family)
{
	FontFamily *f=_FindFamily(family);
	if(f)
	{
		return f->CountStyles();
	}
	return 0;
}

/*!
	\brief Removes a font family from the font list
	\param family The family to remove
*/
void FontServer::RemoveFamily(const char *family)
{
	FontFamily *f=_FindFamily(family);
	if(f)
	{
		families->RemoveItem(f);
		delete f;
	}
}

/*!
	\brief Protected function which locates a FontFamily object
	\param name The family to find
	\return Pointer to the specified family or NULL if not found.
	
	Do NOT delete the FontFamily returned by this function.
*/
FontFamily *FontServer::_FindFamily(const char *name)
{
	if(!init)
		return NULL;
	int32 count=families->CountItems(), i;
	FontFamily *family;
	for(i=0; i<count; i++)
	{
		family=(FontFamily*)families->ItemAt(i);
		if(strcmp(family->Name(),name)==0)
			return family;
	}
	return NULL;
}

/*!
	\brief Scan a folder for all valid fonts
	\param fontspath Path of the folder to scan.
	\return 
	- \c B_OK				Success
	- \c B_NAME_TOO_LONG	The path specified is too long
	- \c B_ENTRY_NOT_FOUND	The path does not exist
	- \c B_LINK_LIMIT		A cyclic loop was detected in the file system
	- \c B_BAD_VALUE		Invalid input specified
	- \c B_NO_MEMORY		Insufficient memory to open the folder for reading
	- \c B_BUSY				A busy node could not be accessed
	- \c B_FILE_ERROR		An invalid file prevented the operation.
	- \c B_NO_MORE_FDS		All file descriptors are in use (too many open files). 
*/
status_t FontServer::ScanDirectory(const char *fontspath)
{
	// This bad boy does all the real work. It loads each entry in the
	// directory. If a valid font file, it adds both the family and the style.
	// Both family and style are stored internally as BStrings. Once everything

	BDirectory dir;
	BEntry entry;
	BPath path;
	entry_ref ref;
	status_t stat;
	int32 refcount,i, validcount=0;
    FT_Face  face;
    FT_Error error;
    FT_CharMap charmap;
    FontFamily *family;

	stat=dir.SetTo(fontspath);
	if(stat!=B_OK)
		return stat;

	refcount=dir.CountEntries();		

	for(i=0;i<refcount;i++)
	{
		if(dir.GetNextEntry(&entry)==B_ENTRY_NOT_FOUND)
			break;

		stat=entry.GetPath(&path);
		if(stat!=B_OK)
			continue;
		
	    error=FT_New_Face(ftlib,path.Path(),0,&face);

	    if (error!=0)
			continue;
		
		charmap=_GetSupportedCharmap(face);
		if(!charmap)
    	{
		    FT_Done_Face(face);
		    continue;
    	}
		
		face->charmap=charmap;

		family=_FindFamily(face->family_name);
		
		if(!family)
		{
			#ifdef PRINT_FONT_LIST
			printf("Font Family: %s\n",face->family_name);
			#endif

			family=new FontFamily(face->family_name);
			families->AddItem(family);
		}
		
		if(family->HasStyle(face->style_name))
		{
			FT_Done_Face(face);
			continue;
		}

		#ifdef PRINT_FONT_LIST
		printf("\tFont Style: %s\n",face->style_name);
		#endif

		// Has vertical metrics?
		family->AddStyle(path.Path(),face);
		validcount++;

		FT_Done_Face(face);
	}	// end for(i<refcount)

	dir.Unset();

	need_update=true;
	return B_OK;
}

/*!
	\brief Finds and returns the first valid charmap in a font
	
	\param face Font handle obtained from FT_Load_Face()
	\return An FT_CharMap or NULL if unsuccessful
*/
FT_CharMap FontServer::_GetSupportedCharmap(const FT_Face &face)
{
	int32 i;
	FT_CharMap charmap;
	for(i=0; i< face->num_charmaps; i++)
	{
		charmap=face->charmaps[i];
		switch(charmap->platform_id)
		{
			case 3:
			{
				// if Windows Symbol or Windows Unicode
				if(charmap->encoding_id==0 || charmap->encoding_id==1)
					return charmap;
				break;
			}
			case 1:
			{
				// if Apple Unicode
				if(charmap->encoding_id==0)
					return charmap;
				break;
			}
			case 0:
			{
				// if Apple Roman
				if(charmap->encoding_id==0)
					return charmap;
				break;
			}
			default:
			{
				break;
			}
		}
	}
	return NULL;

}

/*!
	\brief This saves all family names and styles to the file specified in
	ServerConfig.h as SERVER_FONT_LIST as a flattened BMessage.

	This operation is not done very often because the access to disk adds a significant 
	performance hit.

	The format for storage consists of two things: an array of strings with the name 'family'
	and a number of small string arrays which have the name of the font family. These are
	the style lists. 

	Additionally, any fonts which have bitmap strikes contained in them or any fonts which
	are fixed-width are named in the arrays 'tuned' and 'fixed'.
*/
void FontServer::SaveList(void)
{
	int32 famcount=0, stycount=0,i=0,j=0;
	FontFamily *fam;
	FontStyle *sty;
	BMessage fontmsg, familymsg('FONT');
	BString famname, styname, extraname;
	bool fixed,tuned;
	
	famcount=families->CountItems();
	for(i=0; i<famcount; i++)
	{
		fam=(FontFamily*)families->ItemAt(i);
		fixed=false;
		tuned=false;
		if(!fam)
			continue;

		famname=fam->Name();
				
		// Add the family to the message
		familymsg.AddString("name",famname);
		
		stycount=fam->CountStyles();
		for(j=0;j<stycount;j++)
		{
			styname.SetTo(fam->GetStyle(j));
			if(styname.CountChars()>0)
			{
				// Add to list
				familymsg.AddString("styles", styname);
				
				// Check to see if it has prerendered strikes (has "tuned" fonts)
				sty=fam->GetStyle(styname.String());
				if(!sty)
					continue;
				
				if(sty->HasTuned() && sty->IsScalable())
					tuned=true;

				// Check to see if it is fixed-width
				if(sty->IsFixedWidth())
					fixed=true;
			}
		}
		if(tuned)
			familymsg.AddBool("tuned",true);
		if(fixed)
			familymsg.AddBool("fixed",true);
		
		fontmsg.AddMessage("family",&familymsg);
		familymsg.MakeEmpty();
	}
	
	BFile file(SERVER_FONT_LIST,B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if(file.InitCheck()==B_OK)
		fontmsg.Flatten(&file);
}

/*!
	\brief Retrieves the FontStyle object
	\param family The font's family
	\param face The font's style
	\return The FontStyle having those attributes or NULL if not available
*/
FontStyle *FontServer::GetStyle(font_family family, font_style face)
{
	FontFamily *ffam=_FindFamily(family);

	if(ffam)
	{
		FontStyle *fsty=ffam->GetStyle(face);
		return fsty;
	}
	return NULL;
}

/*!
	\brief Returns the current object used for the regular style
	\return A ServerFont pointer which is the plain font.
	
	Do NOT delete this object. If you access it, make a copy of it.
*/
ServerFont *FontServer::GetSystemPlain(void)
{
	if(plain)
	{
		ServerFont *f=new ServerFont(*plain);
		return f;
	}
	return NULL;
}

/*!
	\brief Returns the current object used for the bold style
	\return A ServerFont pointer which is the bold font.
	
	Do NOT delete this object. If you access it, make a copy of it.
*/
ServerFont *FontServer::GetSystemBold(void)
{
	if(bold)
	{
		ServerFont *f=new ServerFont(*bold);
		return f;
	}
	return NULL;
}

/*!
	\brief Returns the current object used for the fixed style
	\return A ServerFont pointer which is the fixed font.
	
	Do NOT delete this object. If you access it, make a copy of it.
*/
ServerFont *FontServer::GetSystemFixed(void)
{
	if(fixed)
	{
		ServerFont *f=new ServerFont(*fixed);
		return f;
	}
	return NULL;
}

/*!
	\brief Sets the system's plain font to the specified family and style
	\param family Name of the font's family
	\param style Name of the style desired
	\param size Size desired
	\return true if successful, false if not.
	
*/
bool FontServer::SetSystemPlain(const char *family, const char *style, float size)
{
	FontFamily *fam=_FindFamily(family);
	if(!fam)
		return false;
	FontStyle *sty=fam->GetStyle(style);
	if(!sty)
		return false;
	
	if(plain)
		delete plain;
	plain=sty->Instantiate(size);
	return true;
}

/*!
	\brief Sets the system's bold font to the specified family and style
	\param family Name of the font's family
	\param style Name of the style desired
	\param size Size desired
	\return true if successful, false if not.
	
*/
bool FontServer::SetSystemBold(const char *family, const char *style, float size)
{
	FontFamily *fam=_FindFamily(family);
	if(!fam)
		return false;
	FontStyle *sty=fam->GetStyle(style);
	if(!sty)
		return false;
	
	if(bold)
		delete bold;
	bold=sty->Instantiate(size);
	return true;
}

/*!
	\brief Sets the system's fixed font to the specified family and style
	\param family Name of the font's family
	\param style Name of the style desired
	\param size Size desired
	\return true if successful, false if not.
	
*/
bool FontServer::SetSystemFixed(const char *family, const char *style, float size)
{
	FontFamily *fam=_FindFamily(family);
	if(!fam)
		return false;
	FontStyle *sty=fam->GetStyle(style);
	if(!sty)
		return false;
	
	if(fixed)
		delete fixed;
	fixed=sty->Instantiate(size);
	return true;
}

