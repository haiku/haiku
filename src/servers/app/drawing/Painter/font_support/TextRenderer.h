// TextRenderer.h

#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include "Transformable.h"

struct entry_ref;

class BBitmap;

class TextRenderer : public BArchivable {
 public:
								TextRenderer();
								TextRenderer(BMessage* archive);
								TextRenderer(const TextRenderer& from);
	virtual						~TextRenderer();

	virtual	void				SetTo(const TextRenderer* other);

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;

			bool				SetFontRef(const entry_ref* ref);
	virtual	bool				SetFont(const char* pathToFontFile);
	virtual	void				Unset();

			bool				SetFamilyAndStyle(const char* family,
												  const char* style);
	virtual	const char*			Family() const = 0;
	virtual	const char*			Style() const = 0;
	virtual	const char*			PostScriptName() const = 0;

	virtual	void				SetRotation(float angle);
	virtual	float				Rotation() const;

	virtual	void				SetPointSize(float size);
			float				PointSize() const;

	virtual	void				SetHinting(bool hinting);
			bool				Hinting() const
									{ return fHinted; }

	virtual	void				SetAntialiasing(bool antialiasing);
			bool				Antialiasing() const
									{ return fAntialias; }

	virtual	void				SetOpacity(uint8 opacity);
			uint8				Opacity() const;

	virtual	void				SetAdvanceScale(float scale);
			float				AdvanceScale() const;

	virtual	void				SetLineSpacingScale(float scale);
			float				LineSpacingScale() const;

			float				LineOffset() const;

/*	virtual	void				RenderString(const char* utf8String,
											 BBitmap* bitmap,
											 BRect constrainRect,
											 const Transformable& transform) = 0;

	virtual	BRect				Bounds(const char* utf8String, uint32 length,
									   const Transformable& transform) = 0;*/

	virtual	void				Update() {};

 protected:
			char				fFontFilePath[B_PATH_NAME_LENGTH];
		
			float				fPtSize;
		
			bool				fHinted;			// is glyph hinting active?
			bool				fAntialias;			// is anti-aliasing active?
			bool				fKerning;
			uint8				fOpacity;
			float				fAdvanceScale;
			float				fLineSpacingScale;
};

#endif // FT_TEXT_RENDER_H
