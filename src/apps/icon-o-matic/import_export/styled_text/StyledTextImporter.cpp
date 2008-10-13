/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */
#include "StyledTextImporter.h"

#include <new>
#include <stdint.h>
#include <stdio.h>

#include <Archivable.h>
#include <ByteOrder.h>
#include <DataIO.h>
#include <File.h>
#include <List.h>
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

//#define CALLED() printf("%s()\n", __FUNCTION__);
#define CALLED() do {} while (0)

using std::nothrow;

class ShapeIterator : public BShapeIterator {
 public:
	ShapeIterator(Icon *icon, Shape *to, BPoint offset);
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
};

ShapeIterator::ShapeIterator(Icon *icon, Shape *to, BPoint offset)
{
	CALLED();
	fIcon = icon;
	fShape = to;
	fPath = NULL;
	fOffset = offset;
}

status_t
ShapeIterator::IterateMoveTo(BPoint *point)
{
	CALLED();
	if (fPath)
		NextPath();
	if (!CurrentPath())
		return B_ERROR;
	fPath->AddPoint(fOffset + *point);
	return B_OK;
}

status_t
ShapeIterator::IterateLineTo(int32 lineCount, BPoint *linePts)
{
	CALLED();
	if (!CurrentPath())
		return B_ERROR;
	while (lineCount--)
		fPath->AddPoint(fOffset + *linePts++);
	return B_OK;
}

status_t
ShapeIterator::IterateBezierTo(int32 bezierCount, BPoint *bezierPts)
{
	CALLED();
	if (!CurrentPath())
		return B_ERROR;
	while (bezierCount--) {
		fPath->AddPoint(fOffset + *bezierPts++, 
			fOffset + *bezierPts++, fOffset + *bezierPts++, false);
	}
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
	return B_OK;
}

VectorPath *
ShapeIterator::CurrentPath()
{
	CALLED();
	if (fPath)
		return fPath;
	fPath = new (nothrow) VectorPath();
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
	if (size > UINT32_MAX - 1)
		return E2BIG;
	
	BMallocIO mio;
	mio.SetSize((size_t)size + 1);
	memset((void *)mio.Buffer(), 0, (size_t)size + 1);

	// TODO: read runs

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

	StyleContainer* styles = icon->Styles();
	// import run colors as styles
	if (runs) {
		fStyleMap = new struct style_map[runs->count];
		for (int32 i = 0; runs && i < runs->count; i++) {
			_AddStyle(icon, &runs->runs[i]);
		}
	}

#if 1
	int32 currentRun = 0;
	text_run *run = NULL;
	if (runs)
		run = &runs->runs[0];
	BString str(text);
	int32 len = str.Length();
	int32 chars = str.CountChars();
	BPoint offset(0,0);

	for (int32 i = 0, c = 0; i < len && c < chars; c++) {
		// make sure we are still on the (good) run
		while (run && currentRun < runs->count - 1 && 
			i >= runs->runs[currentRun + 1].offset) {
			printf("switching to run %d\n", currentRun);
			run = &runs->runs[++currentRun];
		}

		BShape glyph;
		BShape *glyphs[1] = { &glyph };
		BFont font(be_plain_font);
		if (run)
			font = run->font;

		font.GetGlyphShapes((str.String() + i), 1, glyphs);
		if (glyph.Bounds().IsValid()) {
			offset.x += glyph.Bounds().Width();
			Shape* shape = new (nothrow) Shape(NULL);
			if (!shape || !icon->Shapes()->AddShape(shape)) {
				delete shape;
				return B_NO_MEMORY;
			}
			for (int j = 0; run && j < fStyleCount; j++) {
				if (fStyleMap[j].run == run) {
					shape->SetStyle(fStyleMap[j].style);
					break;
				}
			}
			ShapeIterator iterator(icon, shape, offset);
			if (iterator.Iterate(&glyph) < B_OK)
				return B_ERROR;

		}

		// skip the rest of UTF-8 char bytes
		for (i++; i < len && str[i] & 0x80; i++);
	}
#endif
	delete[] fStyleMap;

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
	Style* style = new (nothrow) Style(color);
	if (!style) {
		delete style;
		return B_NO_MEMORY;
	}

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
