/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "Utils.h"
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include <BeBuild.h>

#include <malloc.h>
#include <stdio.h>


// some private font information structs
namespace BPrivate {

typedef struct font_folder_info {
	//char name[256];
	char *name;
	uint32 flags;
} font_folder_info;

typedef struct font_file_info {
	char *name;
	uint32 flags;
	font_family family;
	font_style style;
	uint32 dummy;
} font_file_info;

};

using namespace BPrivate;

// this is PRIVATE to libbe and NOT in R5!!!
extern long _count_font_folders_(void);
extern long _count_font_files_(long);
extern status_t _get_nth_font_file_(long, font_file_info **);
extern status_t _get_nth_font_folder_(long, font_folder_info **);

status_t
find_font_file(entry_ref *to, font_family family, font_style style, float size)
{
#ifdef B_BEOS_VERSION_DANO
	status_t err = ENOENT;
	long i, fontcount, foldercount;
	font_file_info *ffi;
	font_folder_info *fdi;
	bool found = false;
	(void)size;
	
	fontcount = _count_font_files_(0);
	for (i = 0; i < fontcount; i++) {
		err = _get_nth_font_file_(i, &ffi);
		if (err)
			continue;
		if (strcmp(ffi->family, family) || strcmp(ffi->style, style))
			continue;
		found = true;
		break;
	}
	if (!found)
		return ENOENT;
	foldercount = _count_font_folders_();
	for (i = 0; i < fontcount; i++) {
		err = _get_nth_font_folder_(i, &fdi);
		if (err)
			continue;
		BPath ffile(fdi->name);
		ffile.Append(ffi->name);
		printf("find_font_file: looking for '%s' in '%s'\n", ffi->name, fdi->name);
		BEntry ent(ffile.Path());
		if (ent.InitCheck())
			continue;
		printf("find_font_file: found\n.");
		return ent.GetRef(to);
	}
#endif
	return ENOENT;
}


#define _BORK(_t) \
	err = find_directory(_t, &path); \
	if (!err && (s = dir->FindFirst(path.Path())) >= 0) { \
		printf("found %s\n", #_t); \
		dir->Remove(s, strlen(path.Path()) - s); \
		BString tok(#_t); \
		tok.Prepend("${"); \
		tok.Append("}"); \
		dir->Insert(tok, s); \
		return B_OK; \
	} \

status_t
escape_find_directory(BString *dir)
{
	status_t err;
	BPath path;
	int32 s;
	
	_BORK(B_DESKTOP_DIRECTORY);
	_BORK(B_TRASH_DIRECTORY);
	//_BORK(B_ROOT_DIRECTORY);
	
	//_BORK(B_BEOS_BOOT_DIRECTORY);
	_BORK(B_BEOS_FONTS_DIRECTORY);
	_BORK(B_BEOS_LIB_DIRECTORY);
	_BORK(B_BEOS_SERVERS_DIRECTORY);
	_BORK(B_BEOS_APPS_DIRECTORY);
	_BORK(B_BEOS_BIN_DIRECTORY);
	_BORK(B_BEOS_ETC_DIRECTORY);
	_BORK(B_BEOS_DOCUMENTATION_DIRECTORY);
	_BORK(B_BEOS_PREFERENCES_DIRECTORY);
	_BORK(B_BEOS_TRANSLATORS_DIRECTORY);
	_BORK(B_BEOS_MEDIA_NODES_DIRECTORY);
	_BORK(B_BEOS_SOUNDS_DIRECTORY);
	// not in the declared order, so others are picked first
	_BORK(B_BEOS_ADDONS_DIRECTORY);
	_BORK(B_BEOS_SYSTEM_DIRECTORY);
	_BORK(B_BEOS_DIRECTORY);
	
	_BORK(B_USER_BOOT_DIRECTORY);
	_BORK(B_USER_FONTS_DIRECTORY);
	_BORK(B_USER_LIB_DIRECTORY);
	_BORK(B_USER_SETTINGS_DIRECTORY);
	_BORK(B_USER_DESKBAR_DIRECTORY);
	_BORK(B_USER_PRINTERS_DIRECTORY);
	_BORK(B_USER_TRANSLATORS_DIRECTORY);
	_BORK(B_USER_MEDIA_NODES_DIRECTORY);
	_BORK(B_USER_SOUNDS_DIRECTORY);
	//
	_BORK(B_USER_ADDONS_DIRECTORY);
	_BORK(B_USER_CONFIG_DIRECTORY);
	_BORK(B_USER_DIRECTORY);
	
	// same for the whole block, prefer user over common
	_BORK(B_COMMON_BOOT_DIRECTORY);
	_BORK(B_COMMON_FONTS_DIRECTORY);
	_BORK(B_COMMON_LIB_DIRECTORY);
	_BORK(B_COMMON_SERVERS_DIRECTORY);
	_BORK(B_COMMON_BIN_DIRECTORY);
	_BORK(B_COMMON_ETC_DIRECTORY);
	_BORK(B_COMMON_DOCUMENTATION_DIRECTORY);
	_BORK(B_COMMON_SETTINGS_DIRECTORY);
	_BORK(B_COMMON_DEVELOP_DIRECTORY);
	_BORK(B_COMMON_LOG_DIRECTORY);
	_BORK(B_COMMON_SPOOL_DIRECTORY);
	_BORK(B_COMMON_TEMP_DIRECTORY);
	_BORK(B_COMMON_VAR_DIRECTORY);
	_BORK(B_COMMON_TRANSLATORS_DIRECTORY);
	_BORK(B_COMMON_MEDIA_NODES_DIRECTORY);
	_BORK(B_COMMON_SOUNDS_DIRECTORY);
	//
	_BORK(B_COMMON_ADDONS_DIRECTORY);
	_BORK(B_COMMON_SYSTEM_DIRECTORY);
	_BORK(B_COMMON_DIRECTORY);

	_BORK(B_APPS_DIRECTORY);
	_BORK(B_PREFERENCES_DIRECTORY);
	_BORK(B_UTILITIES_DIRECTORY);
	
	return B_OK;
}
#undef _BORK


#define _BORK(_t) \
	if (tok == #_t) { \
		err = find_directory(_t, &path); \
		if (err) return err; \
		dir->Remove(s, e - s + 1); \
		dir->Insert(path.Path(), s); \
		return B_OK; \
	} \


status_t
unescape_find_directory(BString *dir)
{
	status_t err = B_ERROR;
	int32 s, e;
	BString tok;
	BPath path;
	s = dir->FindFirst("${");
	if (s < 0)
		return B_OK;
	e = dir->FindFirst("}", s);
	if (e < 0)
		return B_OK;
	dir->CopyInto(tok, s + 2, e - s - 2);
	//printf("tok '%s'\n", tok.String());
	
	_BORK(B_DESKTOP_DIRECTORY);
	_BORK(B_TRASH_DIRECTORY);
#ifdef B_BEOS_VERSION_DANO
	_BORK(B_ROOT_DIRECTORY);
#endif
	
	_BORK(B_BEOS_DIRECTORY);
	_BORK(B_BEOS_SYSTEM_DIRECTORY);
	_BORK(B_BEOS_ADDONS_DIRECTORY);
	_BORK(B_BEOS_BOOT_DIRECTORY);
	_BORK(B_BEOS_FONTS_DIRECTORY);
	_BORK(B_BEOS_LIB_DIRECTORY);
	_BORK(B_BEOS_SERVERS_DIRECTORY);
	_BORK(B_BEOS_APPS_DIRECTORY);
	_BORK(B_BEOS_BIN_DIRECTORY);
	_BORK(B_BEOS_ETC_DIRECTORY);
	_BORK(B_BEOS_DOCUMENTATION_DIRECTORY);
	_BORK(B_BEOS_PREFERENCES_DIRECTORY);
	_BORK(B_BEOS_TRANSLATORS_DIRECTORY);
	_BORK(B_BEOS_MEDIA_NODES_DIRECTORY);
	_BORK(B_BEOS_SOUNDS_DIRECTORY);
	
	_BORK(B_COMMON_DIRECTORY);
	_BORK(B_COMMON_SYSTEM_DIRECTORY);
	_BORK(B_COMMON_ADDONS_DIRECTORY);
	_BORK(B_COMMON_BOOT_DIRECTORY);
	_BORK(B_COMMON_FONTS_DIRECTORY);
	_BORK(B_COMMON_LIB_DIRECTORY);
	_BORK(B_COMMON_SERVERS_DIRECTORY);
	_BORK(B_COMMON_BIN_DIRECTORY);
	_BORK(B_COMMON_ETC_DIRECTORY);
	_BORK(B_COMMON_DOCUMENTATION_DIRECTORY);
	_BORK(B_COMMON_SETTINGS_DIRECTORY);
	_BORK(B_COMMON_DEVELOP_DIRECTORY);
	_BORK(B_COMMON_LOG_DIRECTORY);
	_BORK(B_COMMON_SPOOL_DIRECTORY);
	_BORK(B_COMMON_TEMP_DIRECTORY);
	_BORK(B_COMMON_VAR_DIRECTORY);
	_BORK(B_COMMON_TRANSLATORS_DIRECTORY);
	_BORK(B_COMMON_MEDIA_NODES_DIRECTORY);
	_BORK(B_COMMON_SOUNDS_DIRECTORY);

	_BORK(B_USER_DIRECTORY);
	_BORK(B_USER_CONFIG_DIRECTORY);
	_BORK(B_USER_ADDONS_DIRECTORY);
	_BORK(B_USER_BOOT_DIRECTORY);
	_BORK(B_USER_FONTS_DIRECTORY);
	_BORK(B_USER_LIB_DIRECTORY);
	_BORK(B_USER_SETTINGS_DIRECTORY);
	_BORK(B_USER_DESKBAR_DIRECTORY);
	_BORK(B_USER_PRINTERS_DIRECTORY);
	_BORK(B_USER_TRANSLATORS_DIRECTORY);
	_BORK(B_USER_MEDIA_NODES_DIRECTORY);
	_BORK(B_USER_SOUNDS_DIRECTORY);
	
	_BORK(B_APPS_DIRECTORY);
	_BORK(B_PREFERENCES_DIRECTORY);
	_BORK(B_UTILITIES_DIRECTORY);
	
	return B_OK;
}
#undef _BORK


// copy a file including its attributes
#define BUFF_SZ 1024*1024
status_t
copy_file(entry_ref *ref, const char *to)
{
	char *buff;
	status_t err = B_OK;
	//off_t off;
	//size_t got;
	(void)ref; (void)to;
	
	buff = (char *)malloc(BUFF_SZ);
	// XXX: TODO
	
	
	free(buff);
	return err;
}


int
testhook()
{
	status_t err;
	BString str("/boot/home/config/fonts/ttfonts/toto.ttf");
	err = escape_find_directory(&str);
	printf("error 0x%08lx %s\n", err, str.String());
	err = unescape_find_directory(&str);
	printf("error 0x%08lx %s\n", err, str.String());
	return 0;
}

status_t
FindRGBColor(BMessage &message, const char *name, int32 index, rgb_color *c)
{
#ifdef B_BEOS_VERSION_DANO
	return message.FindRGBColor(name, index, c);
#else
	const void *data;
	ssize_t len;
	status_t err;
	err = message.FindData(name, B_RGB_COLOR_TYPE, index, &data, &len);
	if (err < B_OK)
		return err;
	if (len > (ssize_t)sizeof(*c))
		return E2BIG;
	// Hack
	memcpy((void *)c, data, len);
	return B_OK;
#endif
}


status_t
AddRGBColor(BMessage &message, const char *name, rgb_color a_color, type_code type)
{
#ifdef B_BEOS_VERSION_DANO
	return message.AddRGBColor(name, a_color, type);
#else
	return message.AddData(name, type, &a_color, sizeof(a_color));
#endif
}


status_t
FindFont(BMessage &message, const char *name, int32 index, BFont *f)
{
#ifdef B_BEOS_VERSION_DANO
	return message.FindFlat(name, index, f);
#else
	const void *data;
	ssize_t len;
	status_t err = message.FindData(name, 'FONt', index, &data, &len);
#define DERR(e) { PRINT(("%s: err: %s\n", __FUNCTION__, strerror(e))); }
	if (err < B_OK)
		return err;
	if (len > (ssize_t)sizeof(*f))
		return E2BIG;
	// Hack: only Dano has BFont : public BFlattenable
	memcpy((void *)f, data, len);
	return B_OK;
#endif
}


status_t
AddFont(BMessage &message, const char *name, BFont *f, int32 count)
{
#ifdef B_BEOS_VERSION_DANO
	return message.AddFlat(name, f, count);
#else
	return message.AddData(name, 'FONt', (void *)&f, sizeof(f), true, count);
#endif
}

