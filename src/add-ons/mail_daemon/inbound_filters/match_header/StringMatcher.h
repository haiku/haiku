//--------This file shamelessly stolen from Jeremy Friesner's excellent MUSCLE---------
/* This file is Copyright 2001 Level Control Systems.  See the included LICENSE.txt file for details. */

#ifndef STRINGMATCHER_H
#define STRINGMATCHER_H

#include <sys/types.h>

#if (defined(__BEOS__) || defined(__HAIKU__))
# if __POWERPC__
#  include "regex.h"  // use included regex if system doesn't provide one
# else
#  include <regex.h>
# endif
#else
# include <regex.h>
#endif

class BString;
#define PortableString BString

////////////////////////////////////////////////////////////////////////////
//
// NOTE:  This class is based on the psStringMatcher v1.3 class 
//        developed by Lars Jørgen Aas <larsa@tihlde.hist.no> for the
//        Prodigal Software File Requester.  Used by permission.
//
////////////////////////////////////////////////////////////////////////////


/** A utility class for doing globbing or regular expression matching.  (A thin wrapper around the C regex calls) */
class StringMatcher 
{
public:
    /** Default Constructor. */
    StringMatcher();

    /** A constructor that sets the simple expression.
     *  @param matchString the wildcard pattern or regular expression to match with
     */
    StringMatcher(const char * matchString);
    
    /** Destructor */
    ~StringMatcher();

    /** 
     * Set a new wildcard pattern or regular expression for this StringMatcher to use in future Match() calls.
     * @param expression The new globbing pattern or regular expression to match with.
     * @param isSimpleFormat If you wish to use the formal regex syntax, 
     *                       instead of the simple syntax, set isSimpleFormat to false.
     * @return True on success, false on error (e.g. expression wasn't parsable, or out of memory)
     */
    bool SetPattern(const char * const expression, bool isSimpleFormat=true);
    
    /** Returns true iff (string) is matched by the current expression.
     * @param string a string to match against using our current expression.
     * @return true iff (string) matches, false otherwise.
     */
    bool Match(const char *string) const;
    
private:
    bool _regExpValid;
    regex_t _regExp;
}; 

// Some regular expression utility functions

/** Puts a backslash in front of any char in (str) that is "special" to the regex pattern matching.
 *  @param str The string to check for special regex chars and possibly modify to escape them.
 */
void EscapeRegexTokens(PortableString & str);

/** Returns true iff any "special" chars are found in (str).
 *  @param str The string to check for special regex chars.
 *  @return True iff any special regex chars were found in (str).
 */
bool HasRegexTokens(const char * str);

/** Returns true iff (c) is a regular expression "special" char.
 *  @param c an ASCII char
 *  @return true iff (c) is a special regex char.
 */
bool IsRegexToken(char c);

/** Given a regular expression, makes it case insensitive by
 *  replacing every occurance of a letter with a upper-lower combo,
 *  e.g. Hello -> [Hh][Ee][Ll][Ll][Oo]
 *  @param str a string to check for letters, and possibly modify to make case-insensitive
 *  @return true iff anything was changed, false if no changes were necessary.
 */
bool MakeRegexCaseInsensitive(PortableString & str);


#endif
