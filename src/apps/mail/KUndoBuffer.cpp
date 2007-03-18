#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "KUndoBuffer.h"


/*
 KUndoItem
KUndoBuffer に格納される Undo/Redo 情報を記憶するクラス
(The class which remembers the Undo/Redo information which is housed in the KUndoBuffer)
*/
KUndoItem::KUndoItem(const char *redo_text,
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
		RedoText = (char *)malloc(length);
		memcpy(RedoText, redo_text, length);
		if (RedoText!=NULL) {
			Status = B_OK;
		} else {
			Status = B_ERROR;
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
	return Status;
}

void
KUndoItem::Merge(const char *text, int32 length)
{
	RedoText = (char *)realloc(RedoText, Length + length);
	memcpy(&RedoText[Length], text, length);
	Length += length;
}


/*
 KUndoBuffer
Undo/Redo情報リストを管理するクラス。
(The class which manages the Undo/Redo information list.)
このクラス自体は、BTextViewに影響を及ぼすものではない。
(This class itself is not something which exerts influence on the BTextView.)
あくまで、アプリケーションがUndo/Redoを実装するための補助にすぎない。
(To the last, it is no more than an assistance because application mounts the Undo/Redo.)
*/
KUndoBuffer::KUndoBuffer():BList(1024)
{
	FIndex = 0;
	Off();
	FNewItem = true;
}

KUndoBuffer::~KUndoBuffer()
{
	MakeEmpty();
}

/*
Undo情報を追加する。
(Undo information is added.)
リスト途中に追加したら、以降はRedoの必要がなくなるため、
追加位置以降のUndo情報があれば削除する。
(When it adds on the list middle, if later because necessity of the Redo is
gone, there is Undo information after the additional position, it deletes.)
*/
bool
KUndoBuffer::AddItem(KUndoItem *item, int32 index)
{
	for (int32 i=CountItems()-1; i>=index; i--) {
		RemoveItem(i);
	}
	return AddItem(item);
}

bool
KUndoBuffer::AddItem(KUndoItem *item)
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

KUndoItem *
KUndoBuffer::RemoveItem(int32 index)
{
	if (FIndex>=CountItems()) FIndex--;
	delete this->ItemAt(index);
	return (KUndoItem *)BList::RemoveItem(index);
}

KUndoItem *
KUndoBuffer::ItemAt(int32 index) const
{
	return (KUndoItem *)BList::ItemAt(index);
}

/*
 Off() 呼び出し後は、On()を実行しないとUndo情報追加を行わない。
 (Unless OFF () it calls after, ON () executes, Undo information addition is not done.)
*/
void
KUndoBuffer::On()
{
	FNoTouch = false;
}

void
KUndoBuffer::Off()
{
	FNoTouch = true;
}

status_t
KUndoBuffer::NewUndo(const char *text, int32 length, int32 offset, undo_type history, int32 cursor_pos)
{
	KUndoItem *NewUndoItem = new KUndoItem(text, length, offset, history, cursor_pos);
	status_t status = NewUndoItem->InitCheck();
	if ( status != B_OK) {
		delete NewUndoItem;
		return status; 
	}
	AddItem(NewUndoItem, FIndex);
	FIndex++;
	return status;
}

/*
Undo情報をリストに追加する。
(Undo information is added to the list.)
*/
status_t
KUndoBuffer::AddUndo(const char *text, int32 length, int32 offset, undo_type history, int32 cursor_pos)
{
	if (FNoTouch) return B_OK;

	status_t status;

	// 新たな追加予約がある(FNewItem)か、現在位置(FIndex)が最後尾でないか、Undoリストが空であれば
	// 新たにUndo情報を追加する。
	// (There is new additional reservation, if (the FNewItem), 現在位置 (the
	// FIndex) is not the last tail or and the Undo list is the sky, Undo
	// information is added anew.)
	// さもなくば、最後尾Undo情報に結合すべきなら結合し、そうでなければ
	// やはり新たにUndo情報を追加する。
	// (Without, if it should connect to last tail Undo information, if it
	// connects and so is not Undo information is added after all anew.)
	if (FNewItem || (FIndex < CountItems()) || (CountItems()==0)) {
		status = NewUndo(text, length, offset, history, cursor_pos);
		FNewItem = false;
	} else {
		KUndoItem *CurrentUndoItem;
		CurrentUndoItem = ItemAt(FIndex-1);
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
							status = NewUndo(text, length, offset, history, cursor_pos);
						}
						break;
					case K_DELETED:
						status = NewUndo(text, length, offset, history, cursor_pos);
						break;
				}
			} else {
				status = NewUndo(text, length, offset, history, cursor_pos);
			}
		}
	}

	return B_OK;
}

// Enterを押した、矢印キーを押した等、Undoを一区切り起きたい場合に
// MakeNewUndoItem()を呼び出す。
// (The Enter was pushed, one you divide the Undo and the MakeNewUndoItem ()
// you call when we would like to occur e.g., the arrow key was pushed.)
status_t
KUndoBuffer::MakeNewUndoItem()
{
	if (FIndex >= CountItems()) {
		FNewItem = true;
		return B_OK;
	}
	return B_ERROR;
}

// Undo() Redo() は、FIndexをいったりきたりしながら
// 挿入位置、文字長、復元テキスト等の情報を返す。
// (The Undo () the Redo (), the FIndex, while going back and forth, it
// returns the information of insertion position, letter length and the
// restoration text et cetera.)
status_t
KUndoBuffer::Undo(char **text,
				int32 *length,
				int32 *offset,
				undo_type *history,
				int32 *cursor_pos)
{
	KUndoItem *undoItem;
	status_t status;

	if (FIndex>0) {
		undoItem = ItemAt(FIndex-1);
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
		FIndex--;
	} else {
		status = B_ERROR;
	}
	return status;
}

status_t
KUndoBuffer::Redo(char **text,
				int32 *length,
				int32 *offset,
				undo_type *history,
				int32 *cursor_pos,
				bool *replaced)
{
	KUndoItem *undoItem;
	status_t status;
	
	if (FIndex < CountItems()) {
		undoItem = ItemAt(FIndex);
		if (undoItem!=NULL) {
			*text = undoItem->RedoText;
			*length = undoItem->Length;
			*offset = undoItem->Offset;
			*history = undoItem->History;
			*cursor_pos = undoItem->CursorPos;
			if ((FIndex+1) < CountItems()) {
				*replaced = ItemAt(FIndex+1)->History==K_REPLACED;
			} else {
				*replaced = false;
			}
			status = B_OK;
		} else {
			status = B_ERROR;
		}
		FIndex++;
	} else {
		status = B_ERROR;
	}
	return status;
}

// Undo/Redo 情報を標準出力に書きだす。
// (It starts writing Undo/Redo information on standard output.)
// デバッグ時のみ。
// (Only when debugging.)
void
KUndoBuffer::PrintToStream()
{
	for(int32 i=0; i<CountItems(); i++) {
		KUndoItem *item = ItemAt(i);
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
