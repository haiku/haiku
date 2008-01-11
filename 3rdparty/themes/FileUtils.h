#ifndef _FILE_UTILS_H
#define _FILE_UTILS_H

#include <Entry.h>
#include <Font.h>

// find the font file corresponding to family/style
extern status_t find_font_file(entry_ref *to, font_family family, font_style style, float size =-1);

// replace part of paths by symbolic strings "${B_FOO_DIRECTORY}"
extern status_t escape_find_directory(BString *dir);
extern status_t unescape_find_directory(BString *dir);

// copy a file including its attributes
extern status_t copy_file(entry_ref *ref, const char *to);

#endif /* _FILE_UTILS_H */
