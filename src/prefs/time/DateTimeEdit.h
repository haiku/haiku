#ifndef DATETIME_H
#define DATETIME_H

#include "SectionEdit.h"

// TSection descendent to hold uint32 value
class TDateTimeSection: public TSection {
	public:
		TDateTimeSection(BRect frame, uint32 data = 0);
		~TDateTimeSection();
		
		uint32 Data() const;
		void SetData(uint32 data);
	private:
		uint32 f_data;
};

// TSectionEdit descendent to edit time
class TTimeEdit: public TSectionEdit {
	public:
		TTimeEdit(BRect frame, const char *name, uint32 sections);
		~TTimeEdit();
		
		virtual void InitView();
		virtual void DrawSection(uint32 index, bool isfocus);
		virtual void DrawSeperator(uint32 index);
		
		virtual void SetSections(BRect area);
		virtual void SectionFocus(uint32 index);
		virtual void GetSeperatorWidth(uint32 *width);
		
		void CheckRange();
		
		virtual void DoUpPress();
		virtual void DoDownPress();
		
		virtual void BuildDispatch(BMessage *message);
		
		void SetTo(uint32 hour, uint32 minute, uint32 second);
};


/* DATE TSECTIONEDIT */

// TSectionEdit descendent to edit Date
class TDateEdit: public TSectionEdit {
	public:
		TDateEdit(BRect frame, const char *name, uint32 sections);
		~TDateEdit();
		
		virtual void InitView();
		virtual void DrawSection(uint32 index, bool isfocus);
		virtual void DrawSeperator(uint32 index);
		
		virtual void SetSections(BRect area);
		virtual void SectionFocus(uint32 index);
		virtual void GetSeperatorWidth(uint32 *width);
		
		void CheckRange();
		
		virtual void DoUpPress();
		virtual void DoDownPress();
		
		virtual void BuildDispatch(BMessage *message);
		
		void SetTo(uint32 hour, uint32 minute, uint32 second);
};


#endif //DATETIME_H
