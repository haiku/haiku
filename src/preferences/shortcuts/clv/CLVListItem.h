#ifndef CLVListItem_h
#define CLVListItem_h

#include <interface/ListItem.h>

//******************************************************************************************************
//**** PROJECT HEADER FILES AND CLASS NAME DECLARATIONS
//******************************************************************************************************
class ColumnListView;


//******************************************************************************************************
//**** CLVItem CLASS DECLARATION
//******************************************************************************************************
class CLVListItem : public BListItem
{
	public:
		//Constructor and destructor
		CLVListItem(uint32 level = 0, bool superitem = false, bool expanded = false, float minheight = 0.0);
		virtual ~CLVListItem();

		//Archival stuff
		/* Not implemented yet
		CLVItem(BMessage* archive);
		static CLVItem* Instantiate(BMessage* data);
		virtual	status_t Archive(BMessage* data, bool deep = true) const;
		*/
		
		virtual void DrawItemColumn(BView* owner, BRect item_column_rect, int32 column_index, bool columnSelected,
			bool complete) = 0;	//column_index (0-N) is based on the order in which the columns were added
								//to the ColumnListView, not the display order.  An index of -1 indicates
								//that the program needs to draw a blank area beyond the last column.  The
								//main purpose is to allow the highlighting bar to continue all the way to
								//the end of the ColumnListView, even after the end of the last column.
								
		virtual void DrawItem(BView* owner, BRect itemRect, bool complete);
								//In general, you don't need or want to override DrawItem().
		float ExpanderShift(int32 column_index, BView* owner);
		virtual void Update(BView* owner, const BFont* font);
		bool IsSuperItem() const;
		void SetSuperItem(bool superitem);
		uint32 OutlineLevel() const;
		void SetOutlineLevel(uint32 level);

        virtual void Pulse(BView * owner);  // Called periodically when this item is selected.
        
        int32 GetSelectedColumn() const {return _selectedColumn;}
        void SetSelectedColumn(int32 i) {_selectedColumn = i;}
                
	private:
		friend class ColumnListView;

		bool fSuperItem;
		uint32 fOutlineLevel;
		float fMinHeight;
		BRect fExpanderButtonRect;
		BRect fExpanderColumnRect;
		BList* fSortingContextBList;
		ColumnListView* fSortingContextCLV;
		int32 _selectedColumn;
};

#endif
