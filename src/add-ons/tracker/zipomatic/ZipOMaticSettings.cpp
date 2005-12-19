// license: public domain
// authors: jonas.sundstrom@kirilla.com

//
// TODO: proper locking      <<---------
// 

#include <Debug.h>

#include <VolumeRoster.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <File.h>

#include "ZipOMaticMisc.h"
#include "ZipOMaticSettings.h"

ZippoSettings::ZippoSettings()
:	BMessage		 	(),
	m_volume			(),
	m_base_dir			(B_USER_SETTINGS_DIRECTORY),
	m_relative_path		(),
	m_settings_filename	()
{
	PRINT(("ZippoSettings()\n"));
}

ZippoSettings::ZippoSettings(BMessage a_message)
:	BMessage			(a_message),
	m_volume			(),
	m_base_dir			(B_USER_SETTINGS_DIRECTORY),
	m_relative_path		(),
	m_settings_filename	()
{
	PRINT(("ZippoSettings(a_message)\n"));
}

ZippoSettings::~ZippoSettings()	
{
	m_volume.Unset();
}

status_t
ZippoSettings::SetTo (const char *		a_settings_filename, 
					 BVolume *			a_volume,
					 directory_which	a_base_dir,
					 const char *		a_relative_path)
{
	status_t	status	=	B_OK;

	// copy to members
	m_base_dir			=	a_base_dir;
	m_relative_path		=	a_relative_path;
	m_settings_filename	=	a_settings_filename;
	
	// sanity check
	if (a_volume == NULL)
	{
		BVolumeRoster  volume_roster;
		volume_roster.GetBootVolume(& m_volume);
	}
	else
		m_volume  =  *(a_volume);
	
	status = m_volume.InitCheck();
	if (status != B_OK)
		return status;
	
	// InitCheck
	return InitCheck();
}

status_t	
ZippoSettings::InitCheck	(void)
{
	BFile  settings_file;

	return GetSettingsFile	(& settings_file, B_READ_ONLY | B_CREATE_FILE);
}

status_t	
ZippoSettings::GetSettingsFile	(BFile * a_settings_file, uint32 a_openmode)
{
	status_t	status	=	B_OK;
	BPath		settings_path;
	
	status	=	find_and_create_directory(m_base_dir, & m_volume, m_relative_path.String(), & settings_path);
	if (status != B_OK)
		return status;
		
	status	=	settings_path.Append (m_settings_filename.String());
	if (status != B_OK)
		return status;
	
	status	=	a_settings_file->SetTo (settings_path.Path(), a_openmode);
	if (status != B_OK)
		return status;
		
	return B_OK;
}

status_t
ZippoSettings::ReadSettings	(void)
{
	PRINT(("ZippoSettings::ReadSettings()\n"));

	status_t	status	=	B_OK;
	BFile		settings_file;
	
	status 	=	GetSettingsFile	(& settings_file, B_READ_ONLY);
	if (status != B_OK)
		return status;
	
	status	=	Unflatten(& settings_file);
	if (status != B_OK)
		return status;

	return B_OK;
}

status_t
ZippoSettings::WriteSettings	(void)
{
	PRINT(("ZippoSettings::WriteSettings()\n"));

	status_t	status	=	B_OK;
	BFile		settings_file;
	
	status 	=	GetSettingsFile	(& settings_file, B_WRITE_ONLY | B_ERASE_FILE);
	if (status != B_OK)
		return status;
	
	status	=	Flatten(& settings_file);
	if (status != B_OK)
		return status;

	return B_OK;
}

