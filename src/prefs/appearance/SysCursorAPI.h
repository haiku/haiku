#ifndef SYSCURSOR_API_H_
#define SYSCURSOR_API_H_

#include <SysCursor.h>
#include <Bitmap.h>
#include <Cursor.h>

void set_syscursor(cursor_which which, const BCursor *cursor);
void set_syscursor(cursor_which which, const BBitmap *bitmap);

cursor_which get_syscursor(void);

void setcursor(cursor_which which);

#endif
