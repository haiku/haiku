#include "AudioTagAnalyser.h"

#include <new>

#include <Path.h>

#include <audioproperties.h>
#include <tag.h>
#include <fileref.h>


AudioTagAnalyser::AudioTagAnalyser(BString name, const BVolume& volume)
	:
	FileAnalyser(name, volume)
{
	
}


status_t
AudioTagAnalyser::InitCheck()
{
	return B_OK;
}


#include <stdio.h>
void
AudioTagAnalyser::AnalyseEntry(const entry_ref& ref)
{
	BPath path(&ref);

	TagLib::FileRef tagFile(path.Path());
	TagLib::Tag* tag = tagFile.tag();
	if (!tag)
		return;

	TagLib::String artist = tag->artist();
	TagLib::String title = tag->title();
	TagLib::String album = tag->album();
	printf("artist: %s, title: %s, album: %s\n", artist.toCString(),
		   title.toCString(), album.toCString());

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	const char* cArtist = artist.toCString(true);
	file.WriteAttr("Media:Artist", B_STRING_TYPE, 0, cArtist, strlen(cArtist));
	const char* cTitle = title.toCString(true);
	file.WriteAttr("Media:Title", B_STRING_TYPE, 0, cTitle, strlen(cTitle));
	const char* cAlbum = album.toCString(true);
	file.WriteAttr("Media:Album", B_STRING_TYPE, 0, cAlbum, strlen(cAlbum));
}


AudioTagAddOn::AudioTagAddOn(image_id id, const char* name)
	:
	IndexServerAddOn(id, name)
{
	
}


FileAnalyser*
AudioTagAddOn::CreateFileAnalyser(const BVolume& volume)
{
	return new (std::nothrow)AudioTagAnalyser(Name(), volume);
}


extern "C" IndexServerAddOn* (instantiate_index_server_addon)(image_id id,
	const char* name)
{
	return new (std::nothrow)AudioTagAddOn(id, name);
}
