/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FILE_UTILS_H
#define _FILE_UTILS_H

#include <Entry.h>
#include <Font.h>
#include <GraphicsDefs.h>
#include <Message.h>

// find the font file corresponding to family/style
extern status_t find_font_file(entry_ref *to, font_family family, font_style style, float size =-1);

// replace part of paths by symbolic strings "${B_FOO_DIRECTORY}"
extern status_t escape_find_directory(BString *dir);
extern status_t unescape_find_directory(BString *dir);

// copy a file including its attributes
extern status_t copy_file(entry_ref *ref, const char *to);

extern status_t FindRGBColor(BMessage &message, const char *name, int32 index, rgb_color *c);
extern status_t AddRGBColor(BMessage &message, const char *name, rgb_color a_color, type_code type = B_RGB_COLOR_TYPE);

extern status_t FindFont(BMessage &message, const char *name, int32 index, BFont *f);
extern status_t AddFont(BMessage &message, const char *name, BFont *f, int32 count = 1);

#ifdef B_BEOS_VERSION_DANO
#define GET_INFO_NAME_PTR(p) (const char **)(p)
#else
#define GET_INFO_NAME_PTR(p) (p)
#endif


#endif /* _FILE_UTILS_H */
