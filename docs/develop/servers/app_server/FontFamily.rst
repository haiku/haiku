FontFamily class
################

FontFamily objects are used to tie together all related font styles.

Member Functions
================

FontFamily(const char \*name)
-----------------------------

1. Create and set internal name to the one passed to the constructor
2. Create the styles list


~FontFamily(void)
-----------------

1. delete the internal name
2. empty and delete the internal style list

const char \*GetName(void)
---------------------------

Returns the internal family name


void AddStyle(const char \*path, FT_Face face)
----------------------------------------------

Adds the style to the family.


1. Create the FontStyle object and add it to the style list.


void RemoveStyle(const char \*style)
------------------------------------

Removes the style from the FontFamily object.


1. Call GetStyle on the given pointer
2. If non-NULL, delete the object
3. If the style list is now empty, ask the FontServer to remove it from the family list

FontStyle \*GetStyle(const char \*style)
----------------------------------------

Looks up a FontStyle object based on its style name. Returns NULL if
not found.


1. Iterate through the style list

   a. compare style to each FontStyle object's GetName method and return the object if they are the same

2. If all items have been checked and nothing has been returned, return NULL


const char \*GetStyle(int32 index)
----------------------------------

Returns the name of the style at index


1. Get the FontStyle item at index obtained via the style list's
ItemAt call


int32 CountStyles(void)
-----------------------

Returns the number of items in the style list and, thus, the number of
styles in the family


bool HasStyle(const char \*stylename)
-------------------------------------

Returns true if the family has a style with the name stylename


1. Call GetStyle on the style name and return false if NULL, true if
not.

