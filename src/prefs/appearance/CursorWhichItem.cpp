#include "CursorWhichItem.h"
#include <stdio.h>

CursorWhichItem::CursorWhichItem(cursor_which which)
 : BStringItem(NULL,0,false)
{
	SetAttribute(which);
}

CursorWhichItem::~CursorWhichItem(void)
{
	// Empty, but exists for just-in-case
}

void CursorWhichItem::SetAttribute(cursor_which which)
{
	switch(which)
	{
		// cases not existing in R5 which exist in OpenBeOS
		case B_CURSOR_DEFAULT:
		{
			attribute=which;
			SetText("Default");
			break;
		}
		case B_CURSOR_TEXT:
		{
			attribute=which;
			SetText("Text");
			break;
		}
		case B_CURSOR_MOVE:
		{
			attribute=which;
			SetText("Move");
			break;
		}
		case B_CURSOR_DRAG:
		{
			attribute=which;
			SetText("Drag");
			break;
		}
		case B_CURSOR_RESIZE:
		{
			attribute=which;
			SetText("Resize");
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			attribute=which;
			SetText("Diagonal Resize NW-SE");
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			attribute=which;
			SetText("Diagonal Resize NE-SW");
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			attribute=which;
			SetText("Vertical Resize");
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			attribute=which;
			SetText("Horizontal Resize");
			break;
		}
		case B_CURSOR_OTHER:
		{
			attribute=which;
			SetText("Other");
			break;
		}
		case B_CURSOR_INVALID:
		{
			break;
		}
		default:
		{
			printf("unknown code '%c%c%c%c'\n",(char)((which & 0xFF000000) >>  24),
				(char)((which & 0x00FF0000) >>  16),
				(char)((which & 0x0000FF00) >>  8),
				(char)((which & 0x000000FF)) );
			break;
		}
	}
}

cursor_which CursorWhichItem::GetAttribute(void)
{
	return attribute;
}

