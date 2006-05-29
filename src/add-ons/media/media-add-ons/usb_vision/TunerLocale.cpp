/*
 * This file is a part of BeOS USBVision driver project.
 * Copyright (c) 2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 *
 */
#include <Path.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <fstream.h>

#include "TunerLocale.h"

using namespace Locale;

const char *kLocalesSettings = "media/usb_vision/Locales";

Channel::Channel(string &str, uint32 frequency):
                 fName(str), fFrequency(frequency)
{
}

const string &Channel::Name() const
{
  return fName;
}
const uint32 Channel::Frequency()
{
  return fFrequency;
}

TunerLocale::TunerLocale(BEntry &entry)
{
  fStatus = B_NO_INIT;
  if(entry.IsFile()){
    BPath path;
    if((fStatus = entry.GetPath(&path)) == B_OK){
      char name[B_FILE_NAME_LENGTH];
      if((fStatus = entry.GetName(name)) == B_OK){
        fName = name;
        fStatus = LoadLocale(path.Path());
      }
    }
  }    
}

TunerLocale::~TunerLocale()
{
}

status_t TunerLocale::InitCheck()
{
  return fStatus;
}

status_t TunerLocale::LoadLocale(const char *name)
{
  status_t status = B_OK;
  ifstream ifs(name);
  while(ifs.good() && !ifs.eof()){
    string str;
    getline(ifs, str);
    //TODO : validity check and parse strings ... 
    str.erase(0, str.find_first_not_of(" \t"));
    str.erase(str.find_last_not_of(" \t") + 1);
    switch(str[0]){
    case ';': continue;
    case '@':
      str.erase(0, 1);
      fName = str;
      break;
    default: //TODO parse ...
      Channel channel(str, 0);
      fChannels.push_back(channel);
      break;
    }
  }
  return status;
}

status_t TunerLocale::LoadLocales(vector<TunerLocale *> &vector)
{
  status_t status = B_ERROR;
  BPath path;
  if((status = find_directory(B_USER_SETTINGS_DIRECTORY, &path)) != B_OK)
    return status;
  if((status = path.Append(kLocalesSettings)) != B_OK)
    return status;
  BDirectory directory(path.Path());  
  if((status = directory.InitCheck()) != B_OK)
    return status;
  vector.clear();
  BEntry entry;
  while((status = directory.GetNextEntry(&entry)) == B_OK){
    if(entry.IsFile()){
      TunerLocale *locale = new TunerLocale(entry);
      if((status = locale->InitCheck()) == B_OK){
        vector.push_back(locale);
      }
      else
        delete locale;  
    }    
  }
  if(status == B_ENTRY_NOT_FOUND) //we reached the end of entries list ...
    status = B_OK;
  return status;
}

status_t TunerLocale::ReleaseLocales(vector<TunerLocale *> &vector)
{
  for(LocalesIterator locale = vector.begin();
               locale != vector.end(); locale ++){
    delete *locale;
  }
  return B_OK;
}

const string &TunerLocale::Name()
{
  return fName;
} 

const Channel &TunerLocale::GetChannel(int idx)
{
  if(idx < 0) idx = 0;
  if(idx >= (int)fChannels.size()) idx = (int)(fChannels.size() - 1);
  return fChannels[idx];
}

uint32 TunerLocale::ChannelsCount()
{
  return fChannels.size();
}
