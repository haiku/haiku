#ifndef _K_UNDOBUFFER_H
#define _K_UNDOBUFFER_H


#include <List.h>


enum undo_type{
	K_INSERTED,
	K_DELETED,
	K_REPLACED
};


class KUndoItem
{
public:
							KUndoItem(const char* text, int32 length,
								int32 offset, undo_type history,
								int32 cursor_pos);
							~KUndoItem();

			void			Merge(const char* text, int32 length);
			status_t		InitCheck();

			int32			Offset;
			int32			Length;
			char*			RedoText;
			undo_type		History;
			int32			CursorPos;

private:
			status_t		fStatus;
};


class KUndoBuffer : public BList
{
public:
							KUndoBuffer();
							~KUndoBuffer();

			bool			AddItem(KUndoItem* item, int32 index);
			bool			AddItem(KUndoItem* item);
			void			MakeEmpty(void);
			KUndoItem*		RemoveItem(int32 index);
			KUndoItem*		ItemAt(int32 index) const;

			status_t		AddUndo(const char* redo_text, int32 length,
								int32 offset, undo_type history,
								int32 cursor_pos);

			status_t		MakeNewUndoItem();

			status_t		Undo(char** text, int32* length, int32* offset,
								undo_type* history, int32* cursor_pos);

			status_t		Redo(char** text, int32* length, int32* offset,
								undo_type* history, int32* cursor_pos,
								bool* replaced);

			void			PrintToStream();
			void			On();
			void			Off();

private:
			int32			fIndex;
			bool			fNewItem;
			bool			fNoTouch;

			status_t		NewUndo(const char* text, int32 length,
								int32 offset, undo_type history,
								int32 cursor_pos);
};


#endif	// _K_UNDOBUFFER_H

