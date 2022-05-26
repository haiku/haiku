FontServer class
################

The FontServer provides the base functionality for providing font
support for the rest of the system and insulates the rest of the server
from having to deal too much with FreeType.

Member Functions
================

+-----------------------------------+-----------------------------------+
| FontServer(void)                  | ~FontServer(void)                 |
+-----------------------------------+-----------------------------------+
| void Lock(void)                   | void Unlock(void)                 |
+-----------------------------------+-----------------------------------+
| void SaveList(void)               | status_t ScanDirectory(const char |
|                                   | \*path)                           |
+-----------------------------------+-----------------------------------+
| FontStyle \*GetFont(font_family   | FontInstance                      |
| family, font_style face)          | \*GetInstance(font_family family, |
|                                   | font_style face, int16 size,      |
|                                   | int16 rotation, int16 shear)      |
+-----------------------------------+-----------------------------------+
| int32 CountFamiles(void)          | status_t IsInitialized(void)      |
+-----------------------------------+-----------------------------------+
| int32 CountStyles(font_family     | FontStyle \*GetStyle(font_family  |
| family)                           | family, font_style style)         |
+-----------------------------------+-----------------------------------+
| void RemoveFamily(const char      | FontFamily \*_FindFamily(const    |
| \*family)                         | char \*name)                      |
+-----------------------------------+-----------------------------------+
| ServerFont \*GetSystemPlain(void) | ServerFont \*GetSystemBold(void)  |
+-----------------------------------+-----------------------------------+
| ServerFont \*GetSystemFixed(void) | bool SetSystemPlain(const char    |
|                                   | \*family, const char \*style,     |
|                                   | float size)                       |
+-----------------------------------+-----------------------------------+
| void RemoveUnusedFamilies(void)   | bool FontsNeedUpdated(void)       |
+-----------------------------------+-----------------------------------+

FontServer(void)
----------------

1. Create access semaphore
2. Call FT_Init_FreeType()
3. If no error initializing the FreeType library, set init flag to true

~FontServer(void)
-----------------

1. Call FT_Done_FreeType()

void Lock(void), void Unlock(void)
----------------------------------

These functions simply acquire and release the internal access
semaphore.

void SaveList(void)
-------------------

Saves the list of all scanned and valid font families and styles to
disk


1. create a BMessage for storing font family data (hereafter, the font message)
2. create a BMessage for storing a list of font family messages (hereafter, the list message)
3. create a boolean tuned flag and a boolean fixed flag
4. Iterate through all families

   A. for each family, get its name and add its name to the font message as "name"
   B. iterate through the families styles

      a. get the style's name, and if valid, add it to the font message as "styles"
      b. if IsTuned and IsScalable, set the tuned flag to true
      c. if IsFixedWidth, set the fixed flag to true

   C. if the tuned flag is set, add a boolean true to the font message as "tuned"
   D. if the fixed flag is set, add a boolean true to the font message as "fixed"
   E. add the font message to the list message as "family"
   F. empty the font message

5. Create a BFile from the path definition SERVER_FONT_LIST for Read/Write, creating the file if nonexistent and erasing any existing one
6. Flatten the list message to the created BFile object
7. Set the needs_update flag to false


status_t ScanDirectory(const char \*path)
-----------------------------------------

ScanDirectory is where the brunt of the work of the FontServer is done: scan the directory of all fonts which can be loaded.

1. Make a BDirectory object pointer at the path parameter. If the init code is not B_OK, return it.
2. Enter a while() loop, iterating through each entry in the given directory, executing as follows:

   a. Ensure that the entry is not '.' or '..'
   b. Call FT_New_Face on the entry's full path
   c. If a valid FT_Face is returned, iterate through to see if there are any supported character mappings in the current entry.
   d. If there are no supported character mappings, dump the supported mappings to debug output, call FT_Done_Face(), and continue to the next entry
   e. See if the entry's family has been added to the family list. If it hasn't, create one and add it.
   f. Check to see if the font's style has been added to its family. If so, call FT_Done_Face, and continue to the next entry
   g. If the style has not been added, create a new SFont for that family and face, increment the font count, and continue to the next entry.

3. set the needs_update flag to true
4. Return B_OK

Supported character mappings are Windows and Apple Unicode, Windows
symbol, and Apple Roman character mappings, in order of preference
from first to last.

FontStyle \*GetFont(font_family family, font_style face)
--------------------------------------------------------

Returns an FontStyle object for the specified family and style or NULL
if not found.

1. Call \_FindFamily() for the given family
2. If non-NULL, call its FindStyle() method
3. Return the result

FontInstance \*GetInstance(font_family family, font_style face, int16 size, int16 rotation, int16 shear)
--------------------------------------------------------------------------------------------------------

Returns a usable instance of a specified font object with specified
properties.


1. Duplicates and performs the code found in GetFont
2. Assuming that the FontStyle object is non-NULL, it calls its GetInstance method and returns the result.


int32 CountFamilies(void)
-------------------------

Returns the number of valid font families available to the system.


1. Return the number of items in the family list


status_t IsInitialized(void)
----------------------------

Returns the initialization status variable


int32 CountStyles(font_family family)
-------------------------------------

Returns the number of styles available for a given font family.


1. Call \_FindFamily() to get the appropriate font family
2. If non-NULL, call its return the result of its CountStyles method

FontStyle \*GetStyle(font_family family, font_style style)
----------------------------------------------------------

Gets the FontStyle object of the family, style, and flags.

1. Call \_FindFamily() to get the appropriate font family
2. If non-NULL, call the family's GetStyle method on the font_style parameter and return the result
3. If family is NULL, return NULL

void RemoveFamily(const char \*family)
--------------------------------------

Removes a font family from the family list

1. Look up font family in the family list via \_FindFamily()
2. If it exists, delete it

FontFamily \*_FindFamily(const char \*name)
-------------------------------------------

Looks up a FontFamily object based on its family name. Returns NULL if not found.

1. Call the family list's find() method.
2. Return the appropriate FontFamily object or NULL if not found.

ServerFont \*GetSystemPlain(void), ServerFont \*GetSystemBold(void), ServerFont \*GetSystemFixed(void)
------------------------------------------------------------------------------------------------------

These return a copy of a pointer to the system-wide ServerFont objects
which represent the appropriate system font settings. It is the
responsibility of the caller to delete the object returned. NULL is
returned if no setting has been set for a particular system font.


bool SetSystemPlain(const char \*family, const char \*style, float size)
------------------------------------------------------------------------

bool SetSystemBold(const char \*family, const char \*style, float size)
-----------------------------------------------------------------------

bool SetSystemFixed(const char \*family, const char \*style, float size)
------------------------------------------------------------------------


The system fonts settings may be set via these calls by specifying the
family, style, and size. They return true if everything worked out ok
and false if not. Settings are not changed if false is returned.

1. Call \_FindFamily on the family parameter. if NULL, return false
2. Call the family's GetStyle member. if NULL, return false
3. if the appropriate system font pointer is non-NULL, delete it
4. call the style's Instantiate member with the size parameter


void RemoveUnusedFamilies(void)
-------------------------------

The purpose of this function is to allow for a complete rescan of the
fonts in the appropriate directories.


1. Iterate through the family list

   A. Get a family
   B. if it has no dependents, remove it from the list and delete it

2. Set the needs_update flag to true


bool FontsNeedUpdated(void)
---------------------------

Returns the value of the needs_update flag
