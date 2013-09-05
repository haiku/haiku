/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextDocumentTest.h"

#include <stdio.h>

#include <LayoutBuilder.h>
#include <Window.h>

#include "TextDocumentView.h"

TextDocumentTest::TextDocumentTest()
	:
	BApplication("application/x-vnd.Haiku-TextDocumentTest")
{
}


TextDocumentTest::~TextDocumentTest()
{
}


void
TextDocumentTest::ReadyToRun()
{
	BRect frame(50.0, 50.0, 749.0, 549.0);

	BWindow* window = new BWindow(frame, "Text document test",
		B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS);

	TextDocumentView* documentView = new TextDocumentView("text document view");

	BLayoutBuilder::Group<>(window, B_VERTICAL)
		.Add(documentView)
	;

	ParagraphStyle paragraphStyle;
	paragraphStyle.SetJustify(true);
	
	CharacterStyle regularStyle;
	
	CharacterStyle boldStyle(regularStyle);
	boldStyle.SetFont(BFont(be_bold_font));
	
	CharacterStyle bigStyle(regularStyle);
	bigStyle.SetFontSize(24);
	bigStyle.SetForegroundColor(255, 50, 50);

	TextDocumentRef document(new TextDocument(), true);

	Paragraph paragraph(paragraphStyle);
	paragraph.Append(TextSpan("This is a ", regularStyle));
	paragraph.Append(TextSpan("test", bigStyle));
	paragraph.Append(TextSpan(" to see if ", regularStyle));
	paragraph.Append(TextSpan("different", boldStyle));
	paragraph.Append(TextSpan(" character styles already work.", regularStyle));
	document->Append(paragraph);

	paragraphStyle.SetSpacingTop(8.0f);
	paragraphStyle.SetAlignment(ALIGN_CENTER);
	paragraphStyle.SetJustify(false);

	paragraph = Paragraph(paragraphStyle);
	paragraph.Append(TextSpan("Different alignment styles ", regularStyle));
	paragraph.Append(TextSpan("are", boldStyle));
	paragraph.Append(TextSpan(" supported as of now!", regularStyle));
	document->Append(paragraph);

	documentView->SetTextDocument(document);

	window->Show();
}


int
main(int argc, char* argv[])
{
	TextDocumentTest().Run();
	return 0;
}


