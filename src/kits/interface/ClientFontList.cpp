//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku
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
//	File Name:		ClientFontList.cpp
//	Author:			DarkWyrm (bpmagic@columbus.rr.com)
//	Description:	Maintainer object for the list of font families and styles which is
//					kept on the client side.
//------------------------------------------------------------------------------

#include "ClientFontList.h"
#include <File.h>
#include <Message.h>
#include <String.h>
#include <stdio.h>
#include <string.h>
#include <String.h>

#include <PortLink.h>
#include <ServerProtocol.h>
#include <ServerConfig.h>

//#define DEBUG_CLIENT_FONT_LIST
#ifdef DEBUG_CLIENT_FONT_LIST
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


class FontListFamily {
	public:
		FontListFamily(void);
		~FontListFamily(void);

		BString name;
		BList *styles;
		int32 flags;
};


FontListFamily::FontListFamily(void)
{
	styles = new BList(0);
	flags = 0;
}


FontListFamily::~FontListFamily(void)
{
	for (int32 i = styles->CountItems(); i-- > 0; ) {
		delete (BString *)styles->ItemAt(i);
	}

	delete styles;
}


ClientFontList::ClientFontList(void)
{
	STRACE(("ClientFontList()\n"));
	familylist = new BList(0);
	fontlock = create_sem(1, "fontlist_sem");
}


ClientFontList::~ClientFontList(void)
{
	STRACE(("~ClientFontList()\n"));
	
	acquire_sem(fontlock);

	for (int32 i = familylist->CountItems(); i-- > 0; ) {
		delete (font_family *)familylist->ItemAt(i);
	}

	delete familylist;
	delete_sem(fontlock);
}


bool
ClientFontList::Update(bool checkOnly)
{
	STRACE(("ClientFontList::Update(%s) - %s\n", checkOnly ? "true" : "false",
		SERVER_FONT_LIST));

	// Open the font list kept in font list
	acquire_sem(fontlock);

	// ToDo: can't we use BPrivate::AppServerLink here?
	// We're going to ask the server whether the list has changed
	port_id port = find_port(SERVER_PORT_NAME);

	bool needsUpdate = true;
	BPrivate::PortLink link(port);

	if (port >= B_OK) {
		link.StartMessage(AS_QUERY_FONTS_CHANGED);

		int32 code;
		if (link.FlushWithReply(code) == B_OK)
			needsUpdate = code == SERVER_TRUE;
	} else {
		STRACE(("ClientFontList::Update(): Couldn't find app_server port\n"));
	}

	if (checkOnly || !needsUpdate) {
		release_sem(fontlock);
		return needsUpdate;
	}

	BFile file(SERVER_FONT_LIST,B_READ_ONLY);
	BMessage fontMessage;

	if (file.InitCheck() == B_OK
		&& fontMessage.Unflatten(&file) == B_OK) {
#ifdef DEBUG_CLIENT_FONT_LIST
		printf("Font message contents:\n");
		fontmsg.PrintToStream();
#endif

		// Empty the font list
		FontListFamily *family;
		while ((family = (FontListFamily *)familylist->RemoveItem(0L)) != NULL) {
			STRACE(("Removing %s from list\n", family->name.String()));
			delete family;
			
		}
		STRACE(("\n"));

		// Repopulate with new listings
		int32 familyIndex = 0;
		BMessage familyMessage;
		while (fontMessage.FindMessage("family", familyIndex++, &familyMessage) == B_OK) {
			family = new FontListFamily();
			familylist->AddItem(family);
			familyMessage.FindString("name", &family->name);

			STRACE(("Adding %s to list\n", family->name.String()));

			int32 styleIndex = 0;

			// populate family with styles
			BString string;
			while (familyMessage.FindString("styles", styleIndex++, &string) == B_OK) {
				STRACE(("\tAdding %s\n", string.String()));
				family->styles->AddItem(new BString(string));
			}

			if (familyMessage.FindBool("tuned")) {
				STRACE(("Family %s has tuned fonts\n", family->name.String()));
				family->flags |= B_HAS_TUNED_FONT;
			}

			if (familyMessage.FindBool("fixed")) {
				STRACE(("Family %s is fixed-width\n", family->name.String()));
				family->flags |= B_IS_FIXED;
			}
			familyMessage.MakeEmpty();
		}

		link.StartMessage(AS_UPDATED_CLIENT_FONTLIST);
		link.Flush();
	}

	release_sem(fontlock);
	return false;
}


int32
ClientFontList::CountFamilies(void)
{
	STRACE(("ClientFontList::CountFamilies\n"));
	acquire_sem(fontlock);
	int32 count = familylist->CountItems();
	release_sem(fontlock);
	return count;
}


status_t
ClientFontList::GetFamily(int32 index, font_family *name, uint32 *flags)
{
	STRACE(("ClientFontList::GetFamily(%ld)\n",index));
	if (!name) {
		STRACE(("ClientFontList::GetFamily: NULL font_family parameter\n"));
		return B_BAD_VALUE;
	}

	acquire_sem(fontlock);

	FontListFamily *family = (FontListFamily *)familylist->ItemAt(index);
	if (family == NULL) {
		STRACE(("ClientFontList::GetFamily: index not found\n"));
		return B_ERROR;
	}

	// ToDo: respect size of "name"
	strcpy(*name, family->name.String());

	release_sem(fontlock);
	return B_OK;
}


int32
ClientFontList::CountStyles(font_family f)
{
	acquire_sem(fontlock);

	FontListFamily *family = NULL;
	int32 i, count = familylist->CountItems();
	bool found = false;

	for (i = 0; i < count; i++) {
		family = (FontListFamily *)familylist->ItemAt(i);
		if (!family)
			continue;

		if (family->name.ICompare(f) == 0) {
			found = true;
			break;
		}
	}

	count = found ? family->styles->CountItems() : 0;

	release_sem(fontlock);
	return count;
}


status_t
ClientFontList::GetStyle(font_family fontFamily, int32 index, font_style *name,
	uint32 *flags, uint16 *face)
{
	if (!name || !*name || !fontFamily)
		return B_ERROR;

	acquire_sem(fontlock);

	FontListFamily *family = NULL;
	BString *style;
	int32 i, count = familylist->CountItems();
	bool found = false;

	for (i = 0; i < count; i++) {
		family = (FontListFamily *)familylist->ItemAt(i);
		if (!family)
			continue;

		if (family->name.ICompare(fontFamily) == 0) {
			found = true;
			break;
		}
	}

	if (!found) {
		release_sem(fontlock);
		return B_ERROR;
	}

	style = (BString *)family->styles->ItemAt(index);
	if (!style) {
		release_sem(fontlock);
		return B_ERROR;
	}

	strcpy(*name, style->String());

	if (flags)
		*flags = family->flags;

	if (face) {
		if (!style->ICompare("Roman")
			|| !style->ICompare("Regular")
			|| !style->ICompare("Normal")
			|| !style->ICompare("Light")
			|| !style->ICompare("Medium")
			|| !style->ICompare("Plain")) {
			*face |= B_REGULAR_FACE;
			STRACE(("GetStyle: %s Roman face\n", style->String()));
		} else if (!style->ICompare("Bold")) {
			*face |= B_BOLD_FACE;
				STRACE(("GetStyle: %s Bold face\n"));
		} else if (!style->ICompare("Italic")) {
			*face|=B_ITALIC_FACE;
			STRACE(("GetStyle: %s Italic face\n"));
		} else if (!style->ICompare("Bold Italic")) {
			*face|=B_ITALIC_FACE | B_BOLD_FACE;
			STRACE(("GetStyle: %s Bold Italic face\n"));
		} else {
			STRACE(("GetStyle: %s Unknown face %s\n", style->String()));
		}
	}

	release_sem(fontlock);
	return B_OK;
}

