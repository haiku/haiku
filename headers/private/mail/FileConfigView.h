#ifndef FILE_CONFIG_VIEW
#define FILE_CONFIG_VIEW
/* FileConfigView - a file configuration view for filters
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <View.h>
#include <FilePanel.h>


class BTextControl;
class BButton;

class BFileControl : public BView
{
	public:
		BFileControl(BRect rect,const char *name,const char *label,const char *pathOfFile = NULL,uint32 flavors = B_DIRECTORY_NODE);
		~BFileControl();

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *msg);

		void SetText(const char *pathOfFile);
		const char *Text() const;

		void SetEnabled(bool enabled);

		virtual	void GetPreferredSize(float *width, float *height);

	private:
		BTextControl	*fText;
		BButton			*fButton;

		BFilePanel		*fPanel;
		
		uint32			_reserved[5];
};

class BMailFileConfigView : public BFileControl
{
	public:
		BMailFileConfigView(const char *label,const char *name,bool useMeta = false,const char *defaultPath = NULL,uint32 flavors = B_DIRECTORY_NODE);

		void SetTo(const BMessage *archive, BMessage *metadata);
		virtual	status_t Archive(BMessage *into, bool deep = true) const;

	private:
		BMessage	*fMeta;
		bool		fUseMeta;
		const char	*fName;

		uint32			_reserved[5];
};

#endif	/* FILE_CONFIG_VIEW */
