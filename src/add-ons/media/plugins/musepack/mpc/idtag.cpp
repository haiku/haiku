#include <DataIO.h>

#include <stdio.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>
#include "in_mpc.h"
#include "idtag.h"
#include "mpc_dec.h"

static const char ListSeparator[] = { "; " };

/* V A R I A B L E S */
static const char*  GenreList [] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
    "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
    "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
    "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
    "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
    "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
    "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
    "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
    "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
    "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
    "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
    "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
    "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
    "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
    "Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
    "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
    "SynthPop",
};


// free tagdata from memory
void
StreamInfo::Clear()
{
    size_t  i;

    if ( tagitems != NULL ) {
        for ( i = 0; i < tagitem_count; i++ ) {
            if ( tagitems[i].Item  ) free ( tagitems[i].Item  );
            if ( tagitems[i].Value ) free ( tagitems[i].Value );
        }

        free ( tagitems );
        tagitems = NULL;
    }

    if ( filename ) {
        free ( filename );
        filename = NULL;
    }

    tagitem_count = 0;
    tagtype       = (tag_t)0;
}


StreamInfo::StreamInfo ()
{
    memset ( &simple, 0, sizeof (simple) );
    tagitem_count = 0;
    tagtype       = (tag_t)0;
    tagitems      = NULL;
    filename      = NULL;
}


void
StreamInfo::SetFilename ( const char *fn )
{
    if ( filename ) free ( filename );
    filename = strdup ( fn );
}


const char *
StreamInfo::GetFilename ()
{
    return filename ? filename : "";
}


// return pointer to value in tag field
static char *
TagValue(const char* item, const StreamInfo* Info)
{
    size_t  i;

    for ( i = 0; i < Info->tagitem_count; i++ ) {
        // Are items case sensitive?
        if (!strcasecmp(item, Info->tagitems[i].Item))
            return (char*)Info->tagitems[i].Value;
    }

    return 0;
}


// add new field to tagdata
static int
NewTagField(const char* item, const size_t itemsize, const char* value,
	const size_t valuesize, const unsigned int flags, StreamInfo* Info,
	const enum tag_rel reliability)
{
    size_t itemnum = Info->tagitem_count++;

    // no real error handling yet
    Info->tagitems = (TagItem*) realloc ( Info->tagitems, Info->tagitem_count * sizeof (TagItem) );
    if ( Info->tagitems == NULL ) {
        Info->tagitem_count = 0;
        return 1;
    }
    Info->tagitems[itemnum].Item  = (char*)malloc ( itemsize  + 1 );
    if ( Info->tagitems[itemnum].Item == NULL ) {                           // couldn't allocate memory, revert to original items
        Info->tagitems = (TagItem*) realloc ( Info->tagitems, --Info->tagitem_count * sizeof (TagItem) );
        return 1;
    }
    Info->tagitems[itemnum].Value = (char*)malloc ( valuesize + 1 );
    if ( Info->tagitems[itemnum].Value == NULL ) {                          // couldn't allocate memory, revert to original items
        free ( Info->tagitems[itemnum].Item );
        Info->tagitems = (TagItem*) realloc ( Info->tagitems, --Info->tagitem_count * sizeof (TagItem) );
        return 1;
    }
    memcpy ( Info->tagitems[itemnum].Item , item , itemsize  );
    memcpy ( Info->tagitems[itemnum].Value, value, valuesize );

    Info->tagitems[itemnum].Item [itemsize]  = '\0';
    Info->tagitems[itemnum].Value[valuesize] = '\0';

    Info->tagitems[itemnum].ItemSize         = itemsize;
    Info->tagitems[itemnum].ValueSize        = valuesize;
    Info->tagitems[itemnum].Flags            = flags;
    Info->tagitems[itemnum].Reliability      = reliability;

    return 0;
}


// replace old value in tagdata field
static int
ReplaceTagField(const char* value, const size_t valuesize, const unsigned int flags,
	StreamInfo* Info, size_t itemnum, const enum tag_rel reliability)
{
    // no real error handling yet
    free ( Info->tagitems[itemnum].Value );
    Info->tagitems[itemnum].Value = (char*)malloc ( valuesize + 1 );
    if ( Info->tagitems[itemnum].Value == NULL )                            // couldn't allocate memory
        return 1;
    memcpy ( Info->tagitems[itemnum].Value, value, valuesize );

    Info->tagitems[itemnum].Value[valuesize] = '\0';
    Info->tagitems[itemnum].ValueSize        = valuesize;
    Info->tagitems[itemnum].Flags            = flags;
    Info->tagitems[itemnum].Reliability      = reliability;
    return 0;
}


#if 0
// insert item to tagdata. calls either new or replace
static int
InsertTagField(const char* item, size_t itemsize, const char* value,
	size_t valuesize, const unsigned int flags, StreamInfo* Info)
{
    size_t  i;

    if ( itemsize  == 0 ) itemsize  = strlen ( item  );                     // autodetect size
    if ( valuesize == 0 ) valuesize = strlen ( value );                     // autodetect size

    for ( i = 0; i < Info->tagitem_count; i++ ) {
        // are items case sensitive?
        if (!strcasecmp(item, Info->tagitems[i].Item))               // replace value of first item found
            return ReplaceTagField ( value, valuesize, flags, Info, i, (tag_rel)0 );
    }

    return NewTagField ( item, itemsize, value, valuesize, flags, Info, (tag_rel)0 );// insert new field
}
#endif


// insert item to tagdata but only if reliability > previous tag's reliability
static int
InsertTagFieldLonger(const char* item, size_t itemsize, const char* value,
	size_t valuesize, const unsigned int flags, StreamInfo* Info,
	const enum tag_rel reliability)
{
    size_t  i;

    if ( itemsize  == 0 ) itemsize  = strlen ( item  );                     // autodetect size
    if ( valuesize == 0 ) valuesize = strlen ( value );                     // autodetect size

    for ( i = 0; i < Info->tagitem_count; i++ ) {
        // are items case sensitive?
        if (!strcasecmp(item, Info->tagitems[i].Item)) {
            if ( reliability > Info->tagitems[i].Reliability )              // replace value of first item found
                return ReplaceTagField ( value, valuesize, flags, Info, i, reliability );
            else
                return 0;
        }
    }

    return NewTagField ( item, itemsize, value, valuesize, flags, Info, reliability );  // insert new field
}

#if 0
// Convert UNICODE to UTF-8
// Return number of bytes written
static int
unicodeToUtf8(const uint16/*WCHAR*/* lpWideCharStr, char* lpMultiByteStr, int cwcChars)
{
    const unsigned short*   pwc = (unsigned short *)lpWideCharStr;
    unsigned char*          pmb = (unsigned char  *)lpMultiByteStr;
    const unsigned short*   pwce;
    size_t  cBytes = 0;

    if ( cwcChars >= 0 ) {
        pwce = pwc + cwcChars;
    } else {
        pwce = (unsigned short *)((size_t)-1);
    }

    while ( pwc < pwce ) {
        unsigned short  wc = *pwc++;

        if ( wc < 0x00000080 ) {
            *pmb++ = (char)wc;
            cBytes++;
        } else
        if ( wc < 0x00000800 ) {
            *pmb++ = (char)(0xC0 | ((wc >>  6) & 0x1F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        } else
        if ( wc < 0x00010000 ) {
            *pmb++ = (char)(0xE0 | ((wc >> 12) & 0x0F));
            cBytes++;
            *pmb++ = (char)(0x80 | ((wc >>  6) & 0x3F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        }
        if ( wc == L'\0' )
            return cBytes;
    }

    return cBytes;
}

// Convert UTF-8 coded string to UNICODE
// Return number of characters converted
static int
utf8ToUnicode(const char* lpMultiByteStr, uint16/*WCHAR*/* lpWideCharStr, int cmbChars)
{
    const unsigned char*    pmb = (unsigned char  *)lpMultiByteStr;
    unsigned short*         pwc = (unsigned short *)lpWideCharStr;
    const unsigned char*    pmbe;
    size_t  cwChars = 0;

    if ( cmbChars >= 0 ) {
        pmbe = pmb + cmbChars;
    } else {
        pmbe = (unsigned char *)((size_t)-1);
    }

    while ( pmb < pmbe ) {
        char            mb = *pmb++;
        unsigned int    cc = 0;
        unsigned int    wc;

        while ( (cc < 7) && (mb & (1 << (7 - cc)))) {
            cc++;
        }

        if ( cc == 1 || cc > 6 )                    // illegal character combination for UTF-8
            continue;

        if ( cc == 0 ) {
            wc = mb;
        } else {
            wc = (mb & ((1 << (7 - cc)) - 1)) << ((cc - 1) * 6);
            while ( --cc > 0 ) {
                if ( pmb == pmbe )                  // reached end of the buffer
                    return cwChars;
                mb = *pmb++;
                if ( ((mb >> 6) & 0x03) != 2 )      // not part of multibyte character
                    return cwChars;
                wc |= (mb & 0x3F) << ((cc - 1) * 6);
            }
        }

        if ( wc & 0xFFFF0000 )
            wc = L'?';
        *pwc++ = wc;
        cwChars++;
        if ( wc == L'\0' )
            return cwChars;
    }

    return cwChars;
}
#endif

// convert Windows ANSI to UTF-8
static int
ConvertANSIToUTF8(const char *ansi, char *utf8)
{
	// ToDo:
    return 0;
}

#if 0
static int
ConvertTagFieldToUTF8(StreamInfo *Info, size_t itemnum)
{
	// ToDo:
    return 0;
}
#endif

// replace list separator characters in tag field
static int
ReplaceListSeparator(const char* old_sep, const char* new_sep, StreamInfo* Info,
	size_t itemnum)
{
    unsigned char*  new_value;
    unsigned char*  p;
    size_t          os_len;
    size_t          ns_len;
    size_t          count;
    size_t          new_len;
    size_t          i;
    int             error;

    if ( Info->tagitems[itemnum].Flags & 1<<1 )                             // data in binary
        return 0;

    os_len = strlen ( old_sep );
    ns_len = strlen ( new_sep );
    if ( os_len == 0 ) os_len = 1;                                          // allow null character
    if ( ns_len == 0 ) ns_len = 1;

    if ( Info->tagitems[itemnum].Value     == NULL ||                       // nothing to do
         Info->tagitems[itemnum].ValueSize == 0 )
        return 0;

    count = 0;
    for ( i = 0; i < Info->tagitems[itemnum].ValueSize - os_len + 1; i++ ) {
        if ( memcmp ( Info->tagitems[itemnum].Value+i, old_sep, os_len ) == 0 )
            count++;
    }

    if ( count == 0 )
        return 0;

    new_len = Info->tagitems[itemnum].ValueSize - (count * os_len) + (count * ns_len);
    if ( (new_value = (unsigned char *)malloc ( new_len )) == NULL )
        return 1;

    p = new_value;
    for ( i = 0; i < Info->tagitems[itemnum].ValueSize; i++ ) {
        if ( i + os_len - 1 >= Info->tagitems[itemnum].ValueSize ||
             memcmp ( Info->tagitems[itemnum].Value+i, old_sep, os_len ) != 0 ) {
            *p++ = Info->tagitems[itemnum].Value[i];
        } else {
            memcpy ( p, new_sep, ns_len );
            p += ns_len;
            i += os_len - 1;
        }
    }

    error = ReplaceTagField ( (const char*)new_value, new_len, Info->tagitems[itemnum].Flags, Info, itemnum, Info->tagitems[itemnum].Reliability );

    free ( new_value );

    return error;
}


static int
GenreToInteger(const char *GenreStr)
{
    size_t  i;

    for ( i = 0; i < sizeof(GenreList) / sizeof(*GenreList); i++ ) {
        if (!strcasecmp(GenreStr, GenreList[i]))
            return i;
    }

    return -1;
}


static void
GenreToString(char *GenreStr, const unsigned int genre)
{
    GenreStr [0] = '\0';
    if ( genre < NO_GENRES )
        strcpy ( GenreStr, GenreList [genre] );
}


/*
 *  Copies src to dst. Copying is stopped at `\0' char is detected or if
 *  len chars are copied.
 *  Trailing blanks are removed and the string is `\0` terminated.
 */

static void
memcpy_crop(char* dst, const char* src, size_t len)
{
    size_t  i;

    for ( i = 0; i < len; i++ )
        if  ( src[i] != '\0' )
            dst[i] = src[i];
        else
            break;

    // dst[i] points behind the string contents
    while ( i > 0  &&  dst [i-1] == ' ' )
        i--;

    dst [i] = '\0';
}


#if 0
// replaces % sequences with real characters
static void
fix_percentage_sequences(char *string)
{
    char temp[PATH_MAX];
    char* t = temp;
    char* s = string;
    int value;
    int b1, b2; //, b3, b4;
    int v1, v2; //, v3, v4;

    do {
        value = *s++;

        if ( value == '%' ) {
            if ( *s != '\0' )   b1 = *s++;
            else                b1 = '\0';
            if ( *s != '\0' )   b2 = *s++;
            else                b2 = '\0';

            if ( ((b1 >= '0' && b1 <= '9') || (b1 >= 'A' && b1 <= 'F')) &&
                 ((b2 >= '0' && b2 <= '9') || (b2 >= 'A' && b2 <= 'F')) ) {

                if ( b1 <= '9' )  v1 = b1 - '0';
                else              v1 = b1 - 'A' + 10;
                if ( b2 <= '9' )  v2 = b2 - '0';
                else              v2 = b2 - 'A' + 10;

                if ( v1 == 0 && v2 == 0 ) {         // %00xx
                    /*
                    if ( *s != '\0' )   b1 = *s++;
                    else                b1 = '\0';
                    if ( *s != '\0' )   b2 = *s++;
                    else                b2 = '\0';
                    if ( *s != '\0' )   b3 = *s++;
                    else                b2 = '\0';
                    if ( *s != '\0' )   b4 = *s++;
                    else                b4 = '\0';

                    if ( b1 <= '9' )  v1 = b1 - '0';
                    else              v1 = b1 - 'A' + 10;
                    if ( b2 <= '9' )  v2 = b2 - '0';
                    else              v2 = b2 - 'A' + 10;
                    if ( b3 <= '9' )  v3 = b3 - '0';
                    else              v3 = b3 - 'A' + 10;
                    if ( b4 <= '9' )  v4 = b4 - '0';
                    else              v4 = b4 - 'A' + 10;

                    if ( v1 != 0 || v2 != 0 ) {
                        *t++ = ' ';                 // no multibyte support, unknown character
                    } else {                        // %0000+xxxx+
                        *t++ = ' ';

                        while ( *s && ((*s >= '0' && *s <= '9') || (*s >= 'A' && *s <= 'F')) )
                            s++;
                    }
                    */

                    *t++ = ' ';

                    while ( *s && ((*s >= '0' && *s <= '9') || (*s >= 'A' && *s <= 'F')) )
                        s++;
                } else {                            // %xx
                    *t++ = (v1 << 4) + v2;
                }
            } else {                                // %aa
                *t++ = '%';
                *t++ = b1;
                *t++ = b2;
            }
        } else {
            *t++ = value;
        }
    } while ( value != '\0' );

    strcpy ( string, temp );
}
#endif


// searches and reads a ID3v1 tag
int StreamInfo::ReadID3v1Tag(BPositionIO *stream)
{
/*    unsigned */char   tmp [128];
/*    unsigned */char   value [32];
    char            utf8[32*2];

    if (stream->Seek(simple.TagOffset - sizeof(tmp), SEEK_SET) < B_OK)
        return 0;
    if (stream->Read(tmp, sizeof(tmp)) != sizeof(tmp))
        return 0;
    // check for id3-tag
    if (memcmp(tmp, "TAG", 3) != 0)
        return 0;

    memcpy_crop ( value, tmp +  3, 30 );
    if ( value[0] != '\0' ) {
        ConvertANSIToUTF8 ( value, utf8 );
        InsertTagFieldLonger ( APE_TAG_FIELD_TITLE  , 0, utf8, 0, 0, this, ID3v1 );
    }
    memcpy_crop ( value, tmp + 33, 30 );
    if ( value[0] != '\0' ) {
        ConvertANSIToUTF8 ( value, utf8 );
        InsertTagFieldLonger ( APE_TAG_FIELD_ARTIST , 0, utf8, 0, 0, this, ID3v1 );
    }
    memcpy_crop ( value, tmp + 63, 30 );
    if ( value[0] != '\0' ) {
        ConvertANSIToUTF8 ( value, utf8 );
        InsertTagFieldLonger ( APE_TAG_FIELD_ALBUM  , 0, utf8, 0, 0, this, ID3v1 );
    }
    memcpy_crop ( value, tmp + 93,  4 );
    if ( value[0] != '\0' ) {
        ConvertANSIToUTF8 ( value, utf8 );
        InsertTagFieldLonger ( APE_TAG_FIELD_YEAR   , 0, utf8, 0, 0, this, ID3v1 );
    }
    memcpy_crop ( value, tmp + 97, 30 );
    if ( value[0] != '\0' ) {
        ConvertANSIToUTF8 ( value, utf8 );
        InsertTagFieldLonger ( APE_TAG_FIELD_COMMENT, 0, utf8, 0, 0, this, ID3v1 );
    }

    if ( tmp[125] == 0 ) {
        sprintf ( value, "%d", tmp[126] );
        if ( value[0] != '\0' && atoi (value) != 0 ) {
            ConvertANSIToUTF8 ( value, utf8 );
            InsertTagFieldLonger ( APE_TAG_FIELD_TRACK, 0, utf8, 0, 0, this, ID3v1 );
        }
    }
    GenreToString ( value, tmp[127] );
    if ( value[0] != '\0' ) {
        ConvertANSIToUTF8 ( value, utf8 );
        InsertTagFieldLonger ( APE_TAG_FIELD_GENRE  , 0, utf8, 0, 0, this, ID3v1 );
    }

    simple.TagOffset -= 128;
    tagtype = ID3v1_tag;

    return 0;
}


struct APETagFooterStruct {
    unsigned char   ID       [8];    // should equal 'APETAGEX'
    unsigned char   Version  [4];    // 1000 = version 1.0, 2000 = version 2.0
    unsigned char   Length   [4];    // complete size of the tag, including footer, excluding header
    unsigned char   TagCount [4];    // number of fields in the tag
    unsigned char   Flags    [4];    // tag flags (none currently defined)
    unsigned char   Reserved [8];    // reserved for later use
};


static unsigned long
Read_LE_Uint32 ( const unsigned char* p )
{
    return ((unsigned long)p[0] <<  0) |
           ((unsigned long)p[1] <<  8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}

/*static void
Write_LE_Uint32 ( unsigned char* p, const unsigned long value )
{
    p[0] = (unsigned char) (value >>  0);
    p[1] = (unsigned char) (value >>  8);
    p[2] = (unsigned char) (value >> 16);
    p[3] = (unsigned char) (value >> 24);
}
*/

int
StreamInfo::ReadAPE1Tag(BPositionIO *stream)
{
    unsigned long               vsize;
    unsigned long               isize;
    unsigned long               flags;
    /*unsigned */char*              buff;
    /*unsigned */char*              p;
    /*unsigned */char*              end;
    char*                       utf8;
    struct APETagFooterStruct   T;
    unsigned long               TagLen;
    unsigned long               TagCount;

    if (stream->Seek(simple.TagOffset - sizeof T, SEEK_SET) < B_OK)
        return 0;
    if (stream->Read(&T, sizeof T) != sizeof T)
        return 0;
    if ( memcmp ( T.ID, "APETAGEX", sizeof T.ID ) != 0 )
        return 0;
    if ( Read_LE_Uint32 (T.Version) != 1000 )
        return 0;
    TagLen = Read_LE_Uint32 (T.Length);
    if ( TagLen <= sizeof T )
        return 0;
    if (stream->Seek(simple.TagOffset - TagLen, SEEK_SET ) < B_OK)
        return 0;
    buff = (char *)malloc ( TagLen );
    if ( buff == NULL )
        return 1;
    if (stream->Read(buff, TagLen - sizeof(T)) != ssize_t(TagLen - sizeof(T))) {
        free(buff);
        return 0;
    }

    TagCount = Read_LE_Uint32 (T.TagCount);
    end = buff + TagLen - sizeof (T);
    for ( p = buff; p < end /*&& *p */ &&  TagCount--; ) {
        vsize = Read_LE_Uint32 ( (unsigned char *) p ); p += 4;
        flags = Read_LE_Uint32 ( (unsigned char *) p ); p += 4;
        isize = strlen(p);

        if ( vsize > 0 ) {
            if ( (utf8 = (char*)malloc ( (vsize + 1) * 3 )) == NULL ) {
                free ( buff );
                return 1;
            }
            ConvertANSIToUTF8 ( p + isize + 1, utf8 );
            InsertTagFieldLonger ( p, isize, utf8, 0, 0, this, APE1 );  // flags not used with APE 1.0
            free ( utf8 );
        }
        p += isize + 1 + vsize;
    }

    simple.TagOffset -= TagLen;
    tagtype = APE1_tag;

    free(buff);

    return 0;
}


int
StreamInfo::ReadAPE2Tag(BPositionIO *stream)
{
    unsigned long               vsize;
    unsigned long               isize;
    unsigned long               flags;
    /*unsigned */char*              buff;
    /*unsigned */char*              p;
    /*unsigned */char*              end;
    struct APETagFooterStruct   T;
    unsigned long               TagLen;
    unsigned long               TagCount;
    size_t                      i;

	if (stream->Seek(simple.TagOffset - sizeof(T), SEEK_SET) < B_OK)
        return 0;
    if (stream->Read(&T, sizeof(T)) != sizeof(T))
        return 0;
    if (memcmp(T.ID, "APETAGEX", sizeof(T.ID)) != 0)
        return 0;
    if (Read_LE_Uint32(T.Version) != 2000)
        return 0;
    TagLen = Read_LE_Uint32(T.Length);
    if (TagLen <= sizeof(T))
        return 0;
    if (stream->Seek(simple.TagOffset - TagLen, SEEK_SET) < B_OK)
        return 0;
    buff = (char *)malloc(TagLen);
    if (buff == NULL)
        return 1;
    if (stream->Read(buff, TagLen - sizeof(T)) != ssize_t(TagLen - sizeof(T))) {
        free(buff);
        return 0;
    }

    TagCount = Read_LE_Uint32(T.TagCount);
    end = buff + TagLen - sizeof(T);
    for (p = buff; p < end /*&& *p */ &&  TagCount--;) {
        vsize = Read_LE_Uint32((unsigned char *)p); p += 4;
        flags = Read_LE_Uint32((unsigned char *)p); p += 4;
        isize = strlen(p);

        if (vsize > 0)
            InsertTagFieldLonger(p, isize, p + isize + 1, vsize, flags, this, APE2);
        p += isize + 1 + vsize;
    }

    simple.TagOffset -= TagLen;
    tagtype = APE2_tag;

    free (buff);

    for (i = 0; i < tagitem_count; i++) {
        if (ReplaceListSeparator("\0", ListSeparator, this, i) != 0)
            return 2;
    }

    if (Read_LE_Uint32(T.Flags) & 1<<31) {       // Tag contains header
        simple.TagOffset -= sizeof (T);
    } else {                                        // Check if footer was incorrect
        if (stream->Seek(simple.TagOffset - sizeof(T), SEEK_SET ) < B_OK)
            return 0;
        if (stream->Read(&T, sizeof(T)) != sizeof(T))
            return 0;
        if (memcmp(T.ID, "APETAGEX", sizeof(T.ID)) != 0)
            return 0;
        if (Read_LE_Uint32(T.Version) != 2000)
            return 0;
        if (Read_LE_Uint32(T.Flags) & 1<<29 )     // This is header
            simple.TagOffset -= sizeof(T);
    }

    return 0;
}


bool
_validForID3(const char *string, int maxsize)
{
	const unsigned char* p = (unsigned char *)string;

	while (*p) {
		// only ASCII characters are reliable in ID3v1
		if (*p++ >= 0x80)
			return false;
	}

	return ( ((char*)p - string) <= maxsize );
}

// checks if ID3v1 tag can store all tag information
// return 0 if can, 1 if can't
int
CheckID3V1TagDataLoss(const StreamInfo *Info)
{
    const char *id3v1_items[] = {
        APE_TAG_FIELD_TITLE,        // 0
        APE_TAG_FIELD_ARTIST,       // 1
        APE_TAG_FIELD_ALBUM,        // 2
        APE_TAG_FIELD_YEAR,         // 3
        APE_TAG_FIELD_COMMENT,      // 4
        APE_TAG_FIELD_TRACK,        // 5
        APE_TAG_FIELD_GENRE         // 6
    };
    char*   value;
    size_t  i, j;

    if ( (value = TagValue ( APE_TAG_FIELD_TRACK, Info )) ) {
        char* p = value;
        while ( *p ) {
            if ( *p < '0' || *p > '9' ) return 1;                   // not a number
            p++;
        }
        if ( atoi (value) <   0 ||                                  // number won't fit in byte
             atoi (value) > 255 )
            return 1;
    }

    if ( ( value = TagValue ( APE_TAG_FIELD_GENRE, Info )) ) {
        if ( value[0] != '\0' && (unsigned int)GenreToInteger (value) >= NO_GENRES ) // non-standard genre
            return 1;
    }

    for ( i = 0; i < Info->tagitem_count; i++ ) {
        bool bFound = false;
        for ( j = 0; j < sizeof (id3v1_items) / sizeof (*id3v1_items); j++ ) {
            if (!strcasecmp(Info->tagitems[i].Item, id3v1_items[j])) {
                int maxlen;
                switch ( j ) {
                case 3:     // Year
                    maxlen =  4;
                    break;
                case 4:     // Comment
                    value = TagValue ( APE_TAG_FIELD_TRACK, Info );
                    if ( value && strcmp ( value, "" ) != 0 )
                        maxlen = 28;
                    else
                        maxlen = 30;
                    break;
                default:
                    maxlen = 30;
                    break;
                }
                if ( !_validForID3 ( Info->tagitems[i].Value, maxlen ) )
                    return 1;
                bFound = TRUE;
                break;
            }
        }
        if ( !bFound ) return 1;                                    // field not possible in ID3v1
    }

    return 0;
}


// scan file for all supported tags and read them
int
StreamInfo::ReadTags(BPositionIO *fp)
{
    off_t TagOffs;

    do {
        TagOffs = simple.TagOffset;
        ReadAPE1Tag(fp);
        ReadAPE2Tag(fp);
        ReadID3v1Tag(fp);
    } while (TagOffs != simple.TagOffset);

    return 0;
}


// searches for a ID3v2-tag and reads the length (in bytes) of it
// -1 on errors of any kind

long
JumpID3v2(BPositionIO *fp)
{
    unsigned char  tmp [10];
    unsigned int   Unsynchronisation;   // ID3v2.4-flag
    unsigned int   ExtHeaderPresent;    // ID3v2.4-flag
    unsigned int   ExperimentalFlag;    // ID3v2.4-flag
    unsigned int   FooterPresent;       // ID3v2.4-flag
    long           ret;

    fp->Read(tmp, sizeof(tmp));

    // check id3-tag
    if (memcmp(tmp, "ID3", 3) != 0)
        return 0;

    // read flags
    Unsynchronisation = tmp[5] & 0x80;
    ExtHeaderPresent  = tmp[5] & 0x40;
    ExperimentalFlag  = tmp[5] & 0x20;
    FooterPresent     = tmp[5] & 0x10;

    if ( tmp[5] & 0x0F )
        return -1;              // not (yet???) allowed
    if ( (tmp[6] | tmp[7] | tmp[8] | tmp[9]) & 0x80 )
        return -1;              // not allowed

    // read HeaderSize (syncsave: 4 * $0xxxxxxx = 28 significant bits)
    ret  = tmp[6] << 21;
    ret += tmp[7] << 14;
    ret += tmp[8] <<  7;
    ret += tmp[9]      ;
    ret += 10;
    if (FooterPresent)
        ret += 10;

    return ret;
}
