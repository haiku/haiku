#ifndef AUDIO_TAG_ANALYSER_H
#define AUDIO_TAG_ANALYSER_H


#include "IndexServerAddOn.h"


class AudioTagAnalyser : public FileAnalyser {
public:
								AudioTagAnalyser(BString name,
									const BVolume& volume);

			status_t			InitCheck();

			void				AnalyseEntry(const entry_ref& ref);
};


class AudioTagAddOn : public IndexServerAddOn {
public:
								AudioTagAddOn(image_id id, const char* name);

			FileAnalyser*		CreateFileAnalyser(const BVolume& volume);
};

#endif
