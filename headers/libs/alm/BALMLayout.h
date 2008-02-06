#ifndef	BALM_LAYOUT_H
#define	BALM_LAYOUT_H

#include <File.h>
#include <Layout.h>
#include <List.h>
#include <Size.h>
#include <SupportDefs.h>
#include <View.h>

#include "LayoutStyleType.h"
#include "LinearSpec.h"


namespace BALM {

class Area;
class Column;
class Row;
class XTab;
class YTab;
	
/**
 * A GUI layout engine using the ALM.
 */
class BALMLayout : public BLayout, public LinearSpec {
		
public:
						BALMLayout();
	void					SolveLayout();

	XTab*				AddXTab();
	YTab*				AddYTab();
	Row*				AddRow();
	Row*				AddRow(YTab* top, YTab* bottom);
	Column*				AddColumn();
	Column*				AddColumn(XTab* left, XTab* right);
	Area*				AddArea(XTab* left, YTab* top, XTab* right, YTab* bottom,
								BView* content, BSize minContentSize);
	Area*				AddArea(Row* row, Column* column, BView* content,
								BSize minContentSize);
	Area*				AddArea(XTab* left, YTab* top, XTab* right, YTab* bottom,
								BView* content);
	Area*				AddArea(Row* row, Column* column, BView* content);
	Area*				AreaOf(BView* control);
	BList*				Areas() const;
	void					SetAreas(BList* areas);
	XTab*				Left() const;
	void					SetLeft(XTab* left);
	XTab*				Right() const;
	void					SetRight(XTab* right);
	YTab*				Top() const;
	void					SetTop(YTab* top);
	YTab*				Bottom() const;
	void					SetBottom(YTab* bottom);
	
	void					RecoverLayout(BView* parent);
	LayoutStyleType		LayoutStyle() const;
	void					SetLayoutStyle(LayoutStyleType style);

	BLayoutItem*			AddView(BView* child);
	BLayoutItem*			AddView(int32 index, BView* child);
	bool				AddItem(BLayoutItem* item);
	bool				AddItem(int32 index, BLayoutItem* item);
	bool				RemoveView(BView* child);
	bool				RemoveItem(BLayoutItem* item);
	BLayoutItem*			RemoveItem(int32 index);

	BSize				MinSize();
	BSize				MaxSize();
	BSize				PreferredSize();
	BAlignment			Alignment();
	bool				HasHeightForWidth();
	void					GetHeightForWidth(float width, float* min,
								float* max, float* preferred);
	void					InvalidateLayout();
	void					LayoutView();
	
	char*				PerformancePath() const;
	void					SetPerformancePath(char* path);

private:
	BSize				CalculateMinSize();
	BSize				CalculateMaxSize();
	BSize				CalculatePreferredSize();

private:
	LayoutStyleType		fLayoutStyle;
	bool				fActivated;

	BList*				fAreas;
	XTab*				fLeft;
	XTab*				fRight;
	YTab*				fTop;
	YTab*				fBottom;
	BSize				fMinSize;
	BSize				fMaxSize;
	BSize				fPreferredSize;
	char*				fPerformancePath;
};

}	// namespace BALM

using BALM::BALMLayout;

#endif	// BALM_LAYOUT_H
