/*
 * Copyright 2008-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */

#include "StyledTextImporter.h"

#include <new>
#include <stdint.h>
#include <stdio.h>

#include <Alert.h>
#include <Archivable.h>
#include <ByteOrder.h>
#include <Catalog.h>
#include <DataIO.h>
#include <File.h>
#include <List.h>
#include <Locale.h>
#include <Message.h>
#include <NodeInfo.h>
#include <Shape.h>
#include <String.h>
#include <TextView.h>

#include "Defines.h"
#include "Icon.h"
#include "PathContainer.h"
#include "Shape.h"
#include "Style.h"
#include "StyleContainer.h"
#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-StyledTextImport"
//#define CALLED() printf("%s()\n", __FUNCTION__);
#define CALLED() do {} while (0)


using std::nothrow;

class ShapeIterator : public BShapeIterator {
 public:
	ShapeIterator(Icon *icon, Shape *to, BPoint offset, const char *name);
	~ShapeIterator() {};

	virtual	status_t	IterateMoveTo(BPoint *point);
	virtual	status_t	IterateLineTo(int32 lineCount, BPoint *linePts);
	virtual	status_t	IterateBezierTo(int32 bezierCount, BPoint *bezierPts);
	virtual	status_t	IterateClose();

 private:
	VectorPath				*CurrentPath();
	void					NextPath();

	Icon *fIcon;
	Shape *fShape;
	VectorPath *fPath;
	BPoint fOffset;
	const char *fName;
	BPoint fLastPoint;
	bool fHasLastPoint;
};

ShapeIterator::ShapeIterator(Icon *icon, Shape *to, BPoint offset,
	const char *name)
{
	CALLED();
	fIcon = icon;
	fShape = to;
	fPath = NULL;
	fOffset = offset;
	fName = name;
	fLastPoint = offset;
	fHasLastPoint = false;
}

status_t
ShapeIterator::IterateMoveTo(BPoint *point)
{
	CALLED();
	if (fPath)
		NextPath();
	if (!CurrentPath())
		return B_ERROR;
	//fPath->AddPoint(fOffset + *point);
	fLastPoint = fOffset + *point;
	fHasLastPoint = true;
	return B_OK;
}

status_t
ShapeIterator::IterateLineTo(int32 lineCount, BPoint *linePts)
{
	CALLED();
	if (!CurrentPath())
		return B_ERROR;
	while (lineCount--) {
		fPath->AddPoint(fOffset + *linePts);
		fLastPoint = fOffset + *linePts;
		fHasLastPoint = true;
		linePts++;
	}
	return B_OK;
}

status_t
ShapeIterator::IterateBezierTo(int32 bezierCount, BPoint *bezierPts)
{
	CALLED();
	if (!CurrentPath())
		return B_ERROR;
	BPoint start(bezierPts[0]);
	if (fHasLastPoint)
		start = fLastPoint;
	while (bezierCount--) {
		fPath->AddPoint(fOffset + bezierPts[0],
			fLastPoint, fOffset + bezierPts[1], true);
		fLastPoint = fOffset + bezierPts[2];
		bezierPts += 3;
	}
	fPath->AddPoint(fLastPoint);
	fHasLastPoint = true;
	return B_OK;
}

status_t
ShapeIterator::IterateClose()
{
	CALLED();
	if (!CurrentPath())
		return B_ERROR;
	fPath->SetClosed(true);
	NextPath();
	fHasLastPoint = false;
	return B_OK;
}

VectorPath *
ShapeIterator::CurrentPath()
{
	CALLED();
	if (fPath)
		return fPath;
	fPath = new (nothrow) VectorPath();
	fPath->SetName(fName);
	return fPath;
}

void
ShapeIterator::NextPath()
{
	CALLED();
	if (fPath) {
		fIcon->Paths()->AddPath(fPath);
		fShape->Paths()->AddPath(fPath);
	}
	fPath = NULL;
}


// #pragma mark -

// constructor
StyledTextImporter::StyledTextImporter()
	: Importer(),
	  fStyleMap(NULL),
	  fStyleCount(0)
{
	CALLED();
}

// destructor
StyledTextImporter::~StyledTextImporter()
{
	CALLED();
}

// Import
status_t
StyledTextImporter::Import(Icon* icon, BMessage* clipping)
{
	CALLED();
	const char *text;
	int32 textLength;

	if (clipping == NULL)
		return ENOENT;
	if (clipping->FindData("text/plain",
		B_MIME_TYPE, (const void **)&text, &textLength) == B_OK) {
		text_run_array *runs = NULL;
		int32 runsLength;
		if (clipping->FindData("application/x-vnd.Be-text_run_array",
			B_MIME_TYPE, (const void **)&runs, &runsLength) < B_OK)
			runs = NULL;
		BString str(text, textLength);
		return _Import(icon, str.String(), runs);
	}
	return ENOENT;
}

// Import
status_t
StyledTextImporter::Import(Icon* icon, const entry_ref* ref)
{
	CALLED();
	status_t err;
	BFile file(ref, B_READ_ONLY);
	err = file.InitCheck();
	if (err < B_OK)
		return err;

	BNodeInfo info(&file);
	char mime[B_MIME_TYPE_LENGTH];
	err = info.GetType(mime);
	if (err < B_OK)
		return err;

	if (strncmp(mime, "text/plain", B_MIME_TYPE_LENGTH))
		return EINVAL;

	off_t size;
	err = file.GetSize(&size);
	if (err < B_OK)
		return err;
	if (size > 1 * 1024 * 1024) // Don't load files that big
		return E2BIG;

	BMallocIO mio;
	mio.SetSize((size_t)size + 1);
	memset((void *)mio.Buffer(), 0, (size_t)size + 1);

	// TODO: read runs from attribute

	return _Import(icon, (const char *)mio.Buffer(), NULL);
}

// _Import
status_t
StyledTextImporter::_Import(Icon* icon, const char *text, text_run_array *runs)
{
	CALLED();
	status_t ret = Init(icon);
	if (ret < B_OK) {
		printf("StyledTextImporter::Import() - "
			   "Init() error: %s\n", strerror(ret));
		return ret;
	}

	BString str(text);
	if (str.Length() > 50) {
		BAlert* alert = new BAlert(B_TRANSLATE("too big"),
			B_TRANSLATE("The text you are trying to import is quite long,"
				"are you sure?"),
			B_TRANSLATE("Yes"), B_TRANSLATE("No"), NULL, 
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (alert->Go())
			return B_CANCELED;
	}

	// import run colors as styles
	if (runs) {
		delete[] fStyleMap;
		fStyleMap = new struct style_map[runs->count];
		for (int32 i = 0; runs && i < runs->count; i++) {
			_AddStyle(icon, &runs->runs[i]);
		}
	}

	int32 currentRun = 0;
	text_run *run = NULL;
	if (runs)
		run = &runs->runs[0];
	int32 len = str.Length();
	int32 chars = str.CountChars();
	BPoint origin(0,0);
	BPoint offset(origin);

	for (int32 i = 0, c = 0; i < len && c < chars; c++) {
		// make sure we are still on the (good) run
		while (run && currentRun < runs->count - 1 &&
			i >= runs->runs[currentRun + 1].offset) {
			run = &runs->runs[++currentRun];
			//printf("switching to run %d\n", currentRun);
		}

		int charLen;
		for (charLen = 1; str.ByteAt(i + charLen) & 0x80; charLen++);

		BShape glyph;
		BShape *glyphs[1] = { &glyph };
		BFont font(be_plain_font);
		if (run)
			font = run->font;

		// first char
		if (offset == BPoint(0,0)) {
			font_height height;
			font.GetHeight(&height);
			origin.y += height.ascent;
			offset = origin;
		}
		// LF
		if (str[i] == '\n') {
			// XXX: should take the MAX() for the line
			// XXX: should use descent + leading from previous line
			font_height height;
			font.GetHeight(&height);
			origin.y += height.ascent + height.descent + height.leading;
			offset = origin;
			i++;
			continue;
		}

		float charWidth;
		charWidth = font.StringWidth(str.String() + i, charLen);
		//printf("StringWidth( %d) = %f\n", charLen, charWidth);
		BString glyphName(str.String() + i, charLen);
		glyphName.Prepend("Glyph (");
		glyphName.Append(")");

		font.GetGlyphShapes((str.String() + i), 1, glyphs);
		if (glyph.Bounds().IsValid()) {
			//offset.x += glyph.Bounds().Width();
			offset.x += charWidth;
			Shape* shape = new (nothrow) Shape(NULL);
			if (shape == NULL)
				return B_NO_MEMORY;
			shape->SetName(glyphName.String());
			if (!icon->Shapes()->AddShape(shape)) {
				delete shape;
				return B_NO_MEMORY;
			}
			for (int j = 0; run && j < fStyleCount; j++) {
				if (fStyleMap[j].run == run) {
					shape->SetStyle(fStyleMap[j].style);
					break;
				}
			}
			ShapeIterator iterator(icon, shape, offset, glyphName.String());
			if (iterator.Iterate(&glyph) < B_OK)
				return B_ERROR;

		}

		// skip the rest of UTF-8 char bytes
		for (i++; i < len && str[i] & 0x80; i++);
	}

	delete[] fStyleMap;
	fStyleMap = NULL;

	return B_OK;
}

// #pragma mark -

// _AddStyle
status_t
StyledTextImporter::_AddStyle(Icon *icon, text_run *run)
{
	CALLED();
	if (!run)
		return EINVAL;
	rgb_color color = run->color;
	Style* style = new(std::nothrow) Style(color);
	if (style == NULL)
		return B_NO_MEMORY;
	char name[30];
	sprintf(name, B_TRANSLATE("Color (#%02x%02x%02x)"),
		color.red, color.green, color.blue);
	style->SetName(name);

	bool found = false;
	for (int i = 0; i < fStyleCount; i++) {
		if (*style == *(fStyleMap[i].style)) {
			delete style;
			style = fStyleMap[i].style;
			found = true;
			break;
		}
	}

	if (!found && !icon->Styles()->AddStyle(style)) {
		delete style;
		return B_NO_MEMORY;
	}

	fStyleMap[fStyleCount].run = run;
	fStyleMap[fStyleCount].style = style;
	fStyleCount++;

	return B_OK;
}

// _AddPaths
status_t
StyledTextImporter::_AddPaths(Icon *icon, BShape *shape)
{
	CALLED();
	return B_ERROR;
}

// _AddShape
status_t
StyledTextImporter::_AddShape(Icon *icon, BShape *shape, text_run *run)
{
	CALLED();
	return B_ERROR;
}
