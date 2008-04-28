/*
 * This file is a part of BeOS USBVision driver project.
 * Copyright (c) 2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 *
 */
#ifndef _TUNER_LOCALE_H_
 #define _TUNER_LOCALE_H_

#include <string>
#include <vector>
#include <Entry.h>

namespace Locale{

class Channel
{
  string fName;
  uint32 fFrequency;
public:
  Channel(string &str, uint32 frequency);
  const string &Name() const;
  const uint32 Frequency();
};

typedef vector<Channel> Channels;
typedef vector<Channel>::iterator ChannelsIterator;

class TunerLocale
{
  status_t fStatus;
  string   fName;
  Channels fChannels;
  TunerLocale(BEntry &entry);
  status_t LoadLocale(const char *file);
public:
  ~TunerLocale();
  static status_t LoadLocales(vector<TunerLocale *> &vector);
  static status_t ReleaseLocales(vector<TunerLocale *> &vector);
  status_t InitCheck();
  const string &Name();
  const Channel &GetChannel(int idx);
  uint32 ChannelsCount();
};

typedef vector<TunerLocale *> Locales;
typedef vector<TunerLocale *>::iterator LocalesIterator;

} //namespace Locale

#endif//_TUNER_LOCALE_H_
