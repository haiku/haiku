/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood
 */


#include <Bitmap.h>
#include <Font.h>
#include <List.h>
#include <Picture.h>
#include <Screen.h>
#include <ScreenSaver.h>
#include <String.h>
#include <StringView.h>
#include <View.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Double brackets to satisfy a compiler warning
const pattern kCheckered = { { 0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0x33, 0x33 } };


// Get a clever message from fortune
BString *get_message()
{
	BString *result = new BString();
	system("fortune > /tmp/fortune_msg");
	FILE *file = fopen("/tmp/fortune_msg", "r");
	if (file) {
		char buf[512];
		int bytesRead;
		while (!feof(file)) {
			bytesRead = fread(buf, 1, 512, file);
			result->Append(buf, bytesRead);
		}
		fclose(file);
	}
	remove("/tmp/fortune_msg");

	// Just in case
	if (result->Length() <= 0) {
		result->Append("Insert clever anecdote or phrase here!");
	}

	return result;
}


// Inspired by the classic BeOS screensaver, of course.
// Thanks to Jon Watte for writing the original.
class Message : public BScreenSaver
{
	public:
					Message(BMessage *archive, image_id);
					~Message();
		void		Draw(BView *view, int32 frame);
		void		StartConfig(BView *view);
		status_t	StartSaver(BView *view, bool preview);
		
	private:
		BList			*fFontFamilies;
		float			fScaleFactor;
};


BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id image) 
{ 
	return new Message(msg, image);
} 


Message::Message(BMessage *archive, image_id id)
 :	BScreenSaver(archive, id)
{
}


Message::~Message()
{
	if (fFontFamilies) {
		for (int32 i = 0; i < fFontFamilies->CountItems(); i++) {
			if (fFontFamilies->ItemAt(i))
				delete ((font_family) fFontFamilies->ItemAt(i));
		}
		delete fFontFamilies;
	}
}


void 
Message::StartConfig(BView *view) 
{ 
	view->AddChild(new BStringView(BRect(20, 10, 200, 35), "", "Message, by Ryan Leavengood"));
	view->AddChild(new BStringView(BRect(20, 40, 200, 65), "", "Inspired by Jon Watte's Original"));
} 


status_t 
Message::StartSaver(BView *view, bool preview)
{
	// Scale factor is based on the system I developed this on, in
	// other words other factors below depend on this.
	fScaleFactor = view->Bounds().Height() / 1024;

	// Get font families
	int numFamilies = count_font_families();
	fFontFamilies = new BList();
	for (int32 i = 0; i < numFamilies; i++) {
		font_family *family = new font_family[1];
		uint32 flags;
		if (get_font_family(i, family, &flags) == B_OK) {
			// Do not add fixed fonts
			if (!(flags & B_IS_FIXED))
				fFontFamilies->AddItem(*family);
		}
	}

	// Seed the random number generator
	srand((int)system_time());

	// Set tick size to 30,000,000 microseconds = 30 seconds
	SetTickSize(30000000);
	
	return B_OK;
}


void 
Message::Draw(BView *view, int32 frame)
{
	// Double-buffered drawing
	BScreen screen;
	BBitmap buffer(view->Bounds(), screen.ColorSpace(), true);
	BView offscreen(view->Bounds(), NULL, 0, 0);
	buffer.AddChild(&offscreen);
	buffer.Lock();

	// Set up the colors
	rgb_color base_color = {rand() % 25, rand() % 25, rand() % 25};
	offscreen.SetHighColor(base_color); 
	offscreen.SetLowColor(tint_color(base_color, 0.815F));
	offscreen.FillRect(offscreen.Bounds(), kCheckered);
	rgb_color colors[8] = {
		tint_color(base_color, B_LIGHTEN_1_TINT),
		tint_color(base_color, 0.795F),
		tint_color(base_color, 0.851F),
		tint_color(base_color, 0.926F),
		tint_color(base_color, 1.05F),
		tint_color(base_color, B_DARKEN_1_TINT),
		tint_color(base_color, B_DARKEN_2_TINT),
		tint_color(base_color, B_DARKEN_3_TINT),
	};

	// Set the basic font parameters, including random font family
	BFont font;
	offscreen.GetFont(&font);
	font.SetFlags(B_DISABLE_ANTIALIASING);
	font.SetFace(B_BOLD_FACE);
	font.SetFamilyAndStyle((font_family) fFontFamilies->ItemAt(rand() % fFontFamilies->CountItems()), NULL);
	offscreen.SetFont(&font);

	// Get the message
	// TODO: Display the whole message at the bottom of the screen 
	// in a translucent rect
	BString *message = get_message();
	// Replace newlines and tabs with spaces
	message->ReplaceSet("\n\t", ' ');

	int height = (int) offscreen.Bounds().Height();
	int width = (int) offscreen.Bounds().Width();

	// From 8 to 16 iterations
	int32 iterations = (rand() % 9) + 8;
	for (int32 i = 0; i < iterations; i++) {
		// Randomly set font size and shear
		BFont font;
		offscreen.GetFont(&font);
		float fontSize = ((rand() % 320) + 42) * fScaleFactor;
		font.SetSize(fontSize);
		// Set the shear off 90 about 1/2 of the time
		if (rand() % 2 == 1)
			font.SetShear((float) ((rand() % 135) + (rand() % 45)));
		else
			font.SetShear(90.0);
		offscreen.SetFont(&font);

		// Randomly set drawing location
		// TODO: Improve this
		int x = (rand() % width) - (rand() % width/((rand() % 8)+1));
		int y = (rand() % height) - (rand() % height/((rand() % 8)+1));

		// Draw new text
		offscreen.SetHighColor(colors[rand() % 8]);
		int strLength = message->Length();
		// See how wide this string is with the current font
		float strWidth = offscreen.StringWidth(message->String());
		int drawingLength = (int) (strLength * (width / strWidth));
		int start = 0;
		if (drawingLength >= strLength)
			drawingLength = strLength;
		else
			start = rand() % (strLength - drawingLength);
		char *toDraw = new char[drawingLength+1];
		strncpy(toDraw, message->String()+start, drawingLength);
		toDraw[drawingLength] = 0;
		offscreen.DrawString(toDraw, BPoint(x, y));
		delete toDraw;
	}

	offscreen.Sync();
	buffer.Unlock();
	view->DrawBitmap(&buffer);
	buffer.RemoveChild(&offscreen);
}

