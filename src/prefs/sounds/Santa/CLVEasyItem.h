//CLVEasyItem header file

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


#ifndef _CLV_EASY_ITEM_H_
#define _CLV_EASY_ITEM_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <List.h>
#include <GraphicsDefs.h>

//******************************************************************************************************
//**** PROJECT HEADER FILES AND CLASS NAME DECLARATIONS
//******************************************************************************************************
#include "CLVListItem.h"


//******************************************************************************************************
//**** TYPE DEFINITIONS AND CONSTANTS
//******************************************************************************************************
enum
{
	CLVColNone =				0x00000000,
	CLVColStaticText = 			0x00000001,
	CLVColTruncateText =		0x00000002,
	CLVColBitmap = 				0x00000003,
	CLVColUserText = 			0x00000004,
	CLVColTruncateUserText =	0x00000005,

	CLVColTypesMask =			0x00000007,

	CLVColFlagBitmapIsCopy =	0x00000008,
	CLVColFlagNeedsTruncation =	0x00000010,
	CLVColFlagRightJustify =	0x00000020
};

//******************************************************************************************************
//**** CLVEasyItem CLASS DECLARATION
//******************************************************************************************************
class CLVEasyItem : public CLVListItem
{
	public:
		//Constructor and destructor
		CLVEasyItem(uint32 level = 0, bool superitem = false, bool expanded = false, float minheight = 0.0);
		virtual ~CLVEasyItem();

		virtual void SetColumnContent(int column_index, const BBitmap *bitmap, float horizontal_offset = 2.0,
			bool copy = true, bool right_justify = false);
		virtual void SetColumnContent(int column_index, const char *text, bool truncate = true,
			bool right_justify = false);
		virtual void SetColumnUserTextContent(int column_index, bool truncate = true, bool right_justify = false);
		const char* GetColumnContentText(int column_index);
		const BBitmap* GetColumnContentBitmap(int column_index);
		virtual void DrawItemColumn(BView* owner, BRect item_column_rect, int32 column_index, bool complete);
		virtual void Update(BView *owner, const BFont *font);
		static int CompareItems(const CLVListItem* a_Item1, const CLVListItem* a_Item2, int32 KeyColumn);
		BRect TruncateText(int32 column_index, float column_width, BFont* font);
			//Returns the area that needs to be redrawn, or BRect(-1,-1,-1,-1) if nothing
		virtual void ColumnWidthChanged(int32 column_index, float column_width, ColumnListView* the_view);
		virtual void FrameChanged(int32 column_index, BRect new_frame, ColumnListView* the_view);
		inline float GetTextOffset() {return text_offset;}
		virtual const char* GetUserText(int32 column_index, float column_width) const;
		void	SetTextColor(uchar red,uchar green,uchar blue);
		void	SetBackgroundColor(rgb_color *color);
		
		BList m_column_types;	//List of int32's converted from CLVColumnTypes
		BList m_column_content;	//List of char* (full content) or BBitmap*
	private:
		void PrepListsForSet(int column_index);
		
		
		
		BList m_aux_content;	//List of char* (truncated content) or int32 for bitmap horizontal offset
		BList m_cached_rects;	//List of BRect for truncated text

	protected:
		float text_offset;
		rgb_color	fTextColor;
		rgb_color	*fBackgroundColor;
};


#endif
