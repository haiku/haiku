#include <List.h>

enum undo_type{
	K_INSERTED,	// 文字挿入時 (At the time of character insertion)
	K_DELETED,	// 削除時 (When deleting)
	K_REPLACED	// 置換、つまり削除→挿入の連続動作時 (Substitution, in other words deletion -> at the time of continuous action of insertion)
};

class KUndoItem
{
public:
	KUndoItem(const char *text,
			int32 length,
			int32 offset,
			undo_type history,
			int32 cursor_pos);
	~KUndoItem();
	void Merge(const char *text, int32 length);
	status_t InitCheck();

	int32 Offset;			// 開始位置 (Start position)
	int32 Length;			// 文字長 (Letter length)
	char *RedoText;		// 復元すべきテキスト (The text which it should reconstruct)
	undo_type History;		// 操作区分 (Operation division)
	int32 CursorPos;		// カーソル位置 (Cursor position)

private:
	status_t Status;
};

class KUndoBuffer:public BList
{
public:
	KUndoBuffer();
	~KUndoBuffer();

	bool AddItem(KUndoItem *item, int32 index);
	bool AddItem(KUndoItem *item);
	void MakeEmpty(void);
	KUndoItem *RemoveItem(int32 index);
	KUndoItem *ItemAt(int32 index) const;


	status_t AddUndo(const char *redo_text,
					int32 length,
					int32 offset,
					undo_type history,
					int32 cursor_pos);
	status_t MakeNewUndoItem();
	status_t Undo(char **text,
				int32 *length,
				int32 *offset,
				undo_type *history,
				int32 *cursor_pos);
	status_t Redo(char **text,
				int32 *length,
				int32 *offset,
				undo_type *history,
				int32 *cursor_pos,
				bool *replaced);

	void PrintToStream();

	void On();
	void Off();

private:
	int32 FIndex;
	bool FNewItem;
	bool FNoTouch;

	status_t NewUndo(const char *text,
					int32 length,
					int32 offset,
					undo_type history,
					int32 cursor_pos);
};
