#ifndef _idtag_h_
#define _idtag_h_

#include <sys/types.h>

class BPositionIO;
class StreamInfo;

/* D E F I N E S */

#define NO_GENRES      148

// APE Tag item names
#define APE_TAG_FIELD_TITLE             "Title"
#define APE_TAG_FIELD_SUBTITLE          "Subtitle"
#define APE_TAG_FIELD_ARTIST            "Artist"
#define APE_TAG_FIELD_ALBUM             "Album"
#define APE_TAG_FIELD_DEBUTALBUM        "Debut Album"
#define APE_TAG_FIELD_PUBLISHER         "Publisher"
#define APE_TAG_FIELD_CONDUCTOR         "Conductor"
#define APE_TAG_FIELD_COMPOSER          "Composer"
#define APE_TAG_FIELD_COMMENT           "Comment"
#define APE_TAG_FIELD_YEAR              "Year"
#define APE_TAG_FIELD_RECORDDATE        "Record Date"
#define APE_TAG_FIELD_RECORDLOCATION    "Record Location"
#define APE_TAG_FIELD_TRACK             "Track"
#define APE_TAG_FIELD_GENRE             "Genre"
#define APE_TAG_FIELD_COVER_ART_FRONT   "Cover Art (front)"
#define APE_TAG_FIELD_NOTES             "Notes"
#define APE_TAG_FIELD_LYRICS            "Lyrics"
#define APE_TAG_FIELD_COPYRIGHT         "Copyright"
#define APE_TAG_FIELD_PUBLICATIONRIGHT  "Publicationright"
#define APE_TAG_FIELD_FILE              "File"
#define APE_TAG_FIELD_MEDIA             "Media"
#define APE_TAG_FIELD_EANUPC            "EAN/UPC"
#define APE_TAG_FIELD_ISRC              "ISRC"
#define APE_TAG_FIELD_RELATED_URL       "Related"
#define APE_TAG_FIELD_ABSTRACT_URL      "Abstract"
#define APE_TAG_FIELD_BIBLIOGRAPHY_URL  "Bibliography"
#define APE_TAG_FIELD_BUY_URL           "Buy URL"
#define APE_TAG_FIELD_ARTIST_URL        "Artist URL"
#define APE_TAG_FIELD_PUBLISHER_URL     "Publisher URL"
#define APE_TAG_FIELD_FILE_URL          "File URL"
#define APE_TAG_FIELD_COPYRIGHT_URL     "Copyright URL"
#define APE_TAG_FIELD_INDEX             "Index"
#define APE_TAG_FIELD_INTROPLAY         "Introplay"
#define APE_TAG_FIELD_MJ_METADATA       "Media Jukebox Metadata"
#define APE_TAG_FIELD_DUMMY             "Dummy"

enum tag_t {
    auto_tag    =  -1,
    no_tag      =   0,
    ID3v1_tag   =   1,
    APE1_tag    =   2,
    APE2_tag    =   3,
    Ogg_tag     =   4,
    ID3v2       =  16,
    guessed_tag = 255,
};

// reliability of tag information
enum tag_rel {
    guess       =  1,
    ID3v1       = 10,
    APE1        = 19,
    APE2        = 20
};

/* V A R I A B L E S */
//extern const char*  GenreList [];   // holds the genres available for ID3

/* F U N C T I O N S */

// Function that tries to guess tag information from filename
//int GuessTag ( const char* filename, StreamInfo* Info );

// Read tag data
//int   ReadID3v1Tag  ( FILE* fp, StreamInfo* Info );                         // reads ID3v1 tag
//int   ReadAPE1Tag   ( FILE* fp, StreamInfo* Info );                         // reads APE 1.0 tag
//int   ReadAPE2Tag   ( FILE* fp, StreamInfo* Info );                         // reads APE 2.0 tag
//int   ReadTags      ( FILE* fp, StreamInfo* Info );                         // reads all tags from file


// Calculates start of file
long  JumpID3v2     ( BPositionIO *file );
/*
// add new field to tagdata
int NewTagField ( const char* item, const size_t itemsize, const char* value, const size_t valuesize, const unsigned int flags, StreamInfo* Info, const enum tag_rel reliability );

// replace old value in tagdata field
int ReplaceTagField ( const char* value, const size_t valuesize, const unsigned int flags, StreamInfo* Info, size_t itemnum, const enum tag_rel reliability );

// Free all tag fields from memory
//void  FreeTagFields ( StreamInfo* Info );//obsolete, use StreamInfo::Clear()

// insert item to tagdata. calls either new or replace
int InsertTagField ( const char* item, size_t itemsize, const char* value, size_t valuesize, const unsigned int flags, StreamInfo* Info );

// insert item to tagdata but only if reliability > previous tag's reliability
int InsertTagFieldLonger ( const char* item, size_t itemsize, const char* value, size_t valuesize, const unsigned int flags, StreamInfo* Info, const enum tag_rel reliability );

// Copy item to dest, maximum length to copy is count
int   CopyTagValue  ( char* dest, const char* item, const StreamInfo* Info, size_t count );

// Return pointer to item or NULL on error
char* TagValue      ( const char* item, const StreamInfo* Info );

int  GenreToInteger ( const char* GenreStr);
*/
#endif
