//--------This file shamelessly stolen from Jeremy Friesner's excellent MUSCLE---------
/* This file is Copyright 2000 Level Control Systems.  See the included LICENSE.txt file for details. */

#include <new>
#include <stdio.h>

#include "StringMatcher.h"

#include <string.h>
#include <String.h>

StringMatcher::StringMatcher() : _regExpValid(false)
{
   // empty
}

StringMatcher :: StringMatcher(const char * str) : _regExpValid(false)
{
   SetPattern(str);
}

StringMatcher::~StringMatcher()
{
   if (_regExpValid) regfree(&_regExp);
}

bool StringMatcher::SetPattern(const char * str, bool isSimple)
{
   PortableString pattern;

   if (isSimple)
   {
      pattern = "^\\(";

      bool escapeMode = false;
      for (const char * ptr = str; *ptr != '\0'; ptr++)
      {
         if (escapeMode)
         {
            escapeMode = false;
            switch(*ptr)
            {
               case ',': case '|': case '(': case ')': case '?':
                  pattern += *ptr;
               break;

               default:
                  pattern += '\\';
                  pattern += *ptr;
               break;
            }
         }
         else
         {
            switch(*ptr)
            {
               case ',': case '|':
                  pattern += "\\|";
               break;

               case '.': case '(': case ')':
                  pattern += '\\';
                  pattern += *ptr;
               break;

               case '*':
                  pattern += ".*";
               break;

               case '?':
                  pattern += '.';
               break;

               case '\\':
                  escapeMode = true;
               break;

               break;

               default:
                  pattern += *ptr;
               break;
            }
         }
      }
      pattern += "\\)$";
      //printf("OUTPUT: pattern became '%s'.\n", pattern.Cstr());
   }

   // Free the old regular expression, if any
   if (_regExpValid)
   {
     regfree(&_regExp);
     _regExpValid = false;
   }

   // And compile the new one
   _regExpValid = (regcomp(&_regExp, (pattern.Length() > 0) ? pattern.String() : str, 0) == 0);
   return _regExpValid;
}


bool
StringMatcher::Match(const char *str) const
{
	if (_regExpValid == false)
		return false;

	int regExpStat = regexec(&_regExp, str, 0, NULL, 0);

	return (regExpStat != REG_NOMATCH);
}


bool IsRegexToken(char c)
{
   switch(c)
   {
     case '[': case ']': case '*': case '?': case '\\': case ',': case '|': case '(': case ')':
        return true;

     default:
        return false;
   }
}

void EscapeRegexTokens(PortableString & s)
{
   const char * str = s.String();

   PortableString ret;
   while(*str)
   {
     if (IsRegexToken(*str)) ret += '\\';
     ret += *str;
     str++;
   }
   s = ret;
}

bool HasRegexTokens(const char * str)
{
   while(*str)
   {
     if (IsRegexToken(*str)) return true;
                        else str++;
   }
   return false;
}

bool MakeRegexCaseInsensitive(PortableString & str)
{
   bool changed = false;
   PortableString ret;
   for (uint32 i=0; i<(unsigned)str.Length(); i++)
   {
     char next = str[i];
     if ((next >= 'A')&&(next <= 'Z'))
     {
        char buf[5];
        sprintf(buf, "[%c%c]", next, next+('a'-'A'));
        ret += buf;
        changed = true;
     }
     else if ((next >= 'a')&&(next <= 'z'))
     {
        char buf[5];
        sprintf(buf, "[%c%c]", next, next+('A'-'a'));
        ret += buf;
        changed = true;
     }
     else ret += next;
   }
   if (changed) str = ret;
   return changed;
}
