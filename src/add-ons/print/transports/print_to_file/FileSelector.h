#ifndef FILESELECTOR_H
#define FILESELECTOR_H

#include <InterfaceKit.h>
#include <StorageKit.h>

class FileSelector : public BWindow
{
	public:
		// Constructors, destructors, operators...

								FileSelector(void);
								~FileSelector(void);

		typedef BWindow 		inherited;

		// public constantes
		enum	{
			START_MSG			= 'strt',
			SAVE_INTO_MSG		= 'save'
		};
				
		// Virtual function overrides
	public:	
		virtual void 			MessageReceived(BMessage * msg);
		virtual bool 			QuitRequested();
		status_t 				Go(entry_ref * ref);

		// From here, it's none of your business! ;-)
	private:
		BEntry					m_entry;
		volatile status_t 		m_result;
		long 					m_exit_sem;
		BFilePanel *			m_save_panel;
};

#endif // FILESELECTOR_H

