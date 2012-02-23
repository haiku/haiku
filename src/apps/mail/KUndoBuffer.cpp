#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "KUndoBuffer.h"


KUndoItem::KUndoItem(const char* redo_text,
					int32 length,
					int32 offset,
					undo_type history,
					int32 cursor_pos)
{
	Offset = offset;
	Length = length;
	History = history;
	CursorPos = cursor_pos;

	if (redo_text!=NULL) {
		RedoText = (char*)malloc(length);
		memcpy(RedoText, redo_text, length);
		if (RedoText!=NULL) {
			fStatus = B_OK;
		} else {
			fStatus = B_ERROR;
		}
	}
}

KUndoItem::~KUndoItem()
{
	free(RedoText);
}

status_t
KUndoItem::InitCheck()
{
	return fStatus;
}

void
KUndoItem::Merge(const char* text, int32 length)
{
	RedoText = (char*)realloc(RedoText, Length + length);
	memcpy(&RedoText[Length], text, length);
	Length += length;
}


KUndoBuffer::KUndoBuffer():BList(1024)
{
	fIndex = 0;
	Off();
	fNewItem = true;
}

KUndoBuffer::~KUndoBuffer()
{
	MakeEmpty();
}


bool
KUndoBuffer::AddItem(KUndoItem* item, int32 index)
{
	for (int32 i=CountItems()-1; i>=index; i--) {
		RemoveItem(i);
	}
	return AddItem(item);
}

bool
KUndoBuffer::AddItem(KUndoItem* item)
{
	return BList::AddItem(item);
}

void
KUndoBuffer::MakeEmpty(void)
{
	for(int32 i=CountItems()-1; i>=0;i--) {
		RemoveItem(i);
	}
}

KUndoItem*
KUndoBuffer::RemoveItem(int32 index)
{
	if (fIndex>=CountItems()) fIndex--;
	delete this->ItemAt(index);
	return (KUndoItem*)BList::RemoveItem(index);
}

KUndoItem*
KUndoBuffer::ItemAt(int32 index) const
{
	return (KUndoItem*)BList::ItemAt(index);
}


void
KUndoBuffer::On()
{
	fNoTouch = false;
}

void
KUndoBuffer::Off()
{
	fNoTouch = true;
}

status_t
KUndoBuffer::NewUndo(const char* text, int32 length, int32 offset,
	undo_type history, int32 cursor_pos)
{
	KUndoItem* NewUndoItem = new KUndoItem(text, length, offset, history,
		cursor_pos);

	status_t status = NewUndoItem->InitCheck();
	if ( status != B_OK) {
		delete NewUndoItem;
		return status; 
	}
	AddItem(NewUndoItem, fIndex);
	fIndex++;
	return status;
}


status_t
KUndoBuffer::AddUndo(const char* text, int32 length, int32 offset,
	undo_type history, int32 cursor_pos)
{
	if (fNoTouch)
		return B_OK;

	status_t status = B_OK;

	if (fNewItem || (fIndex < CountItems()) || (CountItems()==0)) {
		status = NewUndo(text, length, offset, history, cursor_pos);
		fNewItem = false;
	} else {
		KUndoItem* CurrentUndoItem;
		CurrentUndoItem = ItemAt(fIndex-1);
		if (CurrentUndoItem!=NULL) {
			int32 c_length = CurrentUndoItem->Length;
			int32 c_offset = CurrentUndoItem->Offset;
			undo_type c_history = CurrentUndoItem->History;
			if (c_history == history) {
				switch(c_history) {
					case K_INSERTED:
					case K_REPLACED:
						if ((c_offset + c_length) == offset) {
							CurrentUndoItem->Merge(text, length);
						} else {
							status = NewUndo(text, length, offset, history,
								cursor_pos);
						}
						break;
					case K_DELETED:
						status = NewUndo(text, length, offset, history,
							cursor_pos);
						break;
				}
			} else {
				status = NewUndo(text, length, offset, history, cursor_pos);
			}
		}
	}

	return status;
}


status_t
KUndoBuffer::MakeNewUndoItem()
{
	if (fIndex >= CountItems()) {
		fNewItem = true;
		return B_OK;
	}
	return B_ERROR;
}


status_t
KUndoBuffer::Undo(char** text,
				int32* length,
				int32* offset,
				undo_type* history,
				int32* cursor_pos)
{
	KUndoItem* undoItem;
	status_t status;

	if (fIndex>0) {
		undoItem = ItemAt(fIndex-1);
		if (undoItem!=NULL) {
			*text = undoItem->RedoText;
			*length = undoItem->Length;
			*offset = undoItem->Offset;
			*history = undoItem->History;
			*cursor_pos = undoItem->CursorPos + undoItem->Length;
			status = B_OK;
		} else {
			status = B_ERROR;
		}
		fIndex--;
	} else {
		status = B_ERROR;
	}
	return status;
}

status_t
KUndoBuffer::Redo(char** text,
				int32* length,
				int32* offset,
				undo_type* history,
				int32* cursor_pos,
				bool* replaced)
{
	KUndoItem* undoItem;
	status_t status;
	
	if (fIndex < CountItems()) {
		undoItem = ItemAt(fIndex);
		if (undoItem!=NULL) {
			*text = undoItem->RedoText;
			*length = undoItem->Length;
			*offset = undoItem->Offset;
			*history = undoItem->History;
			*cursor_pos = undoItem->CursorPos;
			if ((fIndex+1) < CountItems()) {
				*replaced = ItemAt(fIndex+1)->History==K_REPLACED;
			} else {
				*replaced = false;
			}
			status = B_OK;
		} else {
			status = B_ERROR;
		}
		fIndex++;
	} else {
		status = B_ERROR;
	}
	return status;
}


void
KUndoBuffer::PrintToStream()
{
	for(int32 i=0; i<CountItems(); i++) {
		KUndoItem* item = ItemAt(i);
		printf("%3.3d   ", (int)i);
		switch(item->History) {
			case K_INSERTED:
				printf("INSERTED  ");
				break;
			case K_DELETED:
				printf("DELETED   ");
				break;
			case K_REPLACED:
				printf("REPLACED  ");
				break;
		}
		printf("Offset = %d  ", (int)item->Offset);
		printf("Length = %d  ", (int)item->Length);
		printf("CursorPos = %d  ", (int)item->CursorPos);
		printf("RedoText = '");
		for(int32 j=0;j<item->Length;j++) {
			uchar c = (uchar)item->RedoText[j];
			if (c >= 0x20) {
				printf("%c", c);
			} else {
				printf("?");
			}
		}
		printf("'\n");
	}
}

