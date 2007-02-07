#ifndef INTERNALEDITORS_H
#define INTERNALEDITORS_H

#include <View.h>
#include <TextControl.h>
#include <Button.h>
#include "Editor.h"

class StringEditView : public BView
{
public:
					StringEditView(const BRect &frame);
					~StringEditView(void);
	
	void			AttachedToWindow(void);
	
	const char *	GetID(void) const { return fIDBox->Text(); }
	void			SetID(const char *idstring) { fIDBox->SetText(idstring); }
	
	const char *	GetName(void) const { return fNameBox->Text(); }
	void			SetName(const char *name) { fNameBox->SetText(name); }
	
	const char *	GetValue(void) const { return fValueBox->Text(); }
	void			SetValue(const char *value) { fValueBox->SetText(value); }
	
	void			EnableID(const bool &value) { fIDBox->SetEnabled(value); }
	bool			IsIDEnabled(void) const { return fIDBox->IsEnabled(); }
	
	float			GetPreferredWidth(void) const;
	float			GetPreferredHeight(void) const;
	
private:
	BTextControl	*fIDBox,
					*fNameBox,
					*fValueBox;
	
	BButton			*fCancel,
					*fOK;
};

class DoubleEditor : public Editor
{
public:
			DoubleEditor(const BRect &frame, ResourceData *data,
						BHandler *owner);
	void	MessageReceived(BMessage *msg);

private:
	StringEditView	*fView;
};

class StringEditor : public Editor
{
public:
			StringEditor(const BRect &frame, ResourceData *data,
						BHandler *owner);
	void	MessageReceived(BMessage *msg);

private:
	StringEditView	*fView;
};

class BitmapView;

class ImageEditor : public Editor
{
public:
			ImageEditor(const BRect &frame, ResourceData *data,
						BHandler *owner);
			~ImageEditor(void);
	void	MessageReceived(BMessage *msg);
	void	FrameResized(float w, float h);
	
private:
	BTextControl	*fIDBox;
	BTextControl	*fNameBox;
	
	BButton			*fOK,
					*fCancel;
	
	BBitmap			*fImage;
	BitmapView		*fImageView;
};

#endif
