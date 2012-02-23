/*

Copyright (c) 2001, 2002 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <stdio.h>
#include <malloc.h>
#include <StorageKit.h>
#include "Fonts.h"
#include "Report.h"

// FontFile

// ---------------------------------------
// Implementation of BArchivable interface
// ---------------------------------------


// --------------------------------------------------
FontFile::FontFile(BMessage *archive)
{
	int8 type;
	archive->FindString("name", &fName);
	archive->FindString("path", &fPath);
	archive->FindInt64 ("size", &fSize);
	if (B_OK == archive->FindInt8  ("type", &type))
		fType = (font_type)type;
	else
		fType = unknown_type;
	archive->FindBool  ("embed", &fEmbed);
	archive->FindString("subst", &fSubst);
}

// --------------------------------------------------
BArchivable *FontFile::Instantiate(BMessage *archive)
{
	if (!validate_instantiation(archive, "FontFile")) {
		return NULL;
	}
	return new FontFile(archive);
}


// --------------------------------------------------
status_t FontFile::Archive(BMessage *archive, bool deep) const
{
	archive->AddString("class", "FontFile");
	archive->AddString("name", fName);
	archive->AddString("path", fPath);
	archive->AddInt64 ("size", fSize);
	archive->AddInt8  ("type", (int8)fType);
	archive->AddBool  ("embed", fEmbed);
	archive->AddString("subst", fSubst);
	return B_OK;
}


// Fonts

#define fmin(x, y) ( (x < y) ? x : y);

#define TRUETYPE_VERSION		0x00010000
#define OPENTYPE_CFF_VERSION	'OTTO'

#define TRUETTYPE_TABLE_NAME_TAG	'name'

static uint16 ttf_get_uint16(FILE * ttf);
static uint32 ttf_get_uint32(FILE *ttf);
static status_t ttf_get_fontname(const char * path, char * fontname, size_t fn_size);
static status_t psf_get_fontname(const char * path, char * fontname, size_t fn_size);


// --------------------------------------------------
Fonts::Fonts()
{
	SetDefaultCJKOrder();
}


void
Fonts::SetDefaultCJKOrder()
{
	SetCJKOrder(0, japanese_encoding,     true);
	SetCJKOrder(1, chinese_gb1_encoding,  true);
	SetCJKOrder(2, chinese_cns1_encoding, true);
	SetCJKOrder(3, korean_encoding,       true);
}

// --------------------------------------------------
bool
Fonts::SetCJKOrder(int i, font_encoding  enc, bool  active)
{
	if (0 <= i && i < no_of_cjk_encodings) {
		fCJKOrder[i].encoding = enc;
		fCJKOrder[i].active   = active;
		return true;
	}
	return false;
}


// --------------------------------------------------
bool
Fonts::GetCJKOrder(int i, font_encoding& enc, bool& active) const
{
	if (0 <= i && i < no_of_cjk_encodings) {
		enc    = fCJKOrder[i].encoding;
		active = fCJKOrder[i].active;
		return true;
	}
	return false;
}


// ---------------------------------------
// Implementation of BArchivable interface
// ---------------------------------------


// --------------------------------------------------
Fonts::Fonts(BMessage *archive)
{
	BMessage m;
	for (int i = 0; archive->FindMessage("fontfile", i, &m) == B_OK; i++) {
		BArchivable* base = instantiate_object(&m);
		if (base) {
			FontFile* f = dynamic_cast<FontFile*>(base);
			if (f) fFontFiles.AddItem(f);
			else delete f;
		}
	}
	if (archive->FindMessage("cjk_order", &m) == B_OK) {
		for (int i = 0; i < no_of_cjk_encodings; i++) {
			bool active; int32 encoding;
			if (m.FindInt32("encoding", i, &encoding) == B_OK &&
				m.FindBool("active", i, &active) == B_OK &&
				first_cjk_encoding <= encoding &&
				encoding < first_cjk_encoding + no_of_cjk_encodings) {
				SetCJKOrder(i, (font_encoding)encoding, active);
			} else {
				SetDefaultCJKOrder(); return;
			}
		}
		return;
	}		
	SetDefaultCJKOrder();
}


// --------------------------------------------------
BArchivable *Fonts::Instantiate(BMessage *archive)
{
	if (!validate_instantiation(archive, "Fonts")) {
		return NULL;
	}
	return new Fonts(archive);
}


// --------------------------------------------------
status_t Fonts::Archive(BMessage *archive, bool deep) const
{
	archive->AddString("class", "Fonts");
	const int n = Length();
	for (int i = 0; i < n; i++) {
		FontFile* f = At(i);
		BMessage m;
		if (f->Archive(&m) == B_OK) {
			archive->AddMessage("fontfile", &m);
		}
	}
	BMessage m;
	font_encoding enc;
	bool active;
	for (int i = 0; GetCJKOrder(i, enc, active); i++) {
		m.AddInt32("encoding", enc);
		m.AddBool("active", active);
	}
	archive->AddMessage("cjk_order", &m);
	return B_OK;
}


// --------------------------------------------------
status_t 
Fonts::CollectFonts()
{
	BPath				path;
	directory_which	*	which_dir;
	directory_which		lookup_dirs[] = {
		B_BEOS_FONTS_DIRECTORY,
		// B_COMMON_FONTS_DIRECTORY,	// seem to be the same directory than B_USER_FONTS_DIRECTORY!!!
		B_USER_FONTS_DIRECTORY,
		(directory_which) -1
	};

	which_dir = lookup_dirs;
	while (*which_dir >= 0) {
		if ( find_directory(*which_dir, &path) == B_OK )
			LookupFontFiles(path);

		which_dir++;
	};
		
	return B_OK;
}


// --------------------------------------------------
status_t
Fonts::LookupFontFiles(BPath path)
{
	BDirectory 	dir(path.Path());
	BEntry 		entry;

	if (dir.InitCheck() != B_OK)
		return B_ERROR;

	dir.Rewind();
	while (dir.GetNextEntry(&entry) >= 0) {
		BPath 		name;
		char 		fn[512];
		font_type	ft = unknown_type; // to keep the compiler silent.
		off_t 		size;
		status_t	status;
		
		entry.GetPath(&name);
		if (entry.IsDirectory())
			// recursivly lookup in sub-directories...
			LookupFontFiles(name);

		if (! entry.IsFile())
			continue;

		fn[0] = 0;
		ft = unknown_type;
				
		// is it a truetype file?
		status = ttf_get_fontname(name.Path(), fn, sizeof(fn));
		if (status == B_OK ) {
			ft = true_type_type;
		} else {
			// okay, maybe it's a postscript type file?
			status = psf_get_fontname(name.Path(), fn, sizeof(fn));
			if (status == B_OK) {
				ft = type1_type;
			}
		} 

		if (ft == unknown_type)
			// not a font file...
			continue;
										
		if (entry.GetSize(&size) != B_OK)
			size = 1024*1024*1024;
		
		REPORT(kDebug, -1, "Installed font %s -> %s", fn, name.Path());			
		fFontFiles.AddItem(new FontFile(fn, name.Path(), size, ft, size < 100*1024));
	}	// while dir.GetNextEntry()...	

	return B_OK;
}

// --------------------------------------------------
void
Fonts::SetTo(BMessage *archive)
{
	BArchivable *a = Instantiate(archive);
	Fonts *f = a ? dynamic_cast<Fonts*>(a) : NULL;
	if (f) {
		const int n = Length();
		const int m = f->Length();
		for (int i = 0; i < m; i ++) {
			FontFile *font = f->At(i);
			for (int j = 0; j < n; j ++) {
				if (strcmp(font->Path(), At(j)->Path()) == 0) {
					At(j)->SetEmbed(font->Embed());
					At(j)->SetSubst(font->Subst());
					break;
				}
			}
		}
		
		font_encoding enc; 
		bool          active;
		for (int i = 0; f->GetCJKOrder(i, enc, active); i++) {
			SetCJKOrder(i, enc, active);
		}
		delete f;
	}
}


// --------------------------------------------------
static uint16 ttf_get_uint16(FILE * ttf)
{
    uint16 v;

	if (fread(&v, 1, 2, ttf) != 2)
		return 0;

	return B_BENDIAN_TO_HOST_INT16(v);
}


// --------------------------------------------------
static uint32 ttf_get_uint32(FILE *ttf)
{
    uint32 buf;

    if (fread(&buf, 1, 4, ttf) != 4)
		return 0;

	return B_BENDIAN_TO_HOST_INT32(buf);
}


// --------------------------------------------------
static status_t ttf_get_fontname(const char * path, char * fontname, size_t fn_size)
{
	FILE *		ttf;
	status_t	status;
	uint16		nb_tables, nb_records;
	uint16		i;
	uint32		tag;
	uint32		table_offset;
	uint32		strings_offset;
	char		family_name[256];
	char		face_name[256];
	int			names_found;

	status = B_ERROR;
	
	ttf = fopen(path, "rb");
	if (! ttf) 
		return status;

    tag = ttf_get_uint32(ttf);		/* version */
	switch(tag) {
		case TRUETYPE_VERSION:
		case OPENTYPE_CFF_VERSION:
			break;
			
		default:
			goto exit;
	}

    /* set up table directory */
    nb_tables = ttf_get_uint16(ttf);

	fseek(ttf, 12, SEEK_SET);
	
	table_offset = 0;	// quiet the compiler...

    for (i = 0; i < nb_tables; ++i) {
		tag				= ttf_get_uint32(ttf);
		ttf_get_uint32(ttf);	// checksum
		table_offset	= ttf_get_uint32(ttf);
		
		if (tag == TRUETTYPE_TABLE_NAME_TAG)
			break;
	}

	if (tag != TRUETTYPE_TABLE_NAME_TAG)
		// Mandatory name table not found!
		goto exit;
		
	// move to name table start
	fseek(ttf, table_offset, SEEK_SET);
		
	ttf_get_uint16(ttf);	// name table format (must be 0!)
    nb_records		= ttf_get_uint16(ttf);
	strings_offset	= table_offset + ttf_get_uint16(ttf); // string storage offset is from table offset

	//    offs = ttf->dir[idx].offset + tp->offsetStrings;

	// printf("  pid   eid   lid   nid   len offset value\n");
        //  65536 65536 65536 65536 65536 65536  ......

	family_name[0] = 0;
	face_name[0] = 0;
	names_found = 0;

	for (i = 0; i < nb_records; ++i) {
		uint16	platform_id, encoding_id, name_id;
		uint16	string_len, string_offset;

		platform_id		= ttf_get_uint16(ttf);
		encoding_id		= ttf_get_uint16(ttf);
		ttf_get_uint16(ttf);	// language_id
		name_id			= ttf_get_uint16(ttf);
		string_len		= ttf_get_uint16(ttf);
		string_offset	= ttf_get_uint16(ttf);

		if ( name_id != 1 && name_id != 2 )
			continue;

		// printf("%5d %5d %5d %5d %5d %5d ", 
		// 	platform_id, encoding_id, language_id, name_id, string_len, string_offset);

		if (string_len != 0) {
			long	pos;
			char *	buffer;

			pos = ftell(ttf);
			fseek(ttf, strings_offset + string_offset, SEEK_SET);

			buffer = (char *) malloc(string_len + 16);

			fread(buffer, 1, string_len, ttf); 
			buffer[string_len] = '\0';

			fseek(ttf, pos, SEEK_SET);
			
			if ( (platform_id == 3 && encoding_id == 1) || // Windows Unicode
				 (platform_id == 0) ) { // Unicode 
				// dirty unicode -> ascii conversion
				int k;

				for (k=0; k < string_len/2; k++)
					buffer[k] = buffer[2*k + 1];
				buffer[k] = '\0';
			}

			// printf("%s\n", buffer);
			
			if (name_id == 1)
				strncpy(family_name, buffer, sizeof(family_name));
			else if (name_id == 2)
				strncpy(face_name, buffer, sizeof(face_name));

			names_found += name_id;

			free(buffer);
		}
		// else
			// printf("<null>\n");

		if (names_found == 3)
			break;
	}
		
	if (names_found == 3) {
#ifndef __POWERPC__
		snprintf(fontname, fn_size, "%s-%s", family_name, face_name);
#else
		sprintf(fontname, "%s-%s", family_name, face_name);
#endif
		status = B_OK;
	}
		
exit:
	fclose(ttf);
	return status;
}


// --------------------------------------------------
static status_t psf_get_fontname(const char * path, char * fontname, size_t fn_size)
{
	FILE *		psf;
	status_t	status;
	int			i;
	char		line[1024];
	char * 		token;
	char *		name;

	// *.afm	search for "FontName <font_name_without_blank>" line
	// *.pfa 	search for "/FontName /<font_name_without_blank> def" line

	status = B_ERROR;
	
	psf = fopen(path, "r");
	if (! psf) 
		return status;

	name = NULL;
	
	i = 0;
	while ( fgets(line, sizeof(line), psf) != NULL ) {
		i++;
		if ( i > 64 )
			// only check the first 64 lines of files...
			break;
		
		token = strtok(line, " \r\n");
		if (! token)
			continue;
			
		if (strcmp(token, "FontName") == 0)
			name = strtok(NULL, " \r\n");
		else if (strcmp(token, "/FontName") == 0) {
			name = strtok(NULL, " \r\n");
			if (name)
				name++;	// skip the '/'
		}

		if (name)
			break;
	}
		
	if (name) {
		strncpy(fontname, name, fn_size);
		status = B_OK;
	}
		
	fclose(psf);
	return status;
}

// Implementation of UserDefinedEncodings

UserDefinedEncodings::UserDefinedEncodings()
	: fCurrentEncoding(0)
	, fCurrentIndex(0)
{
	memset(fUsedMask, 0, sizeof(fUsedMask));
	memset(fEncoding, 0, sizeof(fEncoding));
	memset(fIndex, 0, sizeof(fIndex));
}

bool UserDefinedEncodings::Get(uint16 unicode, uint8 &encoding, uint8 &index) {
	bool missing = !IsUsed(unicode);
	if (missing) {
		SetUsed(unicode);
		fEncoding[unicode] = fCurrentEncoding; fIndex[unicode] = fCurrentIndex;
		if (fCurrentIndex == 255) {
			fCurrentIndex = 0; fCurrentEncoding ++;
		} else {
			fCurrentIndex ++;
		}
	}
	encoding = fEncoding[unicode]; index = fIndex[unicode];
	return missing;
}
