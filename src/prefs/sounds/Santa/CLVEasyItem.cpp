//CLVListItem source file

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


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <string.h>
#include <Region.h>
#include <ClassInfo.h>
#include <String.h>
#include <stdio.h>

//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "CLVEasyItem.h"
#include "Colors.h"
#include "CLVColumn.h"
#include "ColumnListView.h"
#include "NewStrings.h"



//******************************************************************************************************
//**** CLVEasyItem CLASS DEFINITION
//******************************************************************************************************
CLVEasyItem::CLVEasyItem(uint32 level, bool superitem, bool expanded, float minheight)
: CLVListItem(level,superitem,expanded,minheight)
{
	text_offset = 0.0;
	SetTextColor(0,0,0);
	fBackgroundColor=NULL;
}


CLVEasyItem::~CLVEasyItem()
{
	int num_columns = m_column_types.CountItems();
	for(int column = 0; column < num_columns; column++)
	{
		int32 type = (int32)m_column_types.ItemAt(column);
		bool bitmap_is_copy = false;
		if(type & CLVColFlagBitmapIsCopy)
			bitmap_is_copy = true;
		type &= CLVColTypesMask;
		if(type == 	CLVColStaticText || type == CLVColTruncateText)
			delete[] ((char*)m_column_content.ItemAt(column));
		if(type == CLVColTruncateText)
			delete[] ((char*)m_aux_content.ItemAt(column));
		if(type == CLVColBitmap && bitmap_is_copy)
			delete ((BBitmap*)m_column_content.ItemAt(column));
		delete (BRect*)m_cached_rects.ItemAt(column);
	}
	SetBackgroundColor(NULL);
}


void CLVEasyItem::PrepListsForSet(int column_index)
{
	int cur_num_columns = m_column_types.CountItems();
	bool delete_old = (cur_num_columns >= column_index-1);
	while(cur_num_columns <= column_index)
	{
		m_column_types.AddItem((void*)CLVColNone);
		m_column_content.AddItem(NULL);
		m_aux_content.AddItem(NULL);
		m_cached_rects.AddItem(new BRect(-1,-1,-1,-1));
		cur_num_columns++;
	}
	if(delete_old)
	{
		//Column content exists already so delete the old entries
		int32 old_type = (int32)m_column_types.ItemAt(column_index);
		bool bitmap_is_copy = false;
		if(old_type & CLVColFlagBitmapIsCopy)
			bitmap_is_copy = true;
		old_type &= CLVColTypesMask;

		void* old_content = m_column_content.ItemAt(column_index);
		char* old_truncated = (char*)m_aux_content.ItemAt(column_index);
		if(old_type == CLVColStaticText || old_type == CLVColTruncateText)
			delete[] ((char*)old_content);
		if(old_type == CLVColTruncateText)
			delete[] old_truncated;
		if(old_type == CLVColBitmap && bitmap_is_copy)
			delete ((BBitmap*)old_content);
		((BRect**)m_cached_rects.Items())[column_index]->Set(-1,-1,-1,-1);
	}
}


void CLVEasyItem::SetColumnContent(int column_index, const char *text, bool truncate, bool right_justify)
{
	PrepListsForSet(column_index);

	//Create the new entry
	((BRect**)m_cached_rects.Items())[column_index]->Set(-1,-1,-1,-1);
	if(text == NULL || text[0] == 0)
	{
		((int32*)m_column_types.Items())[column_index] = CLVColNone;
		((char**)m_column_content.Items())[column_index] = NULL;
		((char**)m_aux_content.Items())[column_index] = NULL;
	}
	else
	{
		char* copy = Strdup_new(text);
		((char**)m_column_content.Items())[column_index] = copy;

		if(!truncate)
		{
			((int32*)m_column_types.Items())[column_index] = CLVColStaticText;
			((char**)m_aux_content.Items())[column_index] = NULL;
		}
		else
		{
			((int32*)m_column_types.Items())[column_index] = CLVColTruncateText|CLVColFlagNeedsTruncation;
			copy = new char[strlen(text)+3];
			strcpy(copy,text);
			((char**)m_aux_content.Items())[column_index] = copy;
		}
		if(right_justify)
			((int32*)m_column_types.Items())[column_index] |= CLVColFlagRightJustify;
	}
}


void CLVEasyItem::SetColumnContent(int column_index, const BBitmap *bitmap, float horizontal_offset, bool copy,
	bool right_justify)
{
	PrepListsForSet(column_index);

	//Create the new entry
	if(bitmap == NULL)
	{
		((int32*)m_column_types.Items())[column_index] = CLVColNone;
		((char**)m_column_content.Items())[column_index] = NULL;
		((char**)m_aux_content.Items())[column_index] = NULL;
	}
	else
	{
		if(copy)
			((int32*)m_column_types.Items())[column_index] = CLVColBitmap|CLVColFlagBitmapIsCopy;
		else
			((int32*)m_column_types.Items())[column_index] = CLVColBitmap;
		if(right_justify)
			((int32*)m_column_types.Items())[column_index] |= CLVColFlagRightJustify;
		BBitmap* the_bitmap;
		if(copy)
		{
			the_bitmap = new BBitmap(bitmap->Bounds(),bitmap->ColorSpace());
			int32 copy_ints = bitmap->BitsLength()/4;
			int32* source = (int32*)bitmap->Bits();
			int32* dest = (int32*)the_bitmap->Bits();
			for(int32 i = 0; i < copy_ints; i++)
				dest[i] = source[i];
		}
		else
			the_bitmap = (BBitmap*)bitmap;
		((BBitmap**)m_column_content.Items())[column_index] = the_bitmap;
		((int32*)m_aux_content.Items())[column_index] = (int32)horizontal_offset;
	}
}


void CLVEasyItem::SetColumnUserTextContent(int column_index, bool truncate, bool right_justify)
{
	PrepListsForSet(column_index);
	if(truncate)
		((int32*)m_column_types.Items())[column_index] = CLVColTruncateUserText;
	else
		((int32*)m_column_types.Items())[column_index] = CLVColUserText;
	if(right_justify)
		((int32*)m_column_types.Items())[column_index] |= CLVColFlagRightJustify;
}


const char* CLVEasyItem::GetColumnContentText(int column_index)
{
	int32 type = ((int32)m_column_types.ItemAt(column_index)) & CLVColTypesMask;
	if(type == CLVColStaticText || type == CLVColTruncateText)
		return (char*)m_column_content.ItemAt(column_index);
	if(type == CLVColTruncateUserText || type == CLVColUserText)
		return GetUserText(column_index,-1);
	return NULL;
}


const BBitmap* CLVEasyItem::GetColumnContentBitmap(int column_index)
{
	int32 type = ((int32)m_column_types.ItemAt(column_index)) & CLVColTypesMask;
	if(type != CLVColBitmap)
		return NULL;
	return (BBitmap*)m_column_content.ItemAt(column_index);
}


void CLVEasyItem::DrawItemColumn(BView *owner, BRect item_column_rect, int32 column_index, bool complete)
{
	rgb_color color;

	bool selected = IsSelected();
	if(selected)
		color = ((ColumnListView*)owner)->ItemSelectColor();
	else{
		if(fBackgroundColor)
		{
			color.red = fBackgroundColor->red;
			color.blue = fBackgroundColor->blue;
			color.green = fBackgroundColor->green;
			complete = true;
		}else
			color = owner->ViewColor();
	}
	owner->SetLowColor(color);
	owner->SetDrawingMode(B_OP_COPY);
	
	bool highlightTextOnly = ((ColumnListView*)owner)->HighlightTextOnly();
	
	
	if(selected || complete)
	{
		owner->SetHighColor(color);
		if(!highlightTextOnly && selected)
			owner->FillRect(item_column_rect);
		else if(!highlightTextOnly &&complete)
			owner->FillRect(item_column_rect);
		else if(highlightTextOnly&&complete)
			owner->Invalidate(item_column_rect);
	}

	if(column_index == -1)
		return;
	int32 type = (int32)m_column_types.ItemAt(column_index);
	if(type == 0)
		return;
	bool needs_truncation = false;
	if(type & CLVColFlagNeedsTruncation)
		needs_truncation = true;
	bool right_justify = false;
	if(type & CLVColFlagRightJustify)
		right_justify = true;
	type &= CLVColTypesMask;

	BRegion Region;
	Region.Include(item_column_rect);
	owner->ConstrainClippingRegion(&Region);

	BRect* cached_rect = (BRect*)m_cached_rects.ItemAt(column_index);
	if(cached_rect != NULL)
		*cached_rect = item_column_rect;

	if(type == CLVColStaticText || type == CLVColTruncateText || type == CLVColTruncateUserText ||
		type == CLVColUserText)
	{
		const char* text = NULL;
		owner->SetDrawingMode(B_OP_COPY);
		
		
		if(type == CLVColTruncateText)
		{
			if(needs_truncation)
			{
				BFont owner_font;
				owner->GetFont(&owner_font);
				TruncateText(column_index,item_column_rect.right-item_column_rect.left,&owner_font);
				((int32*)m_column_types.Items())[column_index] &= (CLVColFlagNeedsTruncation^0xFFFFFFFF);
			}
			text = (const char*)m_aux_content.ItemAt(column_index);
		}
		else if(type == CLVColStaticText)
			text = (const char*)m_column_content.ItemAt(column_index);
		else if(type == CLVColTruncateUserText)
			text = GetUserText(column_index,item_column_rect.right-item_column_rect.left);
		else if(type == CLVColUserText)
			text = GetUserText(column_index,-1);

		if(text != NULL)
		{
			BPoint draw_point;
			if(!right_justify)
				draw_point.Set(item_column_rect.left+2.0,item_column_rect.top+text_offset);
			else
			{
				BFont font;
				owner->GetFont(&font);
				float string_width = font.StringWidth(text);
				draw_point.Set(item_column_rect.right-2.0-string_width,item_column_rect.top+text_offset);
			}
			if(selected && highlightTextOnly)
			{
				float stringWidth = owner->StringWidth(text);
				if(stringWidth>0)
				{
					font_height FontAttributes;
					BFont owner_font;
					owner->GetFont(&owner_font);
					owner_font.GetHeight(&FontAttributes);
					float FontHeight = ceil(FontAttributes.ascent) + ceil(FontAttributes.descent);
		
					BRect textRect(item_column_rect);
					textRect.right = textRect.left + stringWidth + 3;
					float pad = (textRect.Height() - FontHeight)/2.0;
					textRect.top += pad;
					textRect.bottom = textRect.top + FontHeight;
					owner->SetHighColor(color);
					owner->FillRoundRect(textRect,1,1);
				}
			}	
			owner->SetHighColor(fTextColor);			
			owner->DrawString(text,draw_point);
		}
	}
	else if(type == CLVColBitmap)
	{
		const BBitmap* bitmap = (BBitmap*)m_column_content.ItemAt(column_index);
		BRect bounds = bitmap->Bounds();
		float horizontal_offset = (float)((int32)m_aux_content.ItemAt(column_index));
		if(!right_justify)
		{
			item_column_rect.left += horizontal_offset;
			item_column_rect.right = item_column_rect.left + (bounds.right-bounds.left);
		}
		else
		{
			item_column_rect.left = item_column_rect.right-horizontal_offset-(bounds.right-bounds.left);
			item_column_rect.right -= horizontal_offset;
		}
		item_column_rect.top += ceil(((item_column_rect.bottom-item_column_rect.top)-(bounds.bottom-bounds.top))/2.0);
		item_column_rect.bottom = item_column_rect.top + (bounds.bottom-bounds.top);
		
		if(selected && highlightTextOnly)
		{
			BRect selectRect(item_column_rect);
			owner->SetDrawingMode(B_OP_COPY);
			owner->SetHighColor(color);
			selectRect.bottom += 2;
			selectRect.right += 3;
			selectRect.top -= 2;
			selectRect.left -= 1;
			owner->FillRoundRect(selectRect,2,2);
		}
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(bitmap,item_column_rect);
		owner->SetDrawingMode(B_OP_COPY);
	}
	owner->ConstrainClippingRegion(NULL);
}


void CLVEasyItem::Update(BView *owner, const BFont *font)
{
	CLVListItem::Update(owner,font);
	font_height FontAttributes;
	BFont owner_font;
	owner->GetFont(&owner_font);
	owner_font.GetHeight(&FontAttributes);
	float FontHeight = ceil(FontAttributes.ascent) + ceil(FontAttributes.descent);
	text_offset = ceil(FontAttributes.ascent) + (Height()-FontHeight)/2.0;
}


int CLVEasyItem::CompareItems(const CLVListItem *a_Item1, const CLVListItem *a_Item2, int32 KeyColumn)
{
	const CLVEasyItem* Item1 = cast_as(a_Item1,const CLVEasyItem);
	const CLVEasyItem* Item2 = cast_as(a_Item2,const CLVEasyItem);
	if(Item1 == NULL || Item2 == NULL || Item1->m_column_types.CountItems() <= KeyColumn ||
		Item2->m_column_types.CountItems() <= KeyColumn)
		return 0;
	
	int32 type1 = ((int32)Item1->m_column_types.ItemAt(KeyColumn)) & CLVColTypesMask;
	int32 type2 = ((int32)Item2->m_column_types.ItemAt(KeyColumn)) & CLVColTypesMask;

	if(!((type1 == CLVColStaticText || type1 == CLVColTruncateText || type1 == CLVColTruncateUserText ||
		type1 == CLVColUserText) && (type2 == CLVColStaticText || type2 == CLVColTruncateText ||
		type2 == CLVColTruncateUserText || type2 == CLVColUserText)))
		return 0;

	const char* text1 = NULL;
	const char* text2 = NULL;

	if(type1 == CLVColStaticText || type1 == CLVColTruncateText)
		text1 = (const char*)Item1->m_column_content.ItemAt(KeyColumn);
	else if(type1 == CLVColTruncateUserText || type1 == CLVColUserText)
		text1 = Item1->GetUserText(KeyColumn,-1);

	if(type2 == CLVColStaticText || type2 == CLVColTruncateText)
		text2 = (const char*)Item2->m_column_content.ItemAt(KeyColumn);
	else if(type2 == CLVColTruncateUserText || type2 == CLVColUserText)
		text2 = Item2->GetUserText(KeyColumn,-1);
	
	return strcasecmp(text1,text2);
}


BRect CLVEasyItem::TruncateText(int32 column_index, float column_width, BFont* font)
{
	column_width -= 4;
		//Because when I draw the text I start drawing 2 pixels to the right from the column's left edge, and want
		//to stop 2 pixels before the right edge
	BRect invalid(-1,-1,-1,-1);
	char* full_text = (char*)m_column_content.ItemAt(column_index);
	char new_text[256];
	char* truncated_text = (char*)m_aux_content.ItemAt(column_index);
	BString STR = full_text;
	font->TruncateString(&STR,B_TRUNCATE_END,column_width);
	::strncpy(new_text,STR.String(),256);
	//GetTruncatedString(full_text,new_text,column_width,256,font);
	if(strcmp(truncated_text,new_text)!=0)
	{
		//The truncated text has changed
		BRect* temp = (BRect*)m_cached_rects.ItemAt(column_index);
		if(temp != NULL && *temp != BRect(-1,-1,-1,-1))
		{
			invalid = *temp;

			//Figure out which region just got changed
			int32 cmppos;
			int32 cmplen = strlen(new_text);
			char remember = 0;
			for(cmppos = 0; cmppos <= cmplen; cmppos++)
				if(new_text[cmppos] != truncated_text[cmppos])
				{
					remember = new_text[cmppos];
					new_text[cmppos] = 0;
					break;
				}
			invalid.left += 2 + font->StringWidth(new_text);
			new_text[cmppos] = remember;
		}
		//Remember the new truncated text
		strcpy(truncated_text,new_text);
	}
	return invalid;
}


void CLVEasyItem::ColumnWidthChanged(int32 column_index, float column_width, ColumnListView *the_view)
{
	BRect* cached_rect = (BRect*)m_cached_rects.ItemAt(column_index);
	if(cached_rect == NULL || *cached_rect == BRect(-1,-1,-1,-1))
		return;
	float width_delta = column_width-(cached_rect->right-cached_rect->left);		
	cached_rect->right += width_delta;

	int num_columns = m_cached_rects.CountItems();
	for(int column = 0; column < num_columns; column++)
		if(column != column_index)
		{
			BRect* other_rect = (BRect*)m_cached_rects.ItemAt(column);
			if(other_rect->left > cached_rect->left)
				other_rect->OffsetBy(width_delta,0);
		}

	int32 type = (int32)m_column_types.ItemAt(column_index);
	bool right_justify = (type&CLVColFlagRightJustify);
	type &= CLVColTypesMask;
	BRect invalid;
	BFont view_font;
	if(type == CLVColTruncateText)
	{
		BRect bounds = the_view->Bounds();
		if(cached_rect->top <= bounds.bottom && cached_rect->bottom >= bounds.top)
		{
			//If it's onscreen, truncate and invalidate the changed area
			the_view->GetFont(&view_font);
			invalid = TruncateText(column_index, column_width,&view_font);
			((int32*)m_column_types.Items())[column_index] &= (CLVColFlagNeedsTruncation^0xFFFFFFFF);
			if(invalid != BRect(-1.0,-1.0,-1.0,-1.0))
			{
				if(!right_justify)
					the_view->Invalidate(invalid);
				else
					the_view->Invalidate(*cached_rect);
			}
		}
		else
			//If it's not onscreen flag it for truncation the next time it's drawn
			((int32*)m_column_types.Items())[column_index] |= CLVColFlagNeedsTruncation;
	}
	if(type == CLVColTruncateUserText)
	{
		char old_text[256];
		Strtcpy(old_text,GetUserText(column_index,column_width-width_delta),256);
		char new_text[256];
		Strtcpy(new_text,GetUserText(column_index,column_width),256);
		if(strcmp(old_text,new_text) != 0)
		{
			BRect* temp = (BRect*)m_cached_rects.ItemAt(column_index);
			if(temp != NULL && *temp != BRect(-1,-1,-1,-1))
			{
				invalid = *temp;
				if(!right_justify)
				{
					//The truncation changed, so find the point of divergence.
					int change_pos = 0;
					while(old_text[change_pos] == new_text[change_pos])
						change_pos++;
					new_text[change_pos] = 0;
					the_view->GetFont(&view_font);
					invalid.left += 2 + view_font.StringWidth(new_text);
					the_view->Invalidate(invalid);
				}
				else
					the_view->Invalidate(*cached_rect);
			}
		}
	}
}


void CLVEasyItem::FrameChanged(int32 column_index, BRect new_frame, ColumnListView *the_view)
{
	BRect* cached_rect = (BRect*)m_cached_rects.ItemAt(column_index);
	if(cached_rect == NULL)
		return;
	int32 type = ((int32)m_column_types.ItemAt(column_index)) & CLVColTypesMask;
	if(type == CLVColTruncateText)
		if(*cached_rect != new_frame)
		{
			*cached_rect = new_frame;

			//If it's onscreen, truncate and invalidate the changed area
			if(new_frame.Intersects(the_view->Bounds()))
			{
				BFont view_font;
				the_view->GetFont(&view_font);
				BRect invalid = TruncateText(column_index, new_frame.right-new_frame.left,&view_font);
				((int32*)m_column_types.Items())[column_index] &= (CLVColFlagNeedsTruncation^0xFFFFFFFF);
				if(invalid != BRect(-1.0,-1.0,-1.0,-1.0))
					the_view->Invalidate(invalid);
			}
			else
			//If it's not onscreen flag it for truncation the next time it's drawn
				((int32*)m_column_types.Items())[column_index] |= CLVColFlagNeedsTruncation;
		}
}


const char* CLVEasyItem::GetUserText(int32 column_index, float column_width) const
{
	return NULL;
}


void CLVEasyItem::SetTextColor(uchar red,uchar green,uchar blue)
{
	fTextColor.red = red;
	fTextColor.green = green;
	fTextColor.blue = blue;
}

void CLVEasyItem::SetBackgroundColor(rgb_color *color)
{
	delete fBackgroundColor;
	if(color)
	{
		fBackgroundColor = new rgb_color;
		fBackgroundColor->red = color->red;
		fBackgroundColor->blue = color->blue;
		fBackgroundColor->green = color->green;
	}else
		fBackgroundColor = NULL;
}
