#ifndef __ZIPPO_SETTINGS_H__
#define __ZIPPO_SETTINGS_H__

#include <Message.h>
#include <Volume.h>
#include <String.h>

class ZippoSettings : public BMessage
{
public:
					ZippoSettings		();
					ZippoSettings		(BMessage a_message);
					~ZippoSettings		();
	
	status_t		SetTo		(const char * a_settings_filename, 
								 BVolume * a_volume				=	NULL,
								 directory_which a_base_dir		=	B_USER_SETTINGS_DIRECTORY,
								 const char * a_relative_path	=	NULL);

	status_t		InitCheck	(void);
	
	status_t		ReadSettings		();
	status_t		WriteSettings		();
			
private:

	status_t		GetSettingsFile		(BFile * a_settings_file, uint32 a_openmode);

	BVolume				m_volume;
	directory_which		m_base_dir;
	BString				m_relative_path;
	BString				m_settings_filename;

};

#endif // __ZIPPO_SETTINGS_H__
