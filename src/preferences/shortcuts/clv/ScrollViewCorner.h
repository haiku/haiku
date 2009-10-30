#ifndef ScrollViewCorner_h
#define ScrollViewCorner_h

//If you have a BScrollView with horizontal and vertical sliders that isn't 
//seated to the lower-right corner of a B_DOCUMENT_WINDOW, there's a "hole"
//between the sliders that needs to be filled.  You can use this to fill it.
//In general, it looks best to set the ScrollViewCorner color to 
//BeInactiveControlGrey if the vertical BScrollBar is inactive, and the color
//to BeBackgroundGrey if the vertical BScrollBar is active.  Have a look at
//Demo3 of ColumnListView to see what I mean if this is unclear.


class ScrollViewCorner : public BView
{
	public:
		ScrollViewCorner(float Left,float Top);
		~ScrollViewCorner();
		void Draw(BRect Update);
};

#endif
