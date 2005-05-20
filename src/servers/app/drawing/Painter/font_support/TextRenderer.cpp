// TextRenderer.cpp

#include <stdio.h>
#include <string.h>

#include <Entry.h>
#include <Path.h>
#include <ServerFont.h>

#include "FontManager.h"

#include "TextRenderer.h"

#define MAXPTSIZE 5000

// constructor
TextRenderer::TextRenderer()
	: fPtSize(12.0),
	  fHinted(true),
	  fAntialias(true),
	  fKerning(true),
	  fOpacity(255),
	  fAdvanceScale(1.0),
	  fLineSpacingScale(1.2)
{
	fFontFilePath[0] = 0;
}

TextRenderer::TextRenderer(BMessage* archive)
	: fPtSize(12.0),
	  fHinted(true),
	  fAntialias(true),
	  fKerning(true),
	  fOpacity(255),
	  fAdvanceScale(1.0),
	  fLineSpacingScale(1.2)
{
	// backwards compatibility crap
	int32 size;
	if (archive->FindInt32("size", &size) < B_OK) {
		if (archive->FindFloat("size", &fPtSize) < B_OK)
			fPtSize = 12.0;
	} else {
		fPtSize = size / 16.0;
	}

	int32 opacity;
	if (archive->FindInt32("gray levels", &opacity) < B_OK)
		fOpacity = 255;
	else
		fOpacity = max_c(0, min_c(255, opacity));

	if (archive->FindFloat("advance scale", &fAdvanceScale) < B_OK)
		fAdvanceScale = 1.0;

	if (archive->FindFloat("line spacing scale", &fLineSpacingScale) < B_OK)
		fLineSpacingScale = 1.2;

	fFontFilePath[0] = 0;
	const char* family;
	const char* style;
	if (archive->FindString("family", &family) >= B_OK
		&& archive->FindString("style", &style) >= B_OK) {
		FontManager* fm = FontManager::Default();
//printf("locking font manager,  %s, %s\n", family, style);
		if (fm->Lock()) {
//printf("setting font: %s, %s\n", family, style);
			SetFontRef(fm->FontFileFor(family, style));
			fm->Unlock();
		}
	} else {
//printf("no family or style!\n");
	}
}

// constructor
TextRenderer::TextRenderer(const TextRenderer& from)
	: fPtSize(12.0),
	  fHinted(true),
	  fAntialias(true),
	  fKerning(true),
	  fOpacity(255),
	  fAdvanceScale(1.0),
	  fLineSpacingScale(1.2)
{
	fFontFilePath[0] = 0;
	SetTo(&from);
}

// destructor
TextRenderer::~TextRenderer()
{
	Unset();
}

// SetTo
void
TextRenderer::SetTo(const TextRenderer* other)
{
	Unset();

	fFontFilePath[0] = 0;

	fPtSize = other->fPtSize;
	fHinted = other->fHinted;
	fAntialias = other->fAntialias;
	fKerning = other->fKerning;
	fOpacity = other->fOpacity;
	fAdvanceScale = other->fAdvanceScale;
	fLineSpacingScale = other->fLineSpacingScale;

	strcpy(fFontFilePath, other->fFontFilePath);

	Update();
}

// Archive
status_t
TextRenderer::Archive(BMessage* into, bool deep) const
{
	status_t status = BArchivable::Archive(into, deep);

	if (status >= B_OK) {
		if (Family())
			status = into->AddString("family", Family());
		else
			fprintf(stderr, "no font family to put into message!\n");
		if (status >= B_OK && Style())
			status = into->AddString("style", Style());
		else
			fprintf(stderr, "no font style to put into message!\n");
		if (status >= B_OK)
			status = into->AddFloat("size", fPtSize);
		if (status >= B_OK)
			status = into->AddInt32("gray levels", fOpacity);
		if (status >= B_OK)
			status = into->AddFloat("advance scale", fAdvanceScale);
		if (status >= B_OK)
			status = into->AddFloat("line spacing scale", fLineSpacingScale);

		// finish
		if (status >= B_OK)
			status = into->AddString("class", "TextRenderer");
	}
	return status;
}

// SetFontRef
bool
TextRenderer::SetFontRef(const entry_ref* ref)
{
	// TODO: Remove this, as we do font loading in FontServer
	/*printf("TextRenderer::SetFontRef(%s)\n", ref ? ref->name : "no ref!");
	if (ref) {
		BPath path(ref);
		if (path.InitCheck() >= B_OK)
			return SetFont(path.Path());
		return true;
	}*/
	return false;
}

// SetFont
bool
TextRenderer::SetFont(const ServerFont &font)
{
	sprintf(fFontFilePath, "%s", font.GetPath());
	return true;
}

// Unset
void
TextRenderer::Unset()
{
	fFontFilePath[0] = 0;
}

// SetFamilyAndStyle
bool
TextRenderer::SetFamilyAndStyle(const char* family, const char* style)
{
	FontManager* fm = FontManager::Default();
	if (fm->Lock()) {
		const entry_ref* fontRef = fm->FontFileFor(family, style);
		fm->Unlock();

		return SetFontRef(fontRef);
	}
	return false;
}

// SetPointSize
void
TextRenderer::SetPointSize(float size)
{
	if (size < 0.0)
		size = 0.0;
	if (size * 16.0 > MAXPTSIZE)
		size = MAXPTSIZE / 16.0;
	if (size != fPtSize) {
		fPtSize = size;
		Update();
	}
}

// PointSize
float
TextRenderer::PointSize() const
{
	return fPtSize;
}

// SetHinting
void
TextRenderer::SetHinting(bool hinting)
{
	fHinted = hinting;
	Update();
}

// SetAntialiasing
void
TextRenderer::SetAntialiasing(bool antialiasing)
{
	fAntialias = antialiasing;
	Update();
}

// SetOpacity
void
TextRenderer::SetOpacity(uint8 opacity)
{
	fOpacity = opacity;
}

// Opacity
uint8
TextRenderer::Opacity() const
{
	return fOpacity;
}

// SetAdvanceScale
void
TextRenderer::SetAdvanceScale(float scale)
{
	fAdvanceScale = scale;
	Update();
}

// AdvanceScale
float
TextRenderer::AdvanceScale() const
{
	return fAdvanceScale;
}

// SetLineSpacingScale
void
TextRenderer::SetLineSpacingScale(float scale)
{
	fLineSpacingScale = scale;
	Update();
}

// LineSpacingScale
float
TextRenderer::LineSpacingScale() const
{
	return fLineSpacingScale;
}

// LineOffset
float
TextRenderer::LineOffset() const
{
	return fLineSpacingScale * fPtSize * 16.0;
}
