#include <String.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <File.h>
#include <Message.h>
#include <String.h>
#include "FontFamily.h"
#include "FontServer.h"
#include "ServerFont.h"
#include "ServerProtocol.h"

//#define DEBUG_FONTSERVER
//#define DEBUG_SCANDIR

#ifdef DEBUG_FONTSERVER
#include <stdio.h>
#else
	#ifdef DEBUG_SCANDIR
	#include <stdio.h>
	#endif
#endif

FTC_Manager ftmanager; 
FT_Library ftlib;
FontServer *fontserver;

static FT_Error face_requester(FTC_FaceID face_id, FT_Library library,
	FT_Pointer request_data, FT_Face *aface)
{ 
	CachedFace face = (CachedFace) face_id;
	return FT_New_Face(ftlib,face->file_path.String(),face->face_index,aface); 
} 


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

#ifdef DEBUG_FONTSERVER
if(!init)
	printf("Couldn't initialize FreeType library\n");
#endif
}

FontServer::~FontServer(void)
{
	delete_sem(lock);
	delete families;
	FTC_Manager_Done(ftmanager);
	FT_Done_FreeType(ftlib);
}

void FontServer::Lock(void)
{
	acquire_sem(lock);
}

void FontServer::Unlock(void)
{
	release_sem(lock);
}

int32 FontServer::CountFamilies(void)
{
	if(init)
		return families->CountItems();
	return 0;
}

int32 FontServer::CountStyles(const char *family)
{
	FontFamily *f=_FindFamily(family);
	if(f)
	{
		return f->CountStyles();
	}
	return 0;
}

void FontServer::RemoveFamily(const char *family)
{
	FontFamily *f=_FindFamily(family);
	if(f)
	{
		families->RemoveItem(f);
		delete f;
	}
#ifdef DEBUG_FONTSERVER
	else
		printf("FontServer::RemoveFamily(): Couldn't remove %s\n",family);
#endif
}

FontFamily *FontServer::_FindFamily(const char *name)
{
	if(!init)
		return NULL;
	int32 count=families->CountItems(), i;
	FontFamily *family;
	for(i=0; i<count; i++)
	{
		family=(FontFamily*)families->ItemAt(i);
		if(strcmp(family->GetName(),name)==0)
			return family;
	}
//#ifdef DEBUG_FONTSERVER
//printf("FontServer::_FindFamily(): Couldn't find %s\n",name);
//#endif
	return NULL;
}

status_t FontServer::ScanDirectory(const char *fontspath)
{
	// This bad boy does all the real work. It loads each entry in the
	// directory. If a valid font file, it adds both the family and the style.
	// Both family and style are stored internally as BStrings. Once everything

#ifdef DEBUG_SCANDIR
printf("FontServer::ScanDirectory(%s)\n",fontspath);
#endif
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
//	int32 familycount;

	stat=dir.SetTo(fontspath);
	if(stat!=B_OK)
	{
#ifdef DEBUG_SCANDIR
printf("FontServer::ScanDirectory(): couldn't open path\n");
#endif
		return stat;
	}
	refcount=dir.CountEntries();		
#ifdef DEBUG_SCANDIR
printf("FontServer::ScanDirectory(): %ld entries\n",refcount);
#endif

	for(i=0;i<refcount;i++)
	{
		if(dir.GetNextEntry(&entry)==B_ENTRY_NOT_FOUND)
		{
#ifdef DEBUG_SCANDIR
printf("FontServer::ScanDirectory(): ran out of entries\n");
#endif
			break;
		}
		stat=entry.GetPath(&path);
		if(stat!=B_OK)
		{
#ifdef DEBUG_SCANDIR
if(stat==B_NO_INIT)
	printf("FontServer::ScanDirectory(): BEntry not initialized\n");
if(stat==B_BUSY)
	printf("FontServer::ScanDirectory(): BEntry busy\n");
#endif
			continue;
		}
		
	    error=FT_New_Face(ftlib,path.Path(),0,&face);

	    if (error!=0)
    	{
#ifdef DEBUG_SCANDIR
printf("FontServer::ScanDirectory(): FreeType couldn't load %s\n", path.Path());
#endif
			continue;
		}
		
		charmap=_GetSupportedCharmap(face);
		if(!charmap)
    	{
#ifdef DEBUG_SCANDIR
printf("FontServer::ScanDirectory(): %s has no supported charmaps\n", path.Path());
#endif
		    FT_Done_Face(face);
		    continue;
    	}
		
		face->charmap=charmap;

		family=_FindFamily(face->family_name);
		
		if(!family)
		{
			family=new FontFamily(face->family_name);
			families->AddItem(family);
		}
		
		if(family->HasStyle(face->style_name))
		{
#ifdef DEBUG_SCANDIR
printf("FontServer::ScanDirectory(): %s : %s already loaded\n", face->family_name,face->style_name);
#endif
			FT_Done_Face(face);
			continue;
		}

		// Has vertical metrics?
#ifdef DEBUG_SCANDIR
		if(FT_HAS_VERTICAL(face))
			printf("%s : %s has vertical metrics\n",face->family_name, face->style_name);
#endif
		family->AddStyle(path.Path(),face);
		validcount++;

#ifdef DEBUG_SCANDIR
printf( "FontServer::ScanDirectory: Added %s : %s\n", face->family_name, face->style_name);
if(face->num_fixed_sizes>0)
{
	for(int32 j=0;j<face->num_fixed_sizes;++j)
		printf( "  Fixed size #%ld : %dx%d\n", j, face->available_sizes[j].width, face->available_sizes[j].height );
}
#endif	

		FT_Done_Face(face);
	}	// end for(i<refcount)

#ifdef DEBUG_SCANDIR
printf("Directory %s contains %ld supported fonts\n", fontspath, validcount);
#endif
	dir.Unset();

	// This step is in the AtheOS code, but I doubt it will be necessary here
	// because of implementation differences between this and AtheOS.

	// cull empty families from family list
/*	familycount=families->CountItems();
	for(i=0;i<familycount;i++)
	{
		family=(FontFamily*)families->ItemAt(i);
		if(family)
		{
			if(family->CountStyles()<1)
			{
#ifdef DEBUG_SCANDIR
printf("FontServer::ScanDirectory(): deleting empty family %s\n", family->GetName());
#endif
				families->RemoveItem(family);
				delete family;
			}
		}
	}
*/
	need_update=true;
	return B_OK;
}

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
#ifdef DEBUG_SCANDIR
printf("Font %s:%s has unknown platform id %d\n",face->family_name,face->style_name, charmap->platform_id);
#endif
				break;
			}
		}
	}
	return NULL;

}

/*
This saves all family names and styles to the file 
/boot/home/config/app_server/fontlist as a flattened BMessage

This operation is not done very often because the access to disk adds a significant 
performance hit.

The format for storage consists of two things: an array of strings with the name 'family'
and a number of small string arrays which have the name of the font family. These are
the style lists. 

Additionally, any fonts which have bitmap strikes contained in them or any fonts which
are fixed-width are named in the arrays 'tuned' and 'fixed'

*/
void FontServer::SaveList(void)
{
#ifdef DEBUG_FONTSERVER
	printf("FontServer::SaveList()\n");
#endif
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

		famname=fam->GetName();
				
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
#ifdef DEBUG_FONTSERVER
familymsg.PrintToStream();
#endif
		familymsg.MakeEmpty();
	}
	
	BFile file(SERVER_FONT_LIST,B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if(file.InitCheck()==B_OK)
		fontmsg.Flatten(&file);
#ifdef DEBUG_FONTSERVER
	else
	{
		printf("FontServer::SaveList(): Couldn't open %s for write\n",SERVER_FONT_LIST);
	}
#endif
}

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

ServerFont *FontServer::GetSystemPlain(void)
{
	if(plain)
	{
		ServerFont *f=new ServerFont(*plain);
		return f;
	}
	return NULL;
}

ServerFont *FontServer::GetSystemBold(void)
{
	if(bold)
	{
		ServerFont *f=new ServerFont(*bold);
		return f;
	}
	return NULL;
}

ServerFont *FontServer::GetSystemFixed(void)
{
	if(fixed)
	{
		ServerFont *f=new ServerFont(*fixed);
		return f;
	}
	return NULL;
}

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

