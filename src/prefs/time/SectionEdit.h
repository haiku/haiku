#ifndef SECTIONEDIT_H
#define SECTIONEDIT_H

#include <Control.h>
#include <List.h>

class TSection {
	public:
		TSection(BRect frame);
		
		void SetBounds(BRect frame);
		
		 BRect Bounds() const;
		BRect Frame() const;
	private:
		BRect f_frame;
};


class TSectionEdit: public BControl {
	public:
		TSectionEdit(BRect frame, const char *name, uint32 sections);
		~TSectionEdit();
		
		virtual void AttachedToWindow();
		virtual void Draw(BRect);
		virtual void MouseDown(BPoint);
		virtual void KeyDown(const char *bytes, int32 numbytes);
		
		uint32 CountSections() const;
		int32 FocusIndex() const;
		BRect SectionArea() const;
	protected:
		virtual void InitView();
		//hooks
		virtual void DrawBorder();
		virtual void DrawSection(uint32 index, bool isfocus);
		virtual void DrawSeperator(uint32 index);
		virtual void Draw3DFrame(BRect frame, bool inset);
		
		virtual void SectionFocus(uint32 index);
		virtual void SectionChange(uint32 index, uint32 value);
		virtual void SetSections(BRect area);
		virtual void GetSeperatorWidth(uint32 *width);
		
		virtual void DoUpPress();
		virtual void DoDownPress();
		
		virtual void DispatchMessage();
		virtual void BuildDispatch(BMessage *) = 0;
		
		BBitmap *f_up;
		BBitmap *f_down;
		BList *f_sections;
		
		BRect f_uprect;
		BRect f_downrect;
		BRect f_sectionsarea;
		
		int32 f_focus;
		uint32 f_sectioncount;
		uint32 f_holdvalue;
		
		bool f_showfocus;
};

#endif
