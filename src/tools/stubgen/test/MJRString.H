/**********************************************************************
 * FILE:        MJRString.H
 * AUTHOR:      Michael J. Radwin
 * DATE:        01/28/95
 * DESCRIPTION: The public interface for the String class
 * MODIFIED:    02/27/95
 * CREDITS:     some code borrowed from Deitel, "How to Program C++"
 **********************************************************************/

#ifndef __MJRString_H__
#define __MJRString_H__

#include <iostream.h>

class MJRString {
  friend ostream & operator<<(ostream&, const MJRString &s);
  friend istream & operator>>(istream&, MJRString &s);

public:
  MJRString(const char* s = "");
  MJRString(const MJRString &copy);
  MJRString(double value);
  MJRString(int value);
  ~MJRString();

  // assignment
  const MJRString & operator=(const MJRString &right);
  const MJRString & operator=(const char *right);
  const MJRString & operator+=(const MJRString &right);
  const MJRString & operator+=(const char *right);

  // comparisons
  int operator! () const;
  int operator==(const MJRString &right) const;
  int operator==(const char *right) const;
  int operator!=(const MJRString &right) const;
  int operator!=(const char *right) const;
  int operator< (const MJRString &right) const;
  int operator< (const char *right) const;
  int operator> (const MJRString &right) const;
  int operator> (const char *right) const;
  int operator<=(const MJRString &right) const;
  int operator<=(const char *right) const;
  int operator>=(const MJRString &right) const;
  int operator>=(const char *right) const;
  
  // auxiliary
  MJRString operator+(const MJRString &right) const;
  MJRString operator+(const char *right) const;
  char & operator[](int subscript);
  char operator[](int subscript) const; // safe for const
  MJRString operator()(int index, int subLength) const;
  int length() const;

protected:
  char *pStr_;
  int length_;
} ;

#endif
