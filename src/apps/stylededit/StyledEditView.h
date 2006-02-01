#ifndef STYLED_EDIT_VIEW_H
#define STYLED_EDIT_VIEW_H

#include <File.h>
#include <TextView.h>
#include <DataIO.h>

class StyledEditView : public BTextView {
public:
	StyledEditView(BRect viewframe, BRect textframe, BHandler *handler);
	virtual ~StyledEditView();

	virtual void Select(int32 start, int32 finish);
	virtual void FrameResized(float width, float height);
	
	virtual void Reset();
	virtual status_t GetStyledText(BPositionIO * stream);
	virtual status_t WriteStyledEditFile(BFile * file);
	
	virtual void SetEncoding(uint32 encoding);
	virtual uint32 GetEncoding() const;
protected:
	virtual void InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs = NULL);
	virtual void DeleteText(int32 start, int32 finish);
			
private:
	BHandler	*fHandler;
	BMessage	*fChangeMessage;
	BMessenger 	*fMessenger;
	bool		fSuppressChanges;
	uint32		fEncoding;
};

#endif // STYLED_EDIT_VIEW_H
