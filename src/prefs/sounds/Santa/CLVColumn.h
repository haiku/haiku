//Column list header header file

//*** LICENSE ***
//ColumnListView, its associated classes and source code, and the other components of Santa's Gift Bag are
//being made publicly available and free to use in freeware and shareware products with a price under $25
//(I believe that shareware should be cheap). For overpriced shareware (hehehe) or commercial products,
//please contact me to negotiate a fee for use. After all, I did work hard on this class and invested a lot
//of time into it. That being said, DON'T WORRY I don't want much. It totally depends on the sort of project
//you're working on and how much you expect to make off it. If someone makes money off my work, I'd like to
//get at least a little something.  If any of the components of Santa's Gift Bag are is used in a shareware
//or commercial product, I get a free copy.  The source is made available so that you can improve and extend
//it as you need. In general it is best to customize your ColumnListView through inheritance, so that you
//can take advantage of enhancements and bug fixes as they become available. Feel free to distribute the 
//ColumnListView source, including modified versions, but keep this documentation and license with it.


#ifndef _CLV_COLUMN_H_
#define _CLV_COLUMN_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <SupportDefs.h>
#include <Rect.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES AND CLASS NAME DECLARATIONS
//******************************************************************************************************
class ColumnListView;
class CLVColumn;
class CLVListItem;


//******************************************************************************************************
//**** CONSTANTS
//******************************************************************************************************
//Flags
enum
{
	CLV_SORT_KEYABLE =			0x00000001,		//Can be used as the sorting key
	CLV_NOT_MOVABLE =			0x00000002,		//Column can't be moved by user
	CLV_NOT_RESIZABLE =			0x00000004,		//Column can't be resized by user
	CLV_LOCK_AT_BEGINNING =		0x00000008,		//Movable columns may not be placed or moved by the user
												//into a position before this one
	CLV_LOCK_AT_END =			0x00000010,		//Movable columns may not be placed or moved by the user
												//into a position after this one
	CLV_HIDDEN =				0x00000020,		//This column is hidden initially
	CLV_MERGE_WITH_RIGHT =		0x00000040,		//Merge this column label with the one that follows it.
	CLV_LOCK_WITH_RIGHT =		0x00000080,		//Lock this column to the one that follows it such that
												//if the column to the right is moved by the user, this
												//one will move with it and vice versa
	CLV_EXPANDER =				0x00000100,		//Column contains an expander.  You may only use one
												//expander in a ColumnListView, and an expander may not be
												//added to a non-hierarchal ColumnListView.  It may not
												//have a label.  Its width is automatically set to 20.0.
												//The only flags that affect it are CLV_NOT_MOVABLE,
												//CLV_LOCK_AT_BEGINNING, CLV_NOT_SHOWN and
												//CLV_LOCK_WITH_RIGHT.  The others are set for you:
												//CLV_NOT_RESIZABLE | CLV_MERGE_WITH_RIGHT
	CLV_PUSH_PASS =				0x00000200,		//Causes this column, if pushed by an expander to the
												//left, to pass that push on and also push the next
												//column to the right.
	CLV_HEADER_TRUNCATE =		0x00000400,		//Causes this column label to be tructated with an ellipsis
												//if the column header is narrower than the text it contains.
	CLV_TELL_ITEMS_WIDTH =		0x00000800,		//Causes items in this column to be informed when the column
												//width is changed.  This is necessary for CLVEasyItems.
	CLV_RIGHT_JUSTIFIED =		0x00001000		//Causes the column, when resized, to shift its content,
												//not just the content of subsequent columns.  This does not
												//affect the rendering of content in items in the column,
												//just the area that gets scrolled.
};

enum CLVSortMode
{
	Ascending,
	Descending,
	NoSort
};


//******************************************************************************************************
//**** FUNCTIONS
//******************************************************************************************************
void GetTruncatedString(const char* full_string, char* truncated, float width, int32 truncate_buf_size,
	const BFont* font);


//******************************************************************************************************
//**** ColumnListView CLASS DECLARATION
//******************************************************************************************************
class CLVColumn
{
	public:
		//Constructor and destructor
		CLVColumn(	const char* label,
					float width = 20.0,
					uint32 flags = 0,
					float min_width = 20.0);
		virtual ~CLVColumn();

		//Archival stuff
		/* Not implemented yet
		CLVColumn(BMessage* archive);
		static CLVColumn* Instantiate(BMessage* data);
		virtual	status_t Archive(BMessage* data, bool deep = true) const;
		*/

		//Functions
		float Width() const;
		virtual void SetWidth(float width);		//Can be overridden to detect changes to the column width
												//however since you are probably overriding
												//ColumnListView and dealing with an array of columns
												//anyway, it is probably more useful to override
												//ColumnListView::ColumnWidthChanged to detect changes to
												//column widths
		uint32 Flags() const;
		bool IsShown() const;
		virtual void SetShown(bool shown);
		CLVSortMode SortMode() const;
		virtual void SetSortMode(CLVSortMode mode);
		const char* GetLabel() const;
		ColumnListView* GetParent() const ;
		BView* GetHeaderView() const;
		virtual void DrawColumnHeader(BView* view, BRect header_rect, bool sort_key, bool focus,
			float font_ascent);
			//Can be overridden to implement your own column header drawing, for example if you want to do
			//string truncation.
			//- The background will already be filled with and LowColor set to BeBackgroundGrey
			//- The highlight and shadow edges will already be drawn
			//- The header_rect does not include the one-pixel border for the highlight and shadow edges.
			//- The view font will already be set to the font specified when the ColumnListView was
			//  constructed, and should not be changed
			//- If text is being rendered, it should be rendered at
			//	BPoint text_point(header_rect.left+8.0,header_rect.top+1.0+font_ascent)
			//- If sort_key is true, the text should be underlined, with the underline being drawn from
			//  BPoint(text_point.x-1,text_point.y+2.0) to BPoint(text_point.x-1+label_width,text_point.y+2.0)
			//- If focus is true, the text and underline should be in BeFocusBlue, otherwise in Black.
	private:
		friend class ColumnListView;
		friend class CLVColumnLabelView;
		friend class CLVListItem;

		BRect TruncateText(float column_width);

		char *fLabel;
		char* fTruncatedText;
		BRect fCachedRect;
		float fWidth;
		float fMinWidth;
		float fColumnBegin;
		float fColumnEnd;
		uint32 fFlags;
		bool fPushedByExpander;
		CLVSortMode fSortMode;
		ColumnListView* fParent;
};


#endif
