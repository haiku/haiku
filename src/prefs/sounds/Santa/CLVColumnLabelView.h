//CLVColumnLabelView class header file

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


#ifndef _CLV_COLUMN_LABEL_VIEW_H_
#define _CLV_COLUMN_LABEL_VIEW_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <View.h>
#include <List.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES AND CLASS NAME DECLARATIONS
//******************************************************************************************************
class ColumnListView;
class CLVColumn;


//******************************************************************************************************
//**** CLASS AND STRUCTURE DECLARATIONS, ASSOCIATED CONSTANTS AND STATIC FUNCTIONS
//******************************************************************************************************
struct CLVDragGroup
{
	int32 GroupStartDispListIndex;		//Indices in the column display list where this group starts
	int32 GroupStopDispListIndex;		//and finishes
	float GroupBegin,GroupEnd;			//-1.0 if whole group is hidden
	CLVColumn* LastColumnShown;
	bool AllLockBeginning;
	bool AllLockEnd;
	bool Shown;							//False if none of the columns in this group are shown
	uint32 Flags;						//Uses CLV_NOT_MOVABLE, CLV_LOCK_AT_BEGINNING, CLV_LOCK_AT_END
};


class CLVColumnLabelView : public BView
{
	public:
		//Constructor and destructor
		CLVColumnLabelView(BRect Bounds,ColumnListView* Parent,const BFont* Font);
		~CLVColumnLabelView();

		//BView overrides
		virtual void Draw(BRect UpdateRect);
		virtual void MouseDown(BPoint Point);
		virtual void MouseMoved(BPoint where, uint32 code, const BMessage *message);
		virtual void MouseUp(BPoint where);

	private:
		friend class ColumnListView;
		friend class CLVColumn;

		float fFontAscent;
		BList* fDisplayList;

		//Column select and drag stuff
		CLVColumn* fColumnClicked;
		BPoint fPreviousMousePos;
		BPoint fMouseClickedPos;
		bool fColumnDragging;
		bool fColumnResizing;
		bool fModifiedCursor;
		BList fDragGroups;					//Groups of CLVColumns that must drag together
		int32 fDragGroup;					//Index into DragGroups of the group being dragged by user
		CLVDragGroup* fTheDragGroup;
		CLVDragGroup* fTheShownGroupBefore;
		CLVDragGroup* fTheShownGroupAfter;
		int32 fSnapGroupBefore,				//Index into DragGroups of TheShownGroupBefore and
			fSnapGroupAfter;				//TheShownGroupAfter, if the group the user is dragging is
											//allowed to snap there, otherwise -1
		float fDragBoxMouseHoldOffset,fResizeMouseHoldOffset;
		float fDragBoxWidth;				//Can include multiple columns; depends on CLV_LOCK_WITH_RIGHT
		float fPrevDragOutlineLeft,fPrevDragOutlineRight;
		float fSnapMin,fSnapMax;			//-1.0 indicates the column can't snap in the given direction
		ColumnListView* fParent;

		//Private functions
		void ShiftDragGroup(int32 NewPos);
		void UpdateDragGroups();
		void SetSnapMinMax();
};


#endif
