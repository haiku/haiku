/*

PDF Writer printer driver.

Copyright (c) 2002 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <InterfaceKit.h>
#include <LayoutBuilder.h>
#include <SupportKit.h>
#include "BlockingWindow.h"
#include "FontsWindow.h"
#include "Fonts.h"


class CJKFontItem : public BStringItem
{
	font_encoding fEncoding;
	bool          fActive;
	
public:
	CJKFontItem(const char* label, font_encoding encoding, bool active = true);
	void SetActive(bool active) { fActive = active; }
	void DrawItem(BView* owner, BRect rect, bool drawEverything);	
	
	bool          Active() const   { return fActive; }
	font_encoding Encoding() const { return fEncoding; }
};

CJKFontItem::CJKFontItem(const char* label, font_encoding encoding, bool active)
	: BStringItem(label)
	, fEncoding(encoding)
	, fActive(active)
{
}

void 
CJKFontItem::DrawItem(BView* owner, BRect rect, bool drawEverything)
{
	BStringItem::DrawItem(owner, rect, drawEverything);
	if (!fActive) {
		owner->SetHighColor(0, 0, 0, 0);
		float y = (rect.top + rect.bottom) / 2;
		owner->StrokeLine(BPoint(rect.left, y), BPoint(rect.right, y));
	}
}

static const char* FontName(font_encoding enc) {
	switch(enc) {
		case japanese_encoding:     return "Japanese";
		case chinese_cns1_encoding: return "Traditional Chinese";
		case chinese_gb1_encoding:  return "Simplified Chinese";
		case korean_encoding:       return "Korean"; 
		default:                    return "invalid font encoding!";
	}
}


// #pragma mark -- of DragListView


DragListView::DragListView(BRect frame, const char *name,
		list_view_type type,
		uint32 resizingMode, uint32 flags)
	: BListView(frame, name, type, resizingMode, flags)
{
}


bool
DragListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	BMessage m;
	DragMessage(&m, ItemFrame(index), this);
	return true;
}


/**
 * Constuctor
 *
 * @param 
 * @return
 */
FontsWindow::FontsWindow(Fonts *fonts)
	:	HWindow(BRect(0,0,400,220), "Fonts", B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
			B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	// ---- Ok, build a default page setup user interface

	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	
	fFonts = fonts;
	
	BTabView* tabView = new BTabView("tabview", B_WIDTH_FROM_LABEL);

	// --- Embedding tab ---
	
	// add a *dialog* background
	BView* embeddingPanel = new BView("Embedding", B_WILL_DRAW
		| B_FRAME_EVENTS | B_NAVIGABLE_JUMP);
	embeddingPanel->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// add font list
#if USE_CLV
	fList = new BColumnListView(BRect(),
		"fonts_list", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE_JUMP);
	embeddingPanel->AddChild(fList);
#else
	fList = new BListView("fonts_list", B_MULTIPLE_SELECTION_LIST);
	BScrollView* embeddingScrollView = new BScrollView("scroll_list",
		fList, 0, false, true);
	fList->SetSelectionMessage(new BMessage(SELECTION_MSG));
#endif
	FillFontList();

	// add a "Embed" button, and make it default
	fEmbedButton = new BButton("embed", "Embed", new BMessage(EMBED_MSG));
	fEmbedButton->SetEnabled(false);
	fEmbedButton->MakeDefault(true);


	// add a "Substitute" button	
	fSubstButton = new BButton("substitute", "Substitute",
		new BMessage(SUBST_MSG));
	fSubstButton->SetEnabled(false);
	embeddingScrollView->SetExplicitMinSize(BSize(200, 200));

	BLayoutBuilder::Group<>(embeddingPanel, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(embeddingScrollView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fEmbedButton)
			.Add(fSubstButton);

	tabView->AddTab(embeddingPanel);

	// --- CJK tab ---

	// add a *dialog* background
	BView* cjkPanel = new BView("CJK", B_WILL_DRAW | B_FRAME_EVENTS
		| B_NAVIGABLE_JUMP);
	cjkPanel->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BListView* list = 
		new DragListView(BRect(),
		"cjk", B_MULTIPLE_SELECTION_LIST);
	BScrollView* cjkScrollView = new BScrollView("cjk_scroll_list",
		list, 0, false, true);

	font_encoding enc; bool active;
	for (int i = 0; fFonts->GetCJKOrder(i, enc, active); i++) {
		list->AddItem(new CJKFontItem(FontName(enc), enc, active));
	}
	list->SetSelectionMessage(new BMessage(CJK_SELECTION_MSG));
	fCJKList = list;

	// add a "Embed" button
	fEnableButton = new BButton("enable", "Enable", new BMessage(ENABLE_MSG));
	fEnableButton->SetEnabled(false);

	// add a "Disable" button	
	fDisableButton = new BButton("disable", "Disable",
		new BMessage(DISABLE_MSG));
	fDisableButton->SetEnabled(false);

	// add a "Down" button	
	fDownButton = new BButton("down", "Down", new BMessage(DOWN_MSG));
	fDownButton->SetEnabled(false);

	// add a "Up" button	
	fUpButton = new BButton("up", "Up", new BMessage(UP_MSG));
	fUpButton->SetEnabled(false);

	BLayoutBuilder::Group<>(cjkPanel, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(cjkScrollView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fEnableButton)
			.Add(fDisableButton)
			.Add(fDownButton)
			.Add(fUpButton);

	tabView->AddTab(cjkPanel);

	// add the tabView to the window	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(tabView);

	MoveTo(320,320);
}


// --------------------------------------------------
FontsWindow::~FontsWindow()
{
}


// --------------------------------------------------
bool 
FontsWindow::QuitRequested()
{
	return true;
}


// --------------------------------------------------
void 
FontsWindow::Quit()
{
	inherited::Quit();
}


class ListIterator 
{
	virtual bool DoItem(BListItem* item) = 0;
public:
	static bool DoIt(BListItem* item, void* data);
	virtual ~ListIterator() {}
};


bool
ListIterator::DoIt(BListItem* item, void* data)
{
	if (item->IsSelected()) {
		ListIterator* e = (ListIterator*)data;
		return e->DoItem(item);
	}
	return false;
}

class CountItems : public ListIterator
{	
	BListView* fList;
	Fonts*     fFonts;
	int        fNumEmbed;
	int        fNumSubst;
	
	bool DoItem(BListItem* item);	

public:
	CountItems(BListView* list, Fonts* fonts);
	int GetNumEmbed() { return fNumEmbed; }
	int GetNumSubst() { return fNumSubst; }
};

CountItems::CountItems(BListView* list, Fonts* fonts) 
	: fList(list)
	, fFonts(fonts)
	, fNumEmbed(0)
	, fNumSubst(0) 
{ }

bool CountItems::DoItem(BListItem* item) 
{
	int32 i = fList->IndexOf(item);
	if (0 <= i && i < fFonts->Length()) {
		if (fFonts->At(i)->Embed()) {
			fNumEmbed ++;
		} else {
			fNumSubst ++;
		}
	}
	return false;
}

class EmbedFont : public ListIterator
{
	FontsWindow* fWindow;
	BListView*   fList;
	Fonts*       fFonts;
	bool         fEmbed;
	
	bool DoItem(BListItem* item);
public:
	EmbedFont(FontsWindow* window, BListView* list, Fonts* fonts, bool embed);
};

EmbedFont::EmbedFont(FontsWindow* window, BListView* list, Fonts* fonts, bool embed)
	: fWindow(window)
	, fList(list)
	, fFonts(fonts)
	, fEmbed(embed)
{
}

bool
EmbedFont::DoItem(BListItem* item)
{
	int32 i = fList->IndexOf(item);
	if (0 <= i && i < fFonts->Length()) {
		fFonts->At(i)->SetEmbed(fEmbed);
		fWindow->SetItemText((BStringItem*)item, fFonts->At(i));
	}
	return false;
}

// move selected item in list one position up or down
void
FontsWindow::MoveItemInList(BListView* list, bool up)
{
	int32 from = list->CurrentSelection(0);
	if (from >= 0) {
		int32 to;
		if (up) {
			if (from == 0) return; // at top of list
			to = from - 1;
		} else {
			if (from == list->CountItems() -1) return; // at bottom of list
			to = from + 1;
		}
		list->SwapItems(from, to);
		
		font_encoding enc;
		bool          active;
		fFonts->GetCJKOrder(from, enc, active);
		
		font_encoding enc1;
		bool          active1;
		fFonts->GetCJKOrder(to, enc1, active1);
		fFonts->SetCJKOrder(to, enc,  active);
		fFonts->SetCJKOrder(from, enc1, active1);
	}
}


void
FontsWindow::EnableItemInList(BListView* list, bool enable)
{
	int32 i = list->CurrentSelection(0);
	if (i >= 0) {
		BListItem* item0 = list->ItemAt(i);
		CJKFontItem* item = dynamic_cast<CJKFontItem*>(item0);
		if (item) {
			item->SetActive(enable);
			list->Invalidate();
			font_encoding enc;
			bool          active;
			fFonts->GetCJKOrder(i, enc, active);
			fFonts->SetCJKOrder(i, enc, enable);
		}
	}
}

// --------------------------------------------------
void 
FontsWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what){
		case OK_MSG: Quit();
			break;
		
		case CANCEL_MSG: Quit();
			break;

		case EMBED_MSG:
			{
			#if USE_CLV
			#else
			EmbedFont e(this, fList, fFonts, true);
			fList->DoForEach(EmbedFont::DoIt, (void*)&e);
			}
			PostMessage(SELECTION_MSG);
			fList->Invalidate();
			#endif
			break;
			
		case SUBST_MSG:
			{
			#if USE_CLV
			#else
			EmbedFont e(this, fList, fFonts, false);
			fList->DoForEach(EmbedFont::DoIt, (void*)&e);
			}
			PostMessage(SELECTION_MSG);
			fList->Invalidate();
			#endif
			break;

		case SELECTION_MSG:
			{
				CountItems count(fList, fFonts);
				fList->DoForEach(CountItems::DoIt, (void*)&count);
				fEmbedButton->SetEnabled(count.GetNumSubst() > 0);
				fSubstButton->SetEnabled(count.GetNumEmbed() > 0);
			}
			break;

		case UP_MSG:
		case DOWN_MSG:
			MoveItemInList(fCJKList, msg->what == UP_MSG);
			PostMessage(CJK_SELECTION_MSG);
			break;
		
		case ENABLE_MSG:
		case DISABLE_MSG:
			EnableItemInList(fCJKList, msg->what == ENABLE_MSG);
			PostMessage(CJK_SELECTION_MSG);
			break;

		case CJK_SELECTION_MSG:
			{
				int32 i = fCJKList->CurrentSelection(0);
				if (i >= 0) {
					BListItem* item0 = fCJKList->ItemAt(i);
					CJKFontItem* item = dynamic_cast<CJKFontItem*>(item0);
					if (item) {
						fUpButton->SetEnabled(i != 0);
						fDownButton->SetEnabled(i != fCJKList->CountItems()-1);
						fEnableButton->SetEnabled(!item->Active());
						fDisableButton->SetEnabled(item->Active());
					}
				}
			}
			break;
	
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void
FontsWindow::SetItemText(BStringItem* i, FontFile* f)
{
	const int64 KB = 1024;
	const int64 MB = KB * KB;
	char buffer[40];
	BString s;
	s << f->Name() << ": ";
	if (f->Type() == true_type_type) s << "TrueType";
	else s << "Type 1";
	s << " (";
	if (f->Size() >= MB) {
		sprintf(buffer, "%d MB", (int)(f->Size()/MB));
	} else if (f->Size() >= KB) {
		sprintf(buffer, "%d KB", (int)(f->Size()/KB));
	} else {
		sprintf(buffer, "%d B", (int)(f->Size()));
	} 
	s << buffer;
	s << ") ";
	s << (f->Embed() ? "embed" : "substitute");
	i->SetText(s.String());
}
			
void
FontsWindow::FillFontList()
{
	const int n = fFonts->Length();
	for (int i = 0; i < n; i++) {
#if USE_CLV
#else
		BStringItem* s;
		FontFile* f = fFonts->At(i);
		fList->AddItem(s = new BStringItem(""));
		SetItemText(s, f);
#endif
	}
}

void
FontsWindow::EmptyFontList()
{
#if USE_CLV
#else
	const int n = fList->CountItems();
	for (int i = 0; i < n; i++) {
		BListItem* item = fList->ItemAt(i);
		delete item;
	}
	fList->MakeEmpty();
#endif
}
