/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextDocumentTest.h"

#include <math.h>
#include <stdio.h>

#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <Window.h>

#include "MarkupParser.h"
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

	BScrollView* scrollView = new BScrollView("text scroll view", documentView,
		false, true, B_NO_BORDER);

	BLayoutBuilder::Group<>(window, B_VERTICAL)
		.Add(scrollView)
	;

	CharacterStyle regularStyle;

	float fontSize = regularStyle.Font().Size();

	ParagraphStyle paragraphStyle;
	paragraphStyle.SetJustify(true);
	paragraphStyle.SetSpacingTop(ceilf(fontSize * 0.6f));
	paragraphStyle.SetLineSpacing(ceilf(fontSize * 0.2f));

//	CharacterStyle boldStyle(regularStyle);
//	boldStyle.SetBold(true);
//
//	CharacterStyle italicStyle(regularStyle);
//	italicStyle.SetItalic(true);
//
//	CharacterStyle italicAndBoldStyle(boldStyle);
//	italicAndBoldStyle.SetItalic(true);
//
//	CharacterStyle bigStyle(regularStyle);
//	bigStyle.SetFontSize(24);
//	bigStyle.SetForegroundColor(255, 50, 50);
//
//	TextDocumentRef document(new TextDocument(), true);
//
//	Paragraph paragraph(paragraphStyle);
//	paragraph.Append(TextSpan("This is a", regularStyle));
//	paragraph.Append(TextSpan(" test ", bigStyle));
//	paragraph.Append(TextSpan("to see if ", regularStyle));
//	paragraph.Append(TextSpan("different", boldStyle));
//	paragraph.Append(TextSpan(" character styles already work.", regularStyle));
//	document->Append(paragraph);
//
//	paragraphStyle.SetSpacingTop(8.0f);
//	paragraphStyle.SetAlignment(ALIGN_CENTER);
//	paragraphStyle.SetJustify(false);
//
//	paragraph = Paragraph(paragraphStyle);
//	paragraph.Append(TextSpan("Different alignment styles ", regularStyle));
//	paragraph.Append(TextSpan("are", boldStyle));
//	paragraph.Append(TextSpan(" supported as of now!", regularStyle));
//	document->Append(paragraph);
//
//	// Test a bullet list
//	paragraphStyle.SetSpacingTop(8.0f);
//	paragraphStyle.SetAlignment(ALIGN_LEFT);
//	paragraphStyle.SetJustify(true);
//	paragraphStyle.SetBullet(Bullet("•", 12.0f));
//	paragraphStyle.SetLineInset(10.0f);
//
//	paragraph = Paragraph(paragraphStyle);
//	paragraph.Append(TextSpan("Even bullet lists are supported.", regularStyle));
//	document->Append(paragraph);
//
//	paragraph = Paragraph(paragraphStyle);
//	paragraph.Append(TextSpan("The wrapping in ", regularStyle));
//	paragraph.Append(TextSpan("this", italicStyle));
//
//	paragraph.Append(TextSpan(" bullet item should look visually "
//		"pleasing. And ", regularStyle));
//	paragraph.Append(TextSpan("why", italicAndBoldStyle));
//	paragraph.Append(TextSpan(" should it not?", regularStyle));
//	document->Append(paragraph);

	MarkupParser parser(regularStyle, paragraphStyle);

	TextDocumentRef document = parser.CreateDocumentFromMarkup(
		"== Text document test ==\n"
		"This is a test to see if '''different''' "
		"character styles already work.\n"
		"Different alignment styles '''are''' supported as of now!\n"
		" * Even bullet lists are supported.\n"
		" * The wrapping in ''this'' bullet item should look visually "
		"pleasing. And ''why'' should it not?\n"
	);

	documentView->SetTextDocument(document);
	documentView->SetTextEditor(TextEditorRef(new TextEditor(), true));
	documentView->MakeFocus();

	window->Show();
}


int
main(int argc, char* argv[])
{
	TextDocumentTest().Run();
	return 0;
}


