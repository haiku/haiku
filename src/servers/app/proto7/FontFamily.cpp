#include "FontFamily.h"
#include "ServerFont.h"
#include FT_CACHE_H

extern FTC_Manager ftmanager;

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
}

FontStyle::~FontStyle(void)
{
	delete name;
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

const char *FontStyle::Style(void)
{
	return name->String();
}

FT_Face FontStyle::GetFace(void)
{
	FT_Face f;
	return (FTC_Manager_Lookup_Face(ftmanager,cachedface,&f)!=0)?f:NULL;
}

int16 FontStyle::ConvertToUnicode(uint16 c)
{
	FT_Face f;
	if(FTC_Manager_Lookup_Face(ftmanager,cachedface,&f)!=0)
		return 0;
	
	return FT_Get_Char_Index(f,c);
}

ServerFont *FontStyle::Instantiate(float size, float rotation=0.0, float shear=90.0)
{
	ServerFont *f=new ServerFont(this, size, rotation, shear);
	instances->AddItem(f);
	return f;
}

FontFamily::FontFamily(const char *namestr)
{
	name=new BString(namestr);
	styles=new BList(0);
}

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

const char *FontFamily::GetName(void)
{
	return name->String();
}

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

int32 FontFamily::CountStyles(void)
{
	return styles->CountItems();
}

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

const char *FontFamily::GetStyle(int32 index)
{
	FontStyle *fs=(FontStyle*)styles->ItemAt(index);
	if(!fs)
		return NULL;
	return fs->Style();
}

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
