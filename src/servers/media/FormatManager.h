#ifndef _FORMAT_MANAGER_H
#define _FORMAT_MANAGER_H

class FormatManager
{
public:
				FormatManager();
				~FormatManager();
				
	void		LoadState();
	void		SaveState();
	
	status_t 	GetFormatForDescription(media_format *out_format,
										const media_format_description &in_description);

	status_t 	GetDescriptionForFormat(media_format_description *out_description,
										const media_format &in_format,
										media_format_family in_family);						 
};

#endif // _FORMAT_MANAGER_H
